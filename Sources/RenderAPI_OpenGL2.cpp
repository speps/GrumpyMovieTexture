#include "RenderAPI.h"
#include "PlatformBase.h"

// OpenGL 2 (legacy, deprecated) implementation of RenderAPI.


#if SUPPORT_OPENGL_LEGACY

#include "GLEW/glew.h"


class RenderAPI_OpenGL2 : public RenderAPI
{
public:
    RenderAPI_OpenGL2() {}
    virtual ~RenderAPI_OpenGL2() {}

    virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) {}

    virtual void UpdateTexture(void* handle, int width, int height, uint8_t* data)
    {
        GLuint gltex = (GLuint)(size_t)(handle);
        glBindTexture(GL_TEXTURE_2D, gltex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_ALPHA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};


RenderAPI* CreateRenderAPI_OpenGL2()
{
    return new RenderAPI_OpenGL2();
}


#endif // #if SUPPORT_OPENGL_LEGACY
