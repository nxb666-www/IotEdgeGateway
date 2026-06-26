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
const char* kPhotosDir = "data/media/photos";
const char* kVideosDir = "data/media/videos";
const char* kLogsDir = "data/logs";
const int kStreamPort = 8090;
const char* kStreamPath = "/stream.mjpg";
const char* kLocalStreamUrl = "http://127.0.0.1:8090/stream.mjpg";
const char* kMjpegMagic = "MJPG";
const uint64_t kMjpegMinIntervalMs = 66;
std::string g_video_device = "/dev/video0";

struct MjpegConnState {
    char magic[4];
    uint64_t last_send_ms;
};

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
    int stream_backend = 0; // 0 none, 1 ffmpeg http, 2 gstreamer live.jpg
    int record_backend = 0; // 1 ffmpeg http, 2 live.jpg frames
};

CameraState g_camera;

void ReplyJson(mg_connection* c, int code, const std::string& body) {
    mg_http_reply(c, code, "Content-Type: application/json\r\n",
                  "%s\n", body.c_str());
}

#ifndef _WIN32
bool IsCompleteJpg(const char* path);
std::string ReadSmallTextFile(const std::string& path);
int EnsureDir(const char* path);
bool ReadLiveFrame(std::vector<unsigned char>& frame);
bool IsTcpPortListening(int port);
bool IsFreshJpg(const char* path, int max_age_sec);

std::string AppPath(const char* rel) {
    if (rel == nullptr || rel[0] == '\0') return "";
    if (rel[0] == '/') return rel;
    const char* app_dir = std::getenv("IOTGW_APP_DIR");
    if (app_dir != nullptr && app_dir[0] != '\0') {
        return std::string(app_dir) + "/" + rel;
    }
    return rel;
}

int GetEnvInt(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') return fallback;
    char* end = nullptr;
    long parsed = std::strtol(value, &end, 10);
    return end && *end == '\0' ? static_cast<int>(parsed) : fallback;
}

std::string BuildFfmpegVideoFilter() {
    int rotation = GetEnvInt("IOTGW_VIDEO_ROTATION", 270);
    int width = GetEnvInt("IOTGW_VIDEO_WIDTH", 640);
    int fps = GetEnvInt("IOTGW_VIDEO_FPS", 15);

    std::string filter;
    if (rotation == 90) {
        filter = "transpose=1,";
    } else if (rotation == 270) {
        filter = "transpose=2,";
    } else if (rotation == 180) {
        filter = "transpose=1,transpose=1,";
    }
    filter += "scale=" + std::to_string(width) + ":-1,fps=" + std::to_string(fps);
    return filter;
}

std::string BuildGstreamerTransform() {
    int rotation = GetEnvInt("IOTGW_VIDEO_ROTATION", 270);
    if (rotation == 90) return "! videoflip method=clockwise ";
    if (rotation == 270) return "! videoflip method=counterclockwise ";
    if (rotation == 180) return "! videoflip method=rotate-180 ";
    return "";
}

std::vector<std::string> BuildVideoCandidates() {
    std::vector<std::string> devices;
    auto add_unique = [&](const std::string& device) {
        if (device.empty()) return;
        for (const auto& existing : devices) {
            if (existing == device) return;
        }
        devices.push_back(device);
    };

    const char* env_device = std::getenv("IOTGW_VIDEO_DEVICE");
    if (env_device != nullptr && env_device[0] != '\0') {
        add_unique(env_device);
    }
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
                name.find("rkisp") != std::string::npos ||
                name.find("rkcif") != std::string::npos ||
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

pid_t StartFfmpegHttpStream(const std::string& device) {
    std::string logs_dir = AppPath(kLogsDir);
    EnsureDir(logs_dir.c_str());
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "r", stdin);
        std::string log_path = logs_dir + "/ffmpeg_stream.log";
        freopen(log_path.c_str(), "a", stderr);
        std::string filter = BuildFfmpegVideoFilter();
        std::string quality = std::to_string(GetEnvInt("IOTGW_VIDEO_QUALITY", 6));
        std::string fps = std::to_string(GetEnvInt("IOTGW_VIDEO_FPS", 15));
        // 先尝试 mjpeg 输入（CSI 摄像头常见），失败用 nv12
        execlp("sh", "sh", "-c",
            ("ffmpeg -nostdin -y -loglevel warning -fflags nobuffer -flags low_delay "
             "-f v4l2 -input_format mjpeg -video_size 640x480 -framerate " + fps + " -i " + device +
             " -an -vf " + filter + " -q:v " + quality +
             " -update 1 " + std::string(kLiveJpg) + " 2>/dev/null || "
             "ffmpeg -nostdin -y -loglevel warning -fflags nobuffer -flags low_delay "
             "-f v4l2 -input_format nv12 -video_size 640x480 -framerate " + fps + " -i " + device +
             " -an -vf " + filter + " -q:v " + quality +
             " -update 1 " + std::string(kLiveJpg)).c_str(),
            nullptr);
        _exit(1);
    }
    return pid;
}

pid_t StartGstreamerPreview(const std::string& device) {
    std::string transform = BuildGstreamerTransform();
    // 优先尝试 MJPEG 直出（RK3568 CSI 常见），失败再用 raw + jpegenc
    std::string cmd =
        "command -v gst-launch-1.0 >/dev/null 2>&1 || exit 127; "
        "mkdir -p /tmp/rk_stream; "
        "try_mjpeg() { "
        "  gst-launch-1.0 -q v4l2src device=" + device + " num-buffers=1 "
        "! image/jpeg,width=640,height=480,framerate=15/1 "
        "! filesink location=/tmp/rk_stream/live.jpg.tmp; "
        "}; "
        "try_raw() { "
        "  gst-launch-1.0 -q v4l2src device=" + device + " num-buffers=1 "
        "! video/x-raw,width=640,height=480 "
        "! videoconvert " + transform + "! jpegenc quality=70 "
        "! filesink location=/tmp/rk_stream/live.jpg.tmp; "
        "}; "
        "while true; do "
        "  if try_mjpeg 2>/dev/null || try_raw 2>/dev/null; then "
        "    mv -f /tmp/rk_stream/live.jpg.tmp /tmp/rk_stream/live.jpg; "
        "  fi; "
        "  usleep 100000 2>/dev/null || sleep 1; "
        "done";

    std::string logs_dir = AppPath(kLogsDir);
    EnsureDir(logs_dir.c_str());
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "r", stdin);
        std::string log_path = logs_dir + "/gstreamer.log";
        freopen(log_path.c_str(), "a", stderr);
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

bool WaitForHttpStream(pid_t pid, int wait_ms) {
    int waited = 0;
    while (waited < wait_ms) {
        int status = 0;
        if (waitpid(pid, &status, WNOHANG) == pid) {
            return false;
        }
        if (IsTcpPortListening(kStreamPort)) {
            return true;
        }
        usleep(100000);
        waited += 100;
    }
    int status = 0;
    return waitpid(pid, &status, WNOHANG) == 0 && IsTcpPortListening(kStreamPort);
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

void MakeMediaPath(char* buf, size_t len, const char* dir, const char* prefix, const char* ext) {
    EnsureDir(dir);
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    std::snprintf(buf, len, "%s/", dir);
    size_t used = std::strlen(buf);
    std::strftime(buf + used, len - used, prefix, &tm_now);
    std::snprintf(buf + std::strlen(buf), len - std::strlen(buf), ".%s", ext);
}

int EnsureDir(const char* path) {
    if (path == nullptr || path[0] == '\0') return -1;

    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s", path);
    size_t len = std::strlen(buf);
    if (len == 0 || len >= sizeof(buf)) return -1;

    if (buf[len - 1] == '/') {
        buf[len - 1] = '\0';
    }

    for (char* p = buf + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            struct stat st;
            if (stat(buf, &st) != 0) {
                if (mkdir(buf, 0755) != 0) return -1;
            } else if (!S_ISDIR(st.st_mode)) {
                return -1;
            }
            *p = '/';
        }
    }

    struct stat st;
    if (stat(buf, &st) == 0) return S_ISDIR(st.st_mode) ? 0 : -1;
    return mkdir(buf, 0755);
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

bool IsFreshJpg(const char* path, int max_age_sec) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    time_t now = time(nullptr);
    if (now < st.st_mtime || now - st.st_mtime > max_age_sec) return false;
    return IsCompleteJpg(path);
}

bool IsTcpPortListening(int port) {
    char needle[16];
    std::snprintf(needle, sizeof(needle), ":%04X", port);

    const char* files[] = {"/proc/net/tcp", "/proc/net/tcp6"};
    char line[512];
    for (const char* file : files) {
        FILE* fp = std::fopen(file, "r");
        if (!fp) continue;
        while (std::fgets(line, sizeof(line), fp)) {
            char local[128] = {0};
            char remote[128] = {0};
            char state[8] = {0};
            if (std::sscanf(line, " %*d: %127s %127s %7s", local, remote, state) == 3) {
                if (std::strstr(local, needle) != nullptr && std::strcmp(state, "0A") == 0) {
                    std::fclose(fp);
                    return true;
                }
            }
        }
        std::fclose(fp);
    }
    return false;
}

bool ReadLiveFrame(std::vector<unsigned char>& frame) {
    frame.clear();
    FILE* fp = std::fopen(kLiveJpg, "rb");
    if (!fp) return false;

    if (std::fseek(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        return false;
    }
    long file_size = std::ftell(fp);
    if (file_size <= 4 || file_size > 2 * 1024 * 1024) {
        std::fclose(fp);
        return false;
    }
    std::rewind(fp);

    frame.resize(static_cast<size_t>(file_size));
    size_t nread = std::fread(frame.data(), 1, frame.size(), fp);
    std::fclose(fp);
    if (nread != frame.size()) {
        frame.clear();
        return false;
    }

    return frame[0] == 0xff && frame[1] == 0xd8 &&
           frame[frame.size() - 2] == 0xff && frame[frame.size() - 1] == 0xd9;
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

bool EnsurePreviewRunning() {
    if (g_camera.stream_on && g_camera.stream_pid > 0 &&
        ChildStillRunning(g_camera.stream_pid) && IsFreshJpg(kLiveJpg, 3)) {
        return true;
    }

    StopChild(g_camera.stream_pid);
    g_camera.stream_on = 0;
    g_camera.stream_pid = 0;
    g_camera.stream_backend = 0;

    EnsureDir(kStreamDir);
    std::remove(kLiveJpg);

    for (const auto& device : BuildVideoCandidates()) {
        pid_t pid = StartFfmpegHttpStream(device);
        if (pid > 0 && WaitForPreviewFrame(pid, 2500)) {
            g_video_device = device;
            g_camera.stream_on = 1;
            g_camera.stream_pid = pid;
            g_camera.stream_backend = 2;
            return true;
        }
        StopChild(pid);
    }

    for (const auto& device : BuildVideoCandidates()) {
        pid_t pid = StartGstreamerPreview(device);
        if (pid > 0 && WaitForPreviewFrame(pid, 2500)) {
            g_video_device = device;
            g_camera.stream_on = 1;
            g_camera.stream_pid = pid;
            g_camera.stream_backend = 2;
            return true;
        }
        StopChild(pid);
    }

    return false;
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
    if (ret == 0) {
        bool output_ok = false;
        if (g_camera.stream_backend == 1) {
            output_ok = IsTcpPortListening(kStreamPort);
        } else if (g_camera.stream_backend == 2) {
            output_ok = IsFreshJpg(kLiveJpg, 3);
        }

        if (output_ok) return true;

        StopChild(g_camera.stream_pid);
        g_camera.stream_on = 0;
        g_camera.stream_pid = 0;
        g_camera.stream_backend = 0;
        return false;
    }

    if (ret == g_camera.stream_pid) {
        g_camera.stream_on = 0;
        g_camera.stream_pid = 0;
        g_camera.stream_backend = 0;
    }
    return false;
#endif
}

void CameraApi::HandleStartStream(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    if (CheckStreamProcess()) {
        ReplyJson(c, 200, "{\"result\":\"already_running\",\"mode\":\"mjpeg\",\"path\":\"/api/camera/stream\"}");
        return;
    }

    EnsureDir("stream");
    EnsureDir(kStreamDir);
    std::remove(kLiveJpg);

    for (const auto& device : BuildVideoCandidates()) {
        pid_t pid = StartFfmpegHttpStream(device);
        if (pid <= 0) {
            ReplyJson(c, 500, "{\"error\":\"fork_failed\"}");
            return;
        }
        if (WaitForPreviewFrame(pid, 2500)) {
            g_video_device = device;
            g_camera.stream_on = 1;
            g_camera.stream_pid = pid;
            g_camera.stream_backend = 2;
            ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"mode\":\"mjpeg\",\"backend\":\"ffmpeg\",\"device\":\"")
                + device + "\",\"path\":\"/api/camera/stream\"}");
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
            g_camera.stream_backend = 2;
            ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"mode\":\"mjpeg_poll\",\"backend\":\"gstreamer\",\"device\":\"")
                + device + "\",\"path\":\"/api/camera/stream\"}");
            return;
        }
        StopChild(pid);
    }

    ReplyJson(c, 500, std::string("{\"error\":\"camera_stream_failed\",\"log\":\"") + kLogsDir + "/ffmpeg_stream.log," + kLogsDir + "/gstreamer.log\"}");
#endif
}

void CameraApi::HandleStopStream(mg_connection* c) {
#ifndef _WIN32
    StopChild(g_camera.stream_pid);
#endif
    g_camera.stream_on = 0;
    g_camera.stream_pid = 0;
    g_camera.stream_backend = 0;
    ReplyJson(c, 200, "{\"result\":\"ok\"}");
}

void CameraApi::HandleSnapshot(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    std::string photos_dir = AppPath(kPhotosDir);
    if (EnsureDir(photos_dir.c_str()) != 0) {
        ReplyJson(c, 500, "{\"error\":\"cannot_create_photos_dir\"}");
        return;
    }

    char filename[256];
    MakeMediaPath(filename, sizeof(filename), photos_dir.c_str(), "snap_%Y%m%d_%H%M%S", "jpg");

    int ret = -1;
    EnsurePreviewRunning();
    if (IsCompleteJpg(kLiveJpg)) {
        ret = CopyFile(kLiveJpg, filename);
    }
    if (ret != 0 && CheckStreamProcess() && g_camera.stream_backend == 1) {
        char cmd[768];
        std::snprintf(cmd, sizeof(cmd),
            "ffmpeg -nostdin -y -loglevel error -i '%s' "
            "-frames:v 1 -q:v 3 '%s'",
            kLocalStreamUrl, filename);
        ret = system(cmd);
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
    if (!EnsurePreviewRunning()) {
        ReplyJson(c, 400, "{\"error\":\"please_start_stream_first\"}");
        return;
    }
    std::string videos_dir = AppPath(kVideosDir);
    if (EnsureDir(videos_dir.c_str()) != 0) {
        ReplyJson(c, 500, "{\"error\":\"cannot_create_videos_dir\"}");
        return;
    }

    MakeMediaPath(g_camera.record_file, sizeof(g_camera.record_file),
                  videos_dir.c_str(), "rec_%Y%m%d_%H%M%S", "mp4");

    std::string logs_dir = AppPath(kLogsDir);
    EnsureDir(logs_dir.c_str());

    if (g_camera.stream_backend == 2) {
        std::snprintf(g_camera.record_dir, sizeof(g_camera.record_dir),
                      "/tmp/rk_record_%ld", static_cast<long>(time(nullptr)));
        if (EnsureDir(g_camera.record_dir) != 0) {
            g_camera.record_file[0] = '\0';
            g_camera.record_dir[0] = '\0';
            ReplyJson(c, 500, "{\"error\":\"cannot_create_record_dir\"}");
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            freopen("/dev/null", "r", stdin);
            std::string log_path = logs_dir + "/record.log";
            freopen(log_path.c_str(), "a", stderr);
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
        g_camera.record_backend = 2;
        ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"backend\":\"gstreamer\",\"file\":\"") +
            g_camera.record_file + "\"}");
        return;
    }

    if (g_camera.stream_backend != 1) {
        g_camera.record_file[0] = '\0';
        ReplyJson(c, 400, "{\"error\":\"stream_backend_not_ready\"}");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "r", stdin);
        std::string log_path = logs_dir + "/record.log";
        freopen(log_path.c_str(), "a", stderr);
        execlp("ffmpeg", "ffmpeg",
               "-nostdin", "-y",
               "-loglevel", "warning",
               "-i", kLocalStreamUrl,
               "-an",
               "-c:v", "mpeg4",
               "-q:v", "4",
               "-movflags", "+faststart",
               g_camera.record_file,
               nullptr);
        _exit(1);
    }
    if (pid <= 0) {
        g_camera.record_file[0] = '\0';
        ReplyJson(c, 500, "{\"error\":\"fork_failed\"}");
        return;
    }

    g_camera.record_on = 1;
    g_camera.record_pid = pid;
    g_camera.record_backend = 1;
    ReplyJson(c, 200, std::string("{\"result\":\"ok\",\"backend\":\"ffmpeg\",\"file\":\"") +
        g_camera.record_file + "\"}");
#endif
}

void CameraApi::HandleStopRecord(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"camera_not_supported_on_windows\"}");
#else
    char saved_file[256];
    std::snprintf(saved_file, sizeof(saved_file), "%s", g_camera.record_file);
    char saved_dir[256];
    std::snprintf(saved_dir, sizeof(saved_dir), "%s", g_camera.record_dir);
    int saved_backend = g_camera.record_backend;

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
    if (saved_backend == 2 && saved_file[0] && saved_dir[0]) {
        std::string logs_dir = AppPath(kLogsDir);
        EnsureDir(logs_dir.c_str());
        char cmd[768];
        std::snprintf(cmd, sizeof(cmd),
            "ffmpeg -nostdin -y -loglevel warning -framerate 10 "
            "-start_number 1 -i '%s/frame_%%06d.jpg' -an -c:v mpeg4 -q:v 4 "
            "-movflags +faststart '%s' >> '%s/record.log' 2>&1",
            saved_dir, saved_file, logs_dir.c_str());
        ret = system(cmd);
    }

    if (saved_file[0]) {
        struct stat st;
        if (stat(saved_file, &st) == 0 && st.st_size > 0) {
            ret = 0;
        }
    }

    g_camera.record_on = 0;
    g_camera.record_pid = 0;
    g_camera.record_file[0] = '\0';
    g_camera.record_dir[0] = '\0';
    g_camera.record_backend = 0;

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

void CameraApi::HandleMjpegStream(mg_connection* c) {
#ifdef _WIN32
    ReplyJson(c, 501, "{\"error\":\"mjpeg_not_supported_on_windows\"}");
#else
    if (!CheckStreamProcess()) {
        ReplyJson(c, 503, "{\"error\":\"stream_not_running\",\"hint\":\"call /api/camera/start_stream first\"}");
        return;
    }

    std::memset(c->data, 0, sizeof(c->data));
    auto* state = reinterpret_cast<MjpegConnState*>(c->data);
    std::memcpy(state->magic, kMjpegMagic, sizeof(state->magic));
    state->last_send_ms = 0;
    c->is_resp = 1;
    mg_printf(c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Expires: 0\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n");
    HandleMjpegPoll(c);
#endif
}

bool CameraApi::IsMjpegConnection(mg_connection* c) {
    if (c == nullptr) return false;
    auto* state = reinterpret_cast<MjpegConnState*>(c->data);
    return std::memcmp(state->magic, kMjpegMagic, sizeof(state->magic)) == 0;
}

void CameraApi::HandleMjpegPoll(mg_connection* c) {
#ifndef _WIN32
    if (!IsMjpegConnection(c)) return;
    if (c->send.len > 512 * 1024) return;
    auto* state = reinterpret_cast<MjpegConnState*>(c->data);
    uint64_t now = mg_millis();
    if (state->last_send_ms != 0 && now - state->last_send_ms < kMjpegMinIntervalMs) {
        return;
    }

    std::vector<unsigned char> frame;
    if (!ReadLiveFrame(frame)) return;
    state->last_send_ms = now;

    mg_printf(c,
        "--frame\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %lu\r\n\r\n",
        static_cast<unsigned long>(frame.size()));
    mg_send(c, frame.data(), frame.size());
    mg_send(c, "\r\n", 2);
#else
    (void)c;
#endif
}

static std::string ListDirJson(const std::string& dir, const std::string& ext_filter) {
    std::string json = "[";
    bool first = true;
#ifndef _WIN32
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* ent;
        while ((ent = readdir(d)) != nullptr) {
            std::string name = ent->d_name;
            if (name == "." || name == "..") continue;
            if (!ext_filter.empty()) {
                size_t dot = name.rfind('.');
                if (dot == std::string::npos) continue;
                std::string ext = name.substr(dot);
                if (ext != ext_filter) continue;
            }
            std::string path = dir + "/" + name;
            struct stat st;
            if (stat(path.c_str(), &st) != 0) continue;
            if (!first) json += ",";
            first = false;
            json += "{\"name\":\"" + name + "\",\"size\":" + std::to_string(st.st_size) +
                    ",\"mtime\":" + std::to_string(st.st_mtime) + "}";
        }
        closedir(d);
    }
#endif
    json += "]";
    return json;
}

void CameraApi::HandleListPhotos(mg_connection* c) {
    std::string json = "{\"photos\":" + ListDirJson(AppPath(kPhotosDir), ".jpg") + "}";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json.c_str());
}

void CameraApi::HandleListVideos(mg_connection* c) {
    std::string json = "{\"videos\":" + ListDirJson(AppPath(kVideosDir), ".mp4") + "}";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json.c_str());
}

void CameraApi::HandleListLogs(mg_connection* c) {
    std::string json = "{\"logs\":" + ListDirJson(AppPath(kLogsDir), "") + "}";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json.c_str());
}

static bool IsSafeName(const std::string& name) {
    return !name.empty() &&
           name.find('/') == std::string::npos &&
           name.find('\\') == std::string::npos &&
           name.find("..") == std::string::npos;
}

static bool HasExt(const std::string& name, const std::string& ext) {
    return name.size() >= ext.size() &&
           name.compare(name.size() - ext.size(), ext.size(), ext) == 0;
}

static void DeleteFileByName(mg_connection* c,
                             const std::string& dir,
                             const std::string& name,
                             const std::string& ext) {
    if (!IsSafeName(name) || !HasExt(name, ext)) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                      "{\"ok\":false,\"error\":\"bad_file_name\"}\n");
        return;
    }

    std::string path = dir + "/" + name;
    if (std::remove(path.c_str()) == 0) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                      "{\"ok\":true}\n");
    } else {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n",
                      "{\"ok\":false,\"error\":\"file_not_found\"}\n");
    }
}

static int TruncateDirectory(const std::string& dir) {
    int cleared = 0;
#ifndef _WIN32
    DIR* d = opendir(dir.c_str());
    if (!d) return 0;
    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        std::string name = ent->d_name;
        if (name == "." || name == ".." || !IsSafeName(name)) continue;
        std::string path = dir + "/" + name;
        struct stat st;
        if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
        FILE* fp = std::fopen(path.c_str(), "w");
        if (fp) {
            std::fclose(fp);
            cleared++;
        }
    }
    closedir(d);
#endif
    return cleared;
}

void CameraApi::HandleDeletePhoto(mg_connection* c, const std::string& name) {
    DeleteFileByName(c, AppPath(kPhotosDir), name, ".jpg");
}

void CameraApi::HandleDeleteVideo(mg_connection* c, const std::string& name) {
    DeleteFileByName(c, AppPath(kVideosDir), name, ".mp4");
}

void CameraApi::HandleClearLogs(mg_connection* c) {
    int cleared = TruncateDirectory(AppPath(kLogsDir));
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                  "{\"ok\":true,\"cleared\":%d}\n", cleared);
}

static void SendFileByName(mg_connection* c,
                           const std::string& dir,
                           const std::string& name,
                           const char* content_type) {
    if (!IsSafeName(name)) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                      "{\"error\":\"bad_file_name\"}\n");
        return;
    }

    std::string path = dir + "/" + name;
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n",
                      "{\"error\":\"file_not_found\"}\n");
        return;
    }

    if (std::fseek(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                      "{\"error\":\"seek_failed\"}\n");
        return;
    }
    long file_size = std::ftell(fp);
    if (file_size < 0 || file_size > 50 * 1024 * 1024) {
        std::fclose(fp);
        mg_http_reply(c, 413, "Content-Type: application/json\r\n",
                      "{\"error\":\"file_too_large\"}\n");
        return;
    }
    std::rewind(fp);

    unsigned char* buf = static_cast<unsigned char*>(std::malloc((size_t)file_size));
    if (!buf && file_size > 0) {
        std::fclose(fp);
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                      "{\"error\":\"oom\"}\n");
        return;
    }

    size_t nread = file_size > 0 ? std::fread(buf, 1, (size_t)file_size, fp) : 0;
    std::fclose(fp);
    if ((long)nread != file_size) {
        std::free(buf);
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                      "{\"error\":\"read_failed\"}\n");
        return;
    }

    mg_printf(c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n\r\n",
        content_type, (unsigned long)nread);
    if (nread > 0) {
        mg_send(c, buf, nread);
    }
    c->is_draining = 1;
    std::free(buf);
}

void CameraApi::HandlePhotoFile(mg_connection* c, const std::string& name) {
    SendFileByName(c, AppPath(kPhotosDir), name, "image/jpeg");
}

void CameraApi::HandleVideoFile(mg_connection* c, const std::string& name) {
    SendFileByName(c, AppPath(kVideosDir), name, "video/mp4");
}

void CameraApi::HandleLogFile(mg_connection* c, const std::string& name) {
    SendFileByName(c, AppPath(kLogsDir), name, "text/plain; charset=utf-8");
}
