#include "VideoPlayer.h"

#include <assert.h>

#if WIN32
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)
#else
#define UNITY_INTERFACE_EXPORT
#endif

extern "C" UNITY_INTERFACE_EXPORT void* VPCreate(void* userData, VideoStatusCallback statusCallback)
{
    return new VideoPlayer(userData, statusCallback);
}

extern "C" UNITY_INTERFACE_EXPORT void VPDestroy(VideoPlayer* player)
{
    delete player;
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpenCallback(VideoPlayer* player, VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    return player->openCallback(dataCallback, createTextureCallback, uploadTextureCallback);
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpenFile(VideoPlayer* player, char* filePath, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    return player->openFile(filePath, createTextureCallback, uploadTextureCallback);
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

extern "C" UNITY_INTERFACE_EXPORT void VPGetAudioInfo(VideoPlayer* player, int& numSamples, int& channels, int& frequency)
{
    player->getAudioInfo(numSamples, channels, frequency);
}
