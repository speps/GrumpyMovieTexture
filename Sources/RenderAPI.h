#pragma once

#include "Unity/IUnityGraphics.h"
#include <stdint.h>
#include <assert.h>

struct IUnityInterfaces;

class RenderAPI
{
public:
    virtual ~RenderAPI() {}

    virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;
    virtual void UpdateTexture(void* handle, int width, int height, int rowPitch, uint8_t* data) { assert(false); }
};

RenderAPI* CreateRenderAPI(UnityGfxRenderer apiType);

