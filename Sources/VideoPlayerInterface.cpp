#include "VideoPlayer.h"

#include <assert.h>

#define UNITY_INTERFACE_EXPORT __declspec(dllexport)

extern "C" UNITY_INTERFACE_EXPORT void* VPCreate(void* userData)
{
    return new VideoPlayer(userData);
}

extern "C" UNITY_INTERFACE_EXPORT void VPDestroy(VideoPlayer* player)
{
    delete player;
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpen(VideoPlayer* player, VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    return player->open(dataCallback, createTextureCallback, uploadTextureCallback);
}

extern "C" UNITY_INTERFACE_EXPORT void VPPlay(VideoPlayer* player)
{
    player->play();
}

extern "C" UNITY_INTERFACE_EXPORT bool VPIsPlaying(VideoPlayer* player)
{
    return player->isPlaying();
}

extern "C" UNITY_INTERFACE_EXPORT void VPStop(VideoPlayer* player)
{
    player->stop();
}

extern "C" UNITY_INTERFACE_EXPORT bool VPIsStopped(VideoPlayer* player)
{
    return player->isStopped();
}

extern "C" UNITY_INTERFACE_EXPORT void VPUpdate(VideoPlayer* player, float timeStep)
{
    player->update(timeStep);
}

extern "C" UNITY_INTERFACE_EXPORT void VPGetFrameSize(VideoPlayer* player, int& width, int& height, int& x, int& y)
{
    player->getFrameSize(width, height, x, y);
}
