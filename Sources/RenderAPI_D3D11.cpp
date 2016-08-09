#include "RenderAPI.h"
#include "PlatformBase.h"

// Direct3D 11 implementation of RenderAPI.

#if SUPPORT_D3D11

#include <assert.h>
#include <d3d11.h>
#include "Unity/IUnityGraphicsD3D11.h"

class RenderAPI_D3D11 : public RenderAPI
{
public:
    RenderAPI_D3D11() : m_Device(nullptr) {}
    virtual ~RenderAPI_D3D11() { }

    virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
    {
        switch (type)
        {
        case kUnityGfxDeviceEventInitialize:
        {
            IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
            m_Device = d3d->GetDevice();
            break;
        }
        }
    }

    virtual void UpdateTexture(void* handle, int width, int height, int rowPitch, uint8_t* data)
    {
        ID3D11Texture2D* d3dtex = (ID3D11Texture2D*)handle;
        if (d3dtex == nullptr)
        {
            return;
        }

        ID3D11DeviceContext* ctx = nullptr;
        m_Device->GetImmediateContext(&ctx);
        ctx->UpdateSubresource(d3dtex, 0, nullptr, data, rowPitch, 0);
        ctx->Release();
    }

private:
    ID3D11Device* m_Device;
};


RenderAPI* CreateRenderAPI_D3D11()
{
    return new RenderAPI_D3D11();
}


#endif // #if SUPPORT_D3D11
