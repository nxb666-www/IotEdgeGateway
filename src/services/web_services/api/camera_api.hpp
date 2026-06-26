#ifndef CAMERA_API_HPP
#define CAMERA_API_HPP

#include "mongoose.h"
#include <string>

class CameraApi {
public:
    static void SetVideoDevice(const std::string& device);
    static void HandleStartStream(mg_connection* c);
    static void HandleStopStream(mg_connection* c);
    static void HandleSnapshot(mg_connection* c);
    static void HandleStartRecord(mg_connection* c);
    static void HandleStopRecord(mg_connection* c);
    static void HandleLiveJpg(mg_connection* c);
    static void HandleMjpegStream(mg_connection* c);
    static bool IsMjpegConnection(mg_connection* c);
    static void HandleMjpegPoll(mg_connection* c);
    static void HandleProbe(mg_connection* c);
    static void HandleListPhotos(mg_connection* c);
    static void HandleListVideos(mg_connection* c);
    static void HandleListLogs(mg_connection* c);
    static void HandleDeletePhoto(mg_connection* c, const std::string& name);
    static void HandleDeleteVideo(mg_connection* c, const std::string& name);
    static void HandleClearLogs(mg_connection* c);
    static void HandlePhotoFile(mg_connection* c, const std::string& name);
    static void HandleVideoFile(mg_connection* c, const std::string& name);
    static void HandleLogFile(mg_connection* c, const std::string& name);

private:
    static bool CheckStreamProcess();
};

#endif
