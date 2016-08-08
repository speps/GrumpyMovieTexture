#pragma once

#include "Unity/IUnityGraphics.h"
#include <stdint.h>
#include <assert.h>

struct IUnityInterfaces;

// Super-simple "graphics abstraction". This is nothing like how a proper platform abstraction layer would look like.
// There are implementations of this base class for D3D9, D3D11, OpenGL etc.; see individual RenderAPI_* files.
class RenderAPI
{
public:
    virtual ~RenderAPI() {}

    // Process general event like initialization, shutdown, device loss/reset etc.
    virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

    virtual void UpdateTexture(void* handle, int width, int height, int rowPitch, uint8_t* data) { assert(false); }

    // Begin modifying texture data. You need to pass texture width/height too, since some graphics APIs
    // (e.g. OpenGL ES) do not have a good way to query that from the texture itself
    //
    // Returns pointer into the data buffer to write into (or NULL on failure), and pitch in bytes of a single texture row.
    virtual void* BeginModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int* outRowPitch) = 0;

    // End modifying texture data.
    virtual void EndModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, void* dataPtr) = 0;
};

// Create a graphics API implementation instance for the given API type.
RenderAPI* CreateRenderAPI(UnityGfxRenderer apiType);

