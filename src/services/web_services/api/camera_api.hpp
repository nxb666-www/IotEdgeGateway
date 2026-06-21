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
    static void HandleProbe(mg_connection* c);

private:
    static bool CheckStreamProcess();
};

#endif
