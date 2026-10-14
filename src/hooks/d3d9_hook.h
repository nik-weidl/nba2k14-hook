#pragma once
#include <d3d9.h>

namespace Hooks {
    class D3D9Hook {
    public:
        static bool Init();
        static void Shutdown();

    private:
        static HRESULT WINAPI Hooked_EndScene(IDirect3DDevice9* pDevice);
        static HRESULT WINAPI Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);

        using EndScene_t = HRESULT(WINAPI*)(IDirect3DDevice9*);
        using Reset_t = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

        static EndScene_t s_OriginalEndScene;
        static Reset_t s_OriginalReset;
        static void* s_TargetEndScene;
        static void* s_TargetReset;
        static bool s_Initialized;
    };
}
