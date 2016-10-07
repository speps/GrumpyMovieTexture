#include "VideoPlayer.h"

#include <assert.h>

#if WIN32
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)
#else
#define UNITY_INTERFACE_EXPORT
#endif

extern "C" UNITY_INTERFACE_EXPORT void* VPCreate(void* userData, VideoStatusCallback statusCallback, VideoLogCallback logCallback, VideoGetValueCallback getValueCallback)
{
    return new VideoPlayer(userData, statusCallback, logCallback, getValueCallback);
}

extern "C" UNITY_INTERFACE_EXPORT void VPDestroy(VideoPlayer* player)
{
    delete player;
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpenCallback(VideoPlayer* player, VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    return player && player->openCallback(dataCallback, createTextureCallback, uploadTextureCallback);
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpenFile(VideoPlayer* player, char* filePath, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    return player && player->openFile(filePath, createTextureCallback, uploadTextureCallback);
}

extern "C" UNITY_INTERFACE_EXPORT void VPSetDebugEnabled(VideoPlayer* player, bool enabled)
{
    assert(player);
    player->setDebugEnabled(enabled);
}

extern "C" UNITY_INTERFACE_EXPORT void VPPlay(VideoPlayer* player)
{
    assert(player);
    player->play();
}

extern "C" UNITY_INTERFACE_EXPORT bool VPIsPlaying(VideoPlayer* player)
{
    return player && player->isPlaying();
}

extern "C" UNITY_INTERFACE_EXPORT void VPStop(VideoPlayer* player)
{
    assert(player);
    player->stop();
}

extern "C" UNITY_INTERFACE_EXPORT bool VPIsStopped(VideoPlayer* player)
{
    return player && player->isStopped();
}

extern "C" UNITY_INTERFACE_EXPORT void VPUpdate(VideoPlayer* player, float timeStep)
{
    assert(player);
    player->update(timeStep);
}

extern "C" UNITY_INTERFACE_EXPORT void VPGetFrameSize(VideoPlayer* player, int& width, int& height, int& x, int& y)
{
    if (player)
    {
        player->getFrameSize(width, height, x, y);
    }
}

extern "C" UNITY_INTERFACE_EXPORT void VPGetAudioInfo(VideoPlayer* player, int& channels, int& frequency)
{
    if (player)
    {
        player->getAudioInfo(channels, frequency);
    }
}

extern "C" UNITY_INTERFACE_EXPORT void VPPCMRead(VideoPlayer* player, float* data, int numSamples)
{
    if (player)
    {
        player->pcmRead(data, numSamples);
    }
}
