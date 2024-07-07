#include "bgfx_helper.h"
#include <bgfx/platform.h>
void setup_bgfx_platform_data(bgfx::PlatformData &pd,
                                     const SDL_SysWMinfo &wmi) {

#if BX_PLATFORM_EMSCRIPTEN
  pd.nwh = (void *)"#canvas";
#elif BX_PLATFORM_IOS
  pd.ndt = nullptr;
  pd.nwh = GetIOSWindowHandle(wmi.info.uikit.window);
  pd.context = nullptr;
  pd.backBuffer = nullptr;
  pd.backBufferDS = nullptr;
#else
    switch (wmi.subsystem) {
            case SDL_SYSWM_UNKNOWN: abort();

    #if defined(SDL_VIDEO_DRIVER_X11)
            case SDL_SYSWM_X11:
                pd.ndt = wmi.info.x11.display;
                pd.nwh = (void *)(uintptr_t)wmi.info.x11.window;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_WAYLAND)
            case SDL_SYSWM_WAYLAND:
                pd.ndt = wmi.info.wl.display;
                pd.nwh = wmi.info.wl.surface;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_MIR)
            case SDL_SYSWM_MIR:
                pd.ndt = wmi.info.mir.connection;
                pd.nwh = wmi.info.mir.surface;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_COCOA)
            case SDL_SYSWM_COCOA:
                pd.ndt = NULL;
                pd.nwh = wmi.info.cocoa.window;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_UIKIT)
            case SDL_SYSWM_UIKIT:
                pd.ndt = NULL;
                pd.nwh = wmi.info.uikit.window;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_WINDOWS)
            case SDL_SYSWM_WINDOWS:
                pd.ndt = NULL;
                pd.nwh = wmi.info.win.window;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_WINRT)
            case SDL_SYSWM_WINRT:
                pd.ndt = NULL;
                pd.nwh = wmi.info.winrt.window;
                break;
    #endif

    #if defined(SDL_VIDEO_DRIVER_VIVANTE)
            case SDL_SYSWM_VIVANTE:
                pd.ndt = wmi.info.vivante.display;
                pd.nwh = wmi.info.vivante.window;
                break;
    #endif

            default: std::abort();
        }
#endif
    pd.context = NULL;
    pd.backBuffer = NULL;
    pd.backBufferDS = NULL;
}