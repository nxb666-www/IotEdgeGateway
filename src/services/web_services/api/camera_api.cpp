#include "camera_api.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>

#ifndef _WIN32
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

const char* kBoardWwwDir = "www";
const char* kStreamDir = "/tmp/rk_stream";
const char* kLiveJpg = "/tmp/rk_stream/live.jpg";
std::string g_video_device = "/dev/video0";

struct CameraState {
    int stream_on = 0;
    int record_on = 0;
#ifndef _WIN32
    pid_t stream_pid = 0;
    pid_t record_pid = 0;
#else
    int stream_pid = 0;
    int record_pid = 0;
#endif
    char record_file[256] = {0};
    char record_dir[256] = {0};
};

CameraState g_camera;

void ReplyJson(mg_connection* c, int code, const std::string& body) {
    mg_http_reply(c, code, "Content-Type: application/json\r\n",
                  "%s\n", body.c_str());
}

#ifndef _WIN32
bool IsCompleteJpg(const char* path);
std::string ReadSmallTextFile(const std::string& path);

std::vector<std::string> BuildVideoCandidates() {
    std::vector<std::string> devices;
    auto add_unique = [&](const std::string& device) {
        if (device.empty()) return;
        for (const auto& existing : devices) {
            if (existing == device) return;
        }
        devices.push_back(device);
    };

    add_unique(g_video_device);
    for (int i = 0; i <= 15; ++i) {
        char path[32];
        std::snprintf(path, sizeof(path), "/dev/video%d", i);
        struct stat st;
        if (stat(path, &st) == 0) {
            std::string name = ReadSmallTextFile(std::string("/sys/class/video4linux/video") +
                                                std::to_string(i) + "/name");
            if (name.find("mainpath") != std::string::npos ||
                name.find("selfpath") != std::string::npos ||
                name.find("USB") != std::string::npos ||
                name.find("uvc") != std::string::npos ||
                name.find("camera") != std::string::npos) {
                add_unique(path);
            }
        }
    }

    add_unique("/dev/video0");
    add_unique("/dev/video1");
    return devices;
}

pid_t StartFfmpegPreview(const std::string& device) {
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "r", stdin);
        freopen("stream/ffmpeg.log", "a", stderr);
        execlp("ffmpeg", "ffmpeg",
               "-nostdin", "-y",
               "-loglevel", "warning",
               "-f", "v4l2",
               "-framerate", "25",
               "-i", device.c_str(),
               "-an",
               "-vf", "scale=640:-1,fps=10",
               "-q:v", "6",
               "-update", "1",
               kLiveJpg,
               nullptr);
        _exit(1);
    }
    return pid;
}

pid_t StartGstreamerPreview(const std::string& device) {
    std::string cmd =
        "command -v gst-launch-1.0 >/dev/null 2>&1 || exit 127; "
        "while true; do "
        "gst-launch-1.0 -q v4l2src device=" + device + " num-buffers=1 "
        "! video/x-raw,width=800,height=600 "
        "! videoconvert ! jpegenc "
        "! filesink location=/tmp/rk_stream/live.jpg.tmp "
        "&& mv /tmp/rk_stream/live.jpg.tmp /tmp/rk_stream/live.jpg; "
        "usleep 100000 2>/dev/null || sleep 1; "
        "done";

    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "r", stdin);
        freopen("stream/gstreamer.log", "a", stderr);
        execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(1);
    }
    return pid;
}

bool WaitForPreviewFrame(pid_t pid, int wait_ms) {
    int waited = 0;
    while (waited < wait_ms) {
        int status = 0;
        if (waitpid(pid, &status, WNOHANG) == pid) {
            return false;
        }
        if (IsCompleteJpg(kLiveJpg)) {
            return true;
        }
        usleep(100000);
        waited += 100;
    }
    return false;
}

bool ChildStillRunning(pid_t pid) {
    if (pid <= 0) return false;
    int status = 0;
    pid_t ret = waitpid(pid, &status, WNOHANG);
    return ret == 0;
}

void StopChild(pid_t pid) {
    if (pid <= 0) return;
    kill(pid, SIGTERM);
    for (int i = 0; i < 10; i++) {
        if (waitpid(pid, nullptr, WNOHANG) == pid) return;
        usleep(100000);
    }
    kill(pid, SIGKILL);
    waitpid(pid, nullptr, WNOHANG);
}

void MakeMediaPath(char* buf, size_t len, const char* prefix, const char* ext) {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    std::strftime(buf, len, "www/", &tm_now);
    size_t used = std::strlen(buf);
    std::strftime(buf + used, len - used, prefix, &tm_now);
    std::snprintf(buf + std::strlen(buf), len - std::strlen(buf), ".%s", ext);
}

int EnsureDir(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return 0;
    return mkdir(path, 0755);
}

int CopyFile(const char* src, const char* dst) {
    FILE* in = std::fopen(src, "rb");
    if (!in) return -1;
    FILE* out = std::fopen(dst, "wb");
    if (!out) {
        std::fclose(in);
        return -1;
    }

    unsigned char buf[8192];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), in)) > 0) {
        if (std::fwrite(buf, 1, n, out) != n) {
            std::fclose(in);
            std::fclose(out);
            return -1;
        }
    }
    int ret = std::ferror(in) ? -1 : 0;
    std::fclose(in);
    std::fclose(out);
    return ret;
}

bool IsCompleteJpg(const char* path) {
    FILE* fp = std::fopen(path, "rb");
    if (!fp) return false;
    if (std::fseek(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        return false;
    }
    long size = std::ftell(fp);
    if (size < 4) {
        std::fclose(fp);
        return false;
    }
    unsigned char head[2] = {0}, tail[2] = {0};
    std::rewind(fp);
    bool ok = std::fread(head, 1, 2, fp) == 2;
    ok = ok && std::fseek(fp, -2, SEEK_END) == 0;
    ok = ok && std::fread(tail, 1, 2, fp) == 2;
    std::fclose(fp);
    return ok && head[0] == 0xff && head[1] == 0xd8 &&
           tail[0] == 0xff && tail[1] == 0xd9;
}

void RecordFramesLoop(const char* dir) {
    int frame = 1;
    for (;;) {
        char frame_file[320];
        std::snprintf(frame_file, sizeof(frame_file), "%s/frame_%06d.jpg", dir, frame);
        if (IsCompleteJpg(kLiveJpg) && CopyFile(kLiveJpg, frame_file) == 0) {
            frame++;
        }
        usleep(100000);
    }
}

std::string ReadSmallTextFile(const std::string& path) {
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) return "";
    char buf[256];
    size_t n = std::fread(buf, 1, sizeof(buf) - 1, fp);
    std::fclose(fp);
    buf[n] = '\0';
    std::string out(buf);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }
    return out;
}
#endif

} // namespace

void CameraApi::SetVideoDevice(const std::string& device) {
    if (!device.empty()) {
        g_video_device = device;
    }
}

bool CameraApi::CheckStreamProcess() {
#ifdef _WIN32
    return false;
#else
    if (!g_camera.stream_on || g_camera.stream_pid <= 0) return false;
    int status = 0;
    pid_t ret = waitpid(g_camera.stream_pid, &status, WNOHANG);
    if (ret == 0) return true;
    if (ret == g_camera.stream_pid) {
        g_camera.stream_on = 0;
        g_camera.stream_pid = 0;
    }
    return false;
#endif
}

void CameraApi::HandleStartStream(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    if (CheckStreamProcess()) {
        ReplyJson(c, 200, "{\"result\":\"already_running\",\"mode\":\"preview\",\"backend\":\"ffmpeg\"}");
        return;
    }

    EnsureDir("stream");
    EnsureDir(kStreamDir);
    std::remove(kLiveJpg);

    for (const auto& device : BuildVideoCandidates()) {
        pid_t pid = StartFfmpegPreview(device);
        if (pid <= 0) {
            ReplyJson(c, 500, "{\"error\":\"fork_failed\"}");
            return;
        }
        if (WaitForPreviewFrame(pid, 1200)) {
            g_video_device = device;
            g_camera.stream_on = 1;
            g_camera.stream_pid = pid;
            ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"mode\":\"preview\",\"backend\":\"ffmpeg\",\"device\":\"")
                + device + "\"}");
            return;
        }
        StopChild(pid);
    }

    for (const auto& device : BuildVideoCandidates()) {
        pid_t pid = StartGstreamerPreview(device);
        if (pid <= 0) {
            ReplyJson(c, 500, "{\"error\":\"fork_failed\"}");
            return;
        }
        if (WaitForPreviewFrame(pid, 2500)) {
            g_video_device = device;
            g_camera.stream_on = 1;
            g_camera.stream_pid = pid;
            ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"mode\":\"preview\",\"backend\":\"gstreamer\",\"device\":\"")
                + device + "\"}");
            return;
        }
        StopChild(pid);
    }

    ReplyJson(c, 500, "{\"error\":\"camera_preview_failed\",\"log\":\"stream/ffmpeg.log,stream/gstreamer.log\"}");
#endif
}

void CameraApi::HandleStopStream(mg_connection* c) {
#ifndef _WIN32
    if (g_camera.stream_on && g_camera.stream_pid > 0) {
        kill(g_camera.stream_pid, SIGTERM);
        waitpid(g_camera.stream_pid, nullptr, WNOHANG);
    }
#endif
    g_camera.stream_on = 0;
    g_camera.stream_pid = 0;
    ReplyJson(c, 200, "{\"result\":\"ok\"}");
}

void CameraApi::HandleSnapshot(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    if (EnsureDir(kBoardWwwDir) != 0) {
        ReplyJson(c, 500, "{\"error\":\"cannot_create_www\"}");
        return;
    }

    char filename[256];
    MakeMediaPath(filename, sizeof(filename), "snap_%Y%m%d_%H%M%S", "jpg");

    int ret = -1;
    if (IsCompleteJpg(kLiveJpg)) {
        ret = CopyFile(kLiveJpg, filename);
    }
    if (ret != 0) {
        char cmd[512];
        std::snprintf(cmd, sizeof(cmd),
            "ffmpeg -nostdin -y -loglevel error -f v4l2 "
            "-i %s -frames:v 1 -q:v 3 '%s'",
            g_video_device.c_str(), filename);
        ret = system(cmd);
    }

    ReplyJson(c, 200, std::string("{\"result\":\"") +
        (ret == 0 ? "ok" : "fail") + "\",\"file\":\"" + filename + "\"}");
#endif
}

void CameraApi::HandleStartRecord(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    if (g_camera.record_on && g_camera.record_pid > 0) {
        ReplyJson(c, 200, std::string("{\"result\":\"already_recording\",\"file\":\"") +
            g_camera.record_file + "\"}");
        return;
    }
    if (!CheckStreamProcess()) {
        ReplyJson(c, 400, "{\"error\":\"please_start_stream_first\"}");
        return;
    }
    if (EnsureDir(kBoardWwwDir) != 0) {
        ReplyJson(c, 500, "{\"error\":\"cannot_create_www\"}");
        return;
    }

    MakeMediaPath(g_camera.record_file, sizeof(g_camera.record_file),
                  "rec_%Y%m%d_%H%M%S", "mp4");
    std::snprintf(g_camera.record_dir, sizeof(g_camera.record_dir),
                  "/tmp/rk_record_%ld", (long) time(nullptr));
    EnsureDir(g_camera.record_dir);

    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "r", stdin);
        freopen("stream/record.log", "a", stderr);
        RecordFramesLoop(g_camera.record_dir);
        _exit(0);
    }
    if (pid <= 0) {
        g_camera.record_file[0] = '\0';
        g_camera.record_dir[0] = '\0';
        ReplyJson(c, 500, "{\"error\":\"fork_failed\"}");
        return;
    }

    g_camera.record_on = 1;
    g_camera.record_pid = pid;
    ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"file\":\"") +
        g_camera.record_file + "\"}");
#endif
}

void CameraApi::HandleStopRecord(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    char saved_file[256];
    char saved_dir[256];
    std::snprintf(saved_file, sizeof(saved_file), "%s", g_camera.record_file);
    std::snprintf(saved_dir, sizeof(saved_dir), "%s", g_camera.record_dir);

    if (g_camera.record_on && g_camera.record_pid > 0) {
        kill(g_camera.record_pid, SIGTERM);
        for (int i = 0; i < 30; i++) {
            if (waitpid(g_camera.record_pid, nullptr, WNOHANG) == g_camera.record_pid) break;
            usleep(100000);
        }
        kill(g_camera.record_pid, SIGKILL);
        waitpid(g_camera.record_pid, nullptr, WNOHANG);
    }

    int ret = -1;
    if (saved_dir[0] && saved_file[0]) {
        char cmd[768];
        std::snprintf(cmd, sizeof(cmd),
            "ffmpeg -nostdin -y -loglevel error -framerate 10 "
            "-i '%s/frame_%%06d.jpg' -an -c:v mpeg4 -q:v 4 "
            "-movflags +faststart '%s' >> stream/record.log 2>&1",
            saved_dir, saved_file);
        ret = system(cmd);
    }

    g_camera.record_on = 0;
    g_camera.record_pid = 0;
    g_camera.record_file[0] = '\0';
    g_camera.record_dir[0] = '\0';

    ReplyJson(c, 200, std::string("{\"result\":\"") +
        (ret == 0 ? "ok" : "fail") + "\",\"file\":\"" + saved_file + "\"}");
#endif
}

void CameraApi::HandleLiveJpg(mg_connection* c) {
    FILE* fp = std::fopen(kLiveJpg, "rb");
    if (!fp) {
        mg_http_reply(c, 503,
            "Content-Type: text/plain\r\nCache-Control: no-cache\r\n",
            "live frame not ready\n");
        return;
    }

    if (std::fseek(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        mg_http_reply(c, 500, "Content-Type: text/plain\r\n", "seek failed\n");
        return;
    }
    long file_size = std::ftell(fp);
    if (file_size <= 0 || file_size > 2 * 1024 * 1024) {
        std::fclose(fp);
        mg_http_reply(c, 503,
            "Content-Type: text/plain\r\nCache-Control: no-cache\r\n",
            "invalid live frame\n");
        return;
    }
    std::rewind(fp);

    unsigned char* buf = static_cast<unsigned char*>(std::malloc((size_t) file_size));
    if (!buf) {
        std::fclose(fp);
        mg_http_reply(c, 500, "Content-Type: text/plain\r\n", "oom\n");
        return;
    }

    size_t nread = std::fread(buf, 1, (size_t) file_size, fp);
    std::fclose(fp);
    if (nread < 4 || buf[0] != 0xff || buf[1] != 0xd8 ||
        buf[nread - 2] != 0xff || buf[nread - 1] != 0xd9) {
        std::free(buf);
        mg_http_reply(c, 503,
            "Content-Type: text/plain\r\nCache-Control: no-cache\r\n",
            "incomplete live frame\n");
        return;
    }

    mg_printf(c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %lu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Expires: 0\r\n"
        "Connection: close\r\n\r\n",
        (unsigned long) nread);
    mg_send(c, buf, nread);
    c->is_draining = 1;
    std::free(buf);
}

void CameraApi::HandleProbe(mg_connection* c) {
#ifdef _WIN32
    mg_http_reply(c, 501, "Content-Type: text/plain\r\n",
                  "camera probe is only supported on Linux\n");
#else
    std::string body;
    body += "[configured]\n";
    body += g_video_device + "\n\n";

    body += "[video nodes]\n";
    DIR* dir = opendir("/dev");
    if (!dir) {
        body += "cannot open /dev\n";
    } else {
        struct dirent* ent = nullptr;
        while ((ent = readdir(dir)) != nullptr) {
            if (std::strncmp(ent->d_name, "video", 5) != 0) continue;
            std::string name = ent->d_name;
            std::string path = "/dev/" + name;
            struct stat st;
            if (stat(path.c_str(), &st) != 0) continue;
            body += path;
            std::string sys_name = ReadSmallTextFile("/sys/class/video4linux/" + name + "/name");
            if (!sys_name.empty()) {
                body += "  ";
                body += sys_name;
            }
            body += "\n";
        }
        closedir(dir);
    }

    body += "\n[stream]\n";
    body += g_camera.stream_on ? "on\n" : "off\n";
    body += "device=" + g_video_device + "\n";

    mg_http_reply(c, 200, "Content-Type: text/plain\r\n",
                  "%s", body.c_str());
#endif
}
