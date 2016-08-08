#include "PlatformBase.h"
#include "RenderAPI.h"
#include "VideoPlayer.h"

#include <assert.h>
#include <math.h>

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_UnityInterfaces = unityInterfaces;
    s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginUnload()
{
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

RenderAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    if (eventType == kUnityGfxDeviceEventInitialize)
    {
        assert(s_CurrentAPI == NULL);
        s_DeviceType = s_Graphics->GetRenderer();
        s_CurrentAPI = CreateRenderAPI(s_DeviceType);
    }

    if (s_CurrentAPI)
    {
        s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
    }

    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        delete s_CurrentAPI;
        s_CurrentAPI = NULL;
        s_DeviceType = kUnityGfxRendererNull;
    }
}

extern "C" UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API VPCreate()
{
    return new VideoPlayer();
}

extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API VPDestroy(VideoPlayer* player)
{
    delete player;
}

extern "C" UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API VPOpen(VideoPlayer* player, VideoDataCallback dataCallback, VideoCreateTextureCallback textureCallback)
{
    return player->open(dataCallback, textureCallback);
}

extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API VPPlay(VideoPlayer* player)
{
    player->play();
}

extern "C" UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API VPIsPlaying(VideoPlayer* player)
{
    return player->isPlaying();
}

extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API VPStop(VideoPlayer* player)
{
    player->stop();
}

extern "C" UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API VPIsStopped(VideoPlayer* player)
{
    return player->isStopped();
}

extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API VPUpdate(VideoPlayer* player, float timeStep)
{
    player->update(timeStep);
}
