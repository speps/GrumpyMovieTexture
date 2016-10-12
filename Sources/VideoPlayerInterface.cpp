#include "VideoPlayer.h"

#include <assert.h>

#if WIN32
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)
#else
#define UNITY_INTERFACE_EXPORT
#endif

#if ANDROID && __arm__
#include <coffeecatch.h>
#define SAFE_CALL(code) \
    COFFEE_TRY() { \
    code; \
    } COFFEE_CATCH() { \
    const char*const message = coffeecatch_get_message(); \
    fprintf(stderr, "**FATAL ERROR: %s\n", message); \
    } COFFEE_END();
#define SAFE_CALL_RET(code,catchcode) \
    COFFEE_TRY() { \
    code; \
    } COFFEE_CATCH() { \
    const char*const message = coffeecatch_get_message(); \
    fprintf(stderr, "**FATAL ERROR: %s\n", message); \
    } COFFEE_END(); \
    catchcode;
#else
    #define SAFE_CALL(code) do { code; } while(false);
    #define SAFE_CALL_RET(code,catchcode) do { code; } while(false);
#endif

extern "C" UNITY_INTERFACE_EXPORT void* VPCreate(void* userData, VideoStatusCallback statusCallback, VideoLogCallback logCallback, VideoGetValueCallback getValueCallback)
{
    SAFE_CALL_RET(return new VideoPlayer(userData, statusCallback, logCallback, getValueCallback),return nullptr);
}

extern "C" UNITY_INTERFACE_EXPORT void VPDestroy(VideoPlayer* player)
{
    SAFE_CALL(delete player);
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpenCallback(VideoPlayer* player, VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    SAFE_CALL_RET(return player && player->openCallback(dataCallback, createTextureCallback, uploadTextureCallback),return false);
}

extern "C" UNITY_INTERFACE_EXPORT bool VPOpenFile(VideoPlayer* player, char* filePath, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    SAFE_CALL_RET(return player && player->openFile(filePath, createTextureCallback, uploadTextureCallback),return false);
}

extern "C" UNITY_INTERFACE_EXPORT void VPSetDebugEnabled(VideoPlayer* player, bool enabled)
{
    assert(player);
    SAFE_CALL(player->setDebugEnabled(enabled));
}

extern "C" UNITY_INTERFACE_EXPORT void VPPlay(VideoPlayer* player)
{
    assert(player);
    SAFE_CALL(player->play());
}

extern "C" UNITY_INTERFACE_EXPORT bool VPIsPlaying(VideoPlayer* player)
{
    SAFE_CALL_RET(return player && player->isPlaying(),return false);
}

extern "C" UNITY_INTERFACE_EXPORT void VPStop(VideoPlayer* player)
{
    assert(player);
    SAFE_CALL(player->stop());
}

extern "C" UNITY_INTERFACE_EXPORT bool VPIsStopped(VideoPlayer* player)
{
    SAFE_CALL_RET(return player && player->isStopped(),return false);
}

extern "C" UNITY_INTERFACE_EXPORT void VPUpdate(VideoPlayer* player, float timeStep)
{
    assert(player);
    SAFE_CALL(player->update(timeStep));
}

extern "C" UNITY_INTERFACE_EXPORT void VPGetFrameSize(VideoPlayer* player, int& width, int& height, int& x, int& y)
{
    if (player)
    {
        SAFE_CALL(player->getFrameSize(width, height, x, y));
    }
}

extern "C" UNITY_INTERFACE_EXPORT void VPGetAudioInfo(VideoPlayer* player, int& channels, int& frequency)
{
    if (player)
    {
        SAFE_CALL(player->getAudioInfo(channels, frequency));
    }
}

extern "C" UNITY_INTERFACE_EXPORT void VPPCMRead(VideoPlayer* player, float* data, int numSamples)
{
    if (player)
    {
        SAFE_CALL(player->pcmRead(data, numSamples));
    }
}
