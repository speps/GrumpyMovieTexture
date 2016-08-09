#include "RenderAPI.h"
#include "PlatformBase.h"

// OpenGL Core profile (desktop) or OpenGL ES (mobile) implementation of RenderAPI.
// Supports several flavors: Core, ES2, ES3


#if SUPPORT_OPENGL_UNIFIED


#include <assert.h>
#if UNITY_IPHONE
#	include <OpenGLES/ES2/gl.h>
#elif UNITY_ANDROID
#	include <GLES2/gl2.h>
#else
#	include "GLEW/glew.h"
#endif


class RenderAPI_OpenGLCoreES : public RenderAPI
{
public:
    RenderAPI_OpenGLCoreES(UnityGfxRenderer apiType) {}
    virtual ~RenderAPI_OpenGLCoreES() {}

    virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) {}

    virtual void UpdateTexture(void* handle, int width, int height, uint8_t* data)
    {
        GLuint gltex = (GLuint)(size_t)(handle);
        glBindTexture(GL_TEXTURE_2D, gltex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_ALPHA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};


RenderAPI* CreateRenderAPI_OpenGLCoreES(UnityGfxRenderer apiType)
{
    return new RenderAPI_OpenGLCoreES(apiType);
}


#endif // #if SUPPORT_OPENGL_UNIFIED
