// sdl_main.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#include <sstream>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_syswm.h>
#undef main

#include <glm/gtc/type_ptr.hpp>

#ifdef USE_ANTTWEAKBAR
#  include <AntTweakBar.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sstream>

#include "RiftAppSkeleton.h"
#include "RenderingMode.h"
#include "Timer.h"
#include "FPSTimer.h"
#include "Logger.h"

RiftAppSkeleton g_app;
RenderingMode g_renderMode;
Timer g_timer;
double g_lastFrameTime = 0.0;
FPSTimer g_fps;
Timer g_logDumpTimer;

int m_keyStates[4096];

// mouse motion internal state
int oldx, oldy, newx, newy;
int which_button = -1;
int modifier_mode = 0;

//ShaderWithVariables g_auxPresent;
SDL_Window* g_pHMDWindow = NULL;
SDL_Window* g_pAuxWindow = NULL;
SDL_GLContext g_HMDglContext = NULL;
Uint32 g_HMDWindowID = 0;
Uint32 g_AuxWindowID = 0;
int g_auxWindow_w = 600;
int g_auxWindow_h = 600;

SDL_Joystick* g_pJoy = NULL;

float g_fpsSmoothingFactor = 0.02f;
float g_fpsDeltaThreshold = 5.0f;
bool g_dynamicallyScaleFBO = false;
int g_targetFPS = 100;

#ifdef USE_ANTTWEAKBAR
TwBar* g_pTweakbar = NULL;
#endif

SDL_Window* initializeAuxiliaryWindow();
void destroyAuxiliaryWindow(SDL_Window* pAuxWindow);

// Set VSync is framework-dependent and has to come before the include
static void SetVsync(int state)
{
    LOG_INFO("SetVsync(%d)", state);

    int ret = 0;
    if (g_pAuxWindow != NULL)
    {
        SDL_GL_MakeCurrent(g_pAuxWindow, g_HMDglContext);
        ret = SDL_GL_SetSwapInterval(state);
        if (ret != 0)
        {
            LOG_INFO("Error occurred in SDL_GL_SetSwapInterval: %s", SDL_GetError());
        }
    }
    SDL_GL_MakeCurrent(g_pHMDWindow, g_HMDglContext);
    ret = SDL_GL_SetSwapInterval(state);
    if (ret != 0)
    {
        LOG_INFO("Error occurred in SDL_GL_SetSwapInterval: %s", SDL_GetError());
    }
}

#include "main_include.cpp"

void timestep()
{
    const double absT = g_timer.seconds();
    const double dt = absT - g_lastFrameTime;
    g_lastFrameTime = absT;
    g_app.timestep(absT, dt);
}

void keyboard(const SDL_Event& event, int key, int codes, int action, int mods)
{
    (void)codes;
    (void)mods;

    const int KEYUP = 0;
    const int KEYDOWN = 1;
    const int keyMasked = key & 0xfff;
    if ((key > -1) && (keyMasked <= 4096))
    {
        int keystate = KEYUP;
        if (action == SDL_KEYDOWN)
            keystate = 1;
        m_keyStates[keyMasked] = keystate;
        //printf("key ac  %d %d\n", key, action);
    }

    const float f = 0.9f;
    const float ff = 0.99f;

    if (action == SDL_KEYDOWN)
    {
        switch (key)
        {
        default:
            g_app.DismissHealthAndSafetyWarning();
            break;

        case SDLK_F1:
            if (SDL_GetModState() & KMOD_LCTRL)
                g_renderMode.toggleRenderingTypeReverse();
            else
                g_renderMode.toggleRenderingType();
            break;

        case SDLK_F2:
            g_renderMode.toggleRenderingTypeMono();
            break;

        case SDLK_F3:
            g_renderMode.toggleRenderingTypeHMD();
            break;

        case SDLK_F4:
            g_renderMode.toggleRenderingTypeDistortion();
            break;

        case SDLK_F5: g_dynamicallyScaleFBO = false; g_app.SetFBOScale(f * g_app.GetFBOScale()); break;
        case SDLK_F6: g_dynamicallyScaleFBO = false; g_app.SetFBOScale(ff * g_app.GetFBOScale()); break;
        case SDLK_F7: g_dynamicallyScaleFBO = false; g_app.SetFBOScale((1.f/ff) * g_app.GetFBOScale()); break;
        case SDLK_F8: g_dynamicallyScaleFBO = false; g_app.SetFBOScale((1.f/f) * g_app.GetFBOScale()); break;

        case SDLK_F9: SetVsync(0); break;
        case SDLK_F10: SetVsync(1); break;
        case SDLK_F11: SetVsync(-1); break;

        case SDLK_DELETE: g_dynamicallyScaleFBO = !g_dynamicallyScaleFBO; break;

        case '`':
            if (g_pAuxWindow == NULL)
            {
                LOG_INFO("Creating auxiliary window.");
                g_pAuxWindow = initializeAuxiliaryWindow();
            }
            else
            {
                LOG_INFO("Destroying auxiliary window.");
                destroyAuxiliaryWindow(g_pAuxWindow);
                g_pAuxWindow = NULL;
            }
            break;

        case SDLK_SPACE:
            g_app.RecenterPose();
            break;

        case 'r':
            g_app.ResetAllTransformations();
            break;

        case SDLK_ESCAPE:
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                if (event.key.windowID == g_HMDWindowID)
                {
                    SDL_GL_DeleteContext(g_HMDglContext);
                    SDL_DestroyWindow(g_pHMDWindow);

                    g_app.exitVR();
                    if (g_pJoy != NULL)
                        SDL_JoystickClose(g_pJoy);
                    SDL_Quit();
                    exit(0);
                }
                else
                {
                    destroyAuxiliaryWindow(g_pAuxWindow);
                    g_pAuxWindow = NULL;
                }
            }
            break;
        }
    }

    //g_app.keyboard(key, action, 0,0);

    const glm::vec3 forward(0.f, 0.f, -1.f);
    const glm::vec3 up(0.f, 1.f, 0.f);
    const glm::vec3 right(1.f, 0.f, 0.f);
    // Handle keyboard movement(WASD & QE keys)
    glm::vec3 keyboardMove(0.0f, 0.0f, 0.0f);
    if (m_keyStates['w'] != KEYUP) { keyboardMove += forward; }
    if (m_keyStates['s'] != KEYUP) { keyboardMove -= forward; }
    if (m_keyStates['a'] != KEYUP) { keyboardMove -= right; }
    if (m_keyStates['d'] != KEYUP) { keyboardMove += right; }
    if (m_keyStates['q'] != KEYUP) { keyboardMove -= up; }
    if (m_keyStates['e'] != KEYUP) { keyboardMove += up; }
    if (m_keyStates[SDLK_UP &0xfff] != KEYUP) { keyboardMove += forward; }
    if (m_keyStates[SDLK_DOWN &0xfff] != KEYUP) { keyboardMove -= forward; }
    if (m_keyStates[SDLK_LEFT &0xfff] != KEYUP) { keyboardMove -= right; }
    if (m_keyStates[SDLK_RIGHT &0xfff] != KEYUP) { keyboardMove += right; }

    float mag = 1.0f;
    if (SDL_GetModState() & KMOD_LSHIFT)
        mag *= 0.1f;
    if (SDL_GetModState() & KMOD_LCTRL)
        mag *= 10.0f;

    // Yaw keys
    g_app.m_keyboardYaw = 0.0f;
    const float dyaw = 0.5f * mag; // radians at 60Hz timestep
    if (m_keyStates['1'] != KEYUP)
    {
        g_app.m_keyboardYaw = -dyaw;
    }
    if (m_keyStates['3'] != KEYUP)
    {
        g_app.m_keyboardYaw = dyaw;
    }

    g_app.m_keyboardMove = mag * keyboardMove;
}

// Joystick state is polled here and stored within SDL.
void joystick()
{
    if (g_pJoy == NULL)
        return;

    if (SDL_JoystickGetAttached(g_pJoy) == SDL_FALSE)
        return;

    const int MAX_BUTTONS = 32;
    Uint8 buttonStates[MAX_BUTTONS];
    const int numButtons = SDL_JoystickNumButtons(g_pJoy);
    for (int i=0; i<std::min(numButtons, MAX_BUTTONS); ++i)
    {
        buttonStates[i] = SDL_JoystickGetButton(g_pJoy, i);
    }

    // Map joystick buttons to move directions
    const glm::vec3 moveDirsGravisGamepadPro[8] = {
        glm::vec3(-1.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  1.f),
        glm::vec3( 1.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f, -1.f),
        glm::vec3( 0.f,  1.f,  0.f),
        glm::vec3( 0.f,  1.f,  0.f),
        glm::vec3( 0.f, -1.f,  0.f),
        glm::vec3( 0.f, -1.f,  0.f),
    };

    ///@note Application must have focus to receive gamepad events!
    // Xbox controller layout:
    // numAxes 5, numButtons 14
    // 0 DPad up
    // 1 Dpad Down
    // 2 DPad left
    // 3 DPad right
    // 4 Start
    // 5 Back
    // 6 Left analog stick click
    // 7 Right analog stick click
    // 8 L bumper
    // 9 R bumper
    // 10 A (down position)
    // 11 B (right position)
    // 12 X (left position)
    // 13 Y (up position)
    // Axis 0 1 Left stick x y -32768, 32767
    // Axis 2 3 right stick x y -32768, 32767
    // Axis 4 5 triggers, -32768(released), 32767
    const glm::vec3 moveDirsXboxController[15] = {
        glm::vec3( 0.f,  1.f,  0.f),
        glm::vec3( 0.f, -1.f,  0.f),
        glm::vec3(-1.f,  0.f,  0.f),
        glm::vec3( 1.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  1.f), // 10
        glm::vec3( 1.f,  0.f,  0.f),
        glm::vec3(-1.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f, -1.f),
        glm::vec3( 0.f, -1.f,  0.f),
    };

    ///@todo Different mappings for different controllers.
    const glm::vec3* moveDirs = moveDirsGravisGamepadPro;
    const std::string joyName(SDL_JoystickName(g_pJoy));
    if (!joyName.compare("XInput Controller #1"))
    {
        moveDirs = moveDirsXboxController;
    }

    glm::vec3 joystickMove(0.0f, 0.0f, 0.0f);
    for (int i=0; i<std::min(32,numButtons); ++i)
    {
        if (buttonStates[i] != 0)
        {
            joystickMove += moveDirs[i];
        }
    }

    const int numAxes = SDL_JoystickNumAxes(g_pJoy);
    float mag = 1.f;
    if (numAxes > 5)
    {
        const Sint16 axisVal = SDL_JoystickGetAxis(g_pJoy, 5);
        const int axisVal32 = static_cast<Sint32>(axisVal);
        const int posval = axisVal32 + -SHRT_MIN;
        const float normVal = static_cast<float>(posval) / ( static_cast<float>(SHRT_MAX) - static_cast<float>(SHRT_MIN) );
        mag = pow(10.f, normVal);
    }
    g_app.m_joystickMove = mag * joystickMove;

    Sint16 x_move = SDL_JoystickGetAxis(g_pJoy, 0);
    const int deadZone = 4096;
    if (abs(x_move) < deadZone)
        x_move = 0;
    g_app.m_joystickYaw = 0.00005f * static_cast<float>(x_move);
}

void mouseDown(int button, int state, int x, int y)
{
    which_button = button;
    oldx = newx = x;
    oldy = newy = y;
    if (state == SDL_RELEASED)
    {
        which_button = -1;
    }
    g_app.OnMouseButton(button, state);
}

void mouseMove(int x, int y)
{
    oldx = newx;
    oldy = newy;
    newx = x;
    newy = y;
    const int mmx = x-oldx;
    const int mmy = y-oldy;

    g_app.m_mouseDeltaYaw = 0.0f;
    g_app.m_mouseMove = glm::vec3(0.0f);

    if (which_button == SDL_BUTTON_LEFT)
    {
        const float spinMagnitude = 0.05f;
        g_app.m_mouseDeltaYaw += static_cast<float>(mmx) * spinMagnitude;
    }
    else if (which_button == SDL_BUTTON_RIGHT)
    {
        const float moveMagnitude = 0.5f;
        g_app.m_mouseMove.x += static_cast<float>(mmx) * moveMagnitude;
        g_app.m_mouseMove.z += static_cast<float>(mmy) * moveMagnitude;
    }
    else if (which_button == SDL_BUTTON_MIDDLE)
    {
        const float moveMagnitude = 0.5f;
        g_app.m_mouseMove.x += static_cast<float>(mmx) * moveMagnitude;
        g_app.m_mouseMove.y -= static_cast<float>(mmy) * moveMagnitude;
    }
    else
    {
        // Passive motion, no mouse button pressed
        g_app.OnMouseMove(static_cast<int>(x), static_cast<int>(y));
    }
}

void mouseWheel(int x, int y)
{
    const int delta = static_cast<int>(y);
    const float curscale = g_app.GetFBOScale();
    const float incr = 1.05f;
    g_app.SetFBOScale(curscale * pow(incr, static_cast<float>(delta)));
    if (fabs(static_cast<float>(x)) > 0.f)
    {
        g_app.OnMouseWheel(x,y);
    }
}

void mouseDown_Aux(int button, int state, int x, int y)
{
#ifdef USE_ANTTWEAKBAR
    TwMouseAction action = TW_MOUSE_RELEASED;
    if (state == SDL_PRESSED)
    {
        action = TW_MOUSE_PRESSED;
    }
    TwMouseButtonID buttonID = TW_MOUSE_LEFT;
    if (button == SDL_BUTTON_LEFT)
        buttonID = TW_MOUSE_LEFT;
    else if (button == SDL_BUTTON_MIDDLE)
        buttonID = TW_MOUSE_MIDDLE;
    else if (button == SDL_BUTTON_RIGHT)
        buttonID = TW_MOUSE_RIGHT;
    const int ant = TwMouseButton(action, buttonID);
    if (ant != 0)
        return;
#endif
    mouseDown(button, state, x, y);
}

void mouseMove_Aux(int x, int y)
{
#ifdef USE_ANTTWEAKBAR
    const int ant = TwMouseMotion(x, y);
    if (ant != 0)
        return;
#endif
    mouseMove(x, y);
}

void display()
{
    switch(g_renderMode.outputType)
    {
    case RenderingMode::Mono_Raw:
        g_app.display_raw();
        SDL_GL_SwapWindow(g_pHMDWindow);
        break;

    case RenderingMode::Mono_Buffered:
        g_app.display_buffered();
        SDL_GL_SwapWindow(g_pHMDWindow);
        break;

    case RenderingMode::SideBySide_Undistorted:
        g_app.display_stereo_undistorted();
        SDL_GL_SwapWindow(g_pHMDWindow);
        break;

    case RenderingMode::OVR_SDK:
        g_app.display_sdk();
        // OVR will do its own swap
        break;

    case RenderingMode::OVR_Client:
        g_app.display_client();
        SDL_GL_SwapWindow(g_pHMDWindow);
        break;

    default:
        LOG_ERROR("Unknown display type: %d", g_renderMode.outputType);
        break;
    }
}

void PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            keyboard(event, event.key.keysym.sym, 0, event.key.type, 0);
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (event.window.windowID == g_AuxWindowID)
                mouseDown_Aux(event.button.button, event.button.state, event.button.x, event.button.y);
            else
                mouseDown(event.button.button, event.button.state, event.button.x, event.button.y);
            break;

        case SDL_MOUSEMOTION:
            if (event.window.windowID == g_AuxWindowID)
                mouseMove_Aux(event.motion.x, event.motion.y);
            else
                mouseMove(event.motion.x, event.motion.y);
            break;

        case SDL_MOUSEWHEEL:
            if (event.window.windowID == g_AuxWindowID)
                mouseWheel(event.wheel.x, event.wheel.y);
            else
                mouseWheel(event.wheel.x, event.wheel.y);
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                if (event.window.windowID == g_AuxWindowID)
                {
                    destroyAuxiliaryWindow(g_pAuxWindow);
                    g_pAuxWindow = NULL;
                }
            }
            else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                if (event.window.windowID != g_HMDWindowID)
                {
                    const int w = event.window.data1;
                    const int h = event.window.data2;
#ifdef USE_ANTTWEAKBAR
                    TwWindowSize(w,h);
#endif
                }
            }
            break;

        case SDL_QUIT:
            exit(0);
            break;

        default:
            break;
        }
    }

    SDL_JoystickUpdate();
    joystick();
}

SDL_Window* initializeAuxiliaryWindow()
{
    SDL_Window* pAuxWindow = SDL_CreateWindow(
        "GL Skeleton - SDL2",
        10, 10, //SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        g_auxWindow_w, g_auxWindow_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    g_AuxWindowID = SDL_GetWindowID(pAuxWindow);
    return pAuxWindow;
}

void destroyAuxiliaryWindow(SDL_Window* pAuxWindow)
{
    SDL_DestroyWindow(pAuxWindow);
    g_AuxWindowID = -1;
}

int main(int argc, char** argv)
{
    bool useOpenGLCoreContext = false;

    RenderingMode renderMode;
    renderMode.outputType = RenderingMode::OVR_SDK;

#ifdef USE_CORE_CONTEXT
    useOpenGLCoreContext = true;
#endif

    // Command line options
    for (int i=0; i<argc; ++i)
    {
        const std::string a = argv[i];
        LOG_INFO("argv[%d]: %s", i, a.c_str());
        if (!a.compare("-sdk"))
        {
            g_renderMode.outputType = RenderingMode::OVR_SDK;
            renderMode.outputType = RenderingMode::OVR_SDK;
        }
        else if (!a.compare("-client"))
        {
            g_renderMode.outputType = RenderingMode::OVR_Client;
            renderMode.outputType = RenderingMode::OVR_Client;
        }
        else if (!a.compare("-core"))
        {
            useOpenGLCoreContext = true;
        }
        else if (!a.compare("-compat"))
        {
            useOpenGLCoreContext = false;
        }
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        return false;
    }

#ifndef _LINUX
    if (useOpenGLCoreContext)
    {
        LOG_INFO("Using OpenGL core context.");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    }
    else
    {
        LOG_INFO("Using OpenGL compatibility context.");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    }
#endif

#ifdef USE_OCULUSSDK
    g_app.initHMD();
    const ovrSizei sz = g_app.getHmdResolution();
    const ovrVector2i pos = g_app.getHmdWindowPos();

    if (g_app.UsingDebugHmd() == true)
    {
        // Create a normal, decorated application window
        LOG_INFO("Using Debug HMD mode.");
        g_pHMDWindow = SDL_CreateWindow(
            "GL Skeleton - SDL2",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            sz.w, sz.h,
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    }
    else
    {
        // HMD active - position undecorated window to fill HMD viewport
        if (g_app.UsingDirectMode())
        {
            LOG_INFO("Using Direct to Rift mode.");

            g_pHMDWindow = SDL_CreateWindow(
                "GL Skeleton - SDL2",
                pos.x, pos.y,
                sz.w, sz.h,
                SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
#if defined(_WIN32)
            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            const bool ret = SDL_GetWindowWMInfo(g_pHMDWindow, &info);
            if (ret)
            {
                g_app.AttachToWindow(info.info.win.window);
            }
#endif
        }
        else
        {
            LOG_INFO("Using Extended desktop mode.");

            g_pHMDWindow = SDL_CreateWindow(
                "GL Skeleton - SDL2",
                pos.x, pos.y,
                sz.w, sz.h,
                SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
        }
        g_renderMode = renderMode;
    }
#else
    g_pHMDWindow = SDL_CreateWindow(
        "GL Skeleton - SDL2",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        sz.w, sz.h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
#endif //USE_OCULUSSDK


    if (g_pHMDWindow == NULL)
    {
        LOG_ERROR("SDL_CreateWindow failed with error: %s", SDL_GetError());
        SDL_Quit();
    }

    g_HMDWindowID = SDL_GetWindowID(g_pHMDWindow);

#ifdef USE_OCULUSSDK
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(g_pHMDWindow, &info);
  #if defined(OVR_OS_WIN32)
    g_app.setWindow(info.info.win.window);
  #elif defined(OVR_OS_LINUX)
    g_app.setWindow(info.info.x11.display);
  #endif
#endif //USE_OCULUSSDK

    // thank you http://www.brandonfoltz.com/2013/12/example-using-opengl-3-0-with-sdl2-and-glew/
    g_HMDglContext = SDL_GL_CreateContext(g_pHMDWindow);
    if (g_HMDglContext == NULL)
    {
        LOG_ERROR("There was an error creating the OpenGL context!");
        return 0;
    }

    const unsigned char *version = glGetString(GL_VERSION);
    if (version == NULL) 
    {
        LOG_ERROR("There was an error creating the OpenGL context!");
        return 1;
    }

    // Joysticks/gamepads
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    const int numjoys = SDL_NumJoysticks();
    for (int i=0; i<numjoys; ++i)
    {
        SDL_Joystick* pJoy = SDL_JoystickOpen(i);
        if (pJoy == NULL)
            continue;

        LOG_INFO("SDL2 opened Joystick #%d: %s w/ %d axes, %d buttons, %d balls",
            i,
            SDL_JoystickName(pJoy),
            SDL_JoystickNumAxes(pJoy),
            SDL_JoystickNumButtons(pJoy),
            SDL_JoystickNumBalls(pJoy));

        SDL_JoystickClose(g_pJoy);
    }

    if (numjoys > 0)
    {
        g_pJoy = SDL_JoystickOpen(0);
    }

    // Log system monitor information
    // Get current display mode of all displays.
    for (int i=0; i<SDL_GetNumVideoDisplays(); ++i)
    {
        SDL_DisplayMode current;
        const int should_be_zero = SDL_GetCurrentDisplayMode(i, &current);

        if(should_be_zero != 0)
        {
            LOG_ERROR("Could not get display mode for video display #%d: %s", i, SDL_GetError());
        }
        else
        {
            LOG_INFO("Monitor #%d: %dx%d @ %dHz",
                i, current.w, current.h, current.refresh_rate);
        }
    }

    LOG_INFO("OpenGL: %s", version);
    LOG_INFO("Vendor: %s", (char*)glGetString(GL_VENDOR));
    LOG_INFO("Renderer: %s", (char*)glGetString(GL_RENDERER));

    SDL_GL_MakeCurrent(g_pHMDWindow, g_HMDglContext);

    // Don't forget to initialize Glew, turn glewExperimental on to
    // avoid problems fetching function pointers...
    glewExperimental = GL_TRUE;
    const GLenum l_Result = glewInit();
    if (l_Result != GLEW_OK)
    {
        LOG_INFO("glewInit() error.");
        exit(EXIT_FAILURE);
    }

#ifdef USE_ANTTWEAKBAR
    LOG_INFO("Using AntTweakbar.");
    TwInit(useOpenGLCoreContext ? TW_OPENGL_CORE : TW_OPENGL, NULL);
    InitializeBar();
#endif

    LOG_INFO("Calling initGL...");
    g_app.initGL();
    LOG_INFO("Calling initVR...");
    g_app.initVR();
    LOG_INFO("initVR complete.");

    memset(m_keyStates, 0, 4096*sizeof(int));


    int quit = 0;
    while (quit == 0)
    {
        g_app.CheckForTapToDismissHealthAndSafetyWarning();
        PollEvents();
        timestep();
        g_fps.OnFrame();
        if (g_dynamicallyScaleFBO)
        {
            DynamicallyScaleFBO();
        }

        display();

#ifndef _LINUX
        // Indicate FPS in window title
        // This is absolute death for performance in Ubuntu Linux 12.04
        {
            std::ostringstream oss;
            oss << "SDL Oculus Rift Test - "
                << static_cast<int>(g_fps.GetFPS())
                << " fps";
            SDL_SetWindowTitle(g_pHMDWindow, oss.str().c_str());
            if (g_pAuxWindow != NULL)
                SDL_SetWindowTitle(g_pAuxWindow, oss.str().c_str());
        }
#endif

        const float dumpInterval = 1.f;
        if (g_logDumpTimer.seconds() > dumpInterval)
        {
            LOG_INFO("Frame rate: %d fps", static_cast<int>(g_fps.GetFPS()));
            g_logDumpTimer.reset();
        }

        // Optionally display to auxiliary mono view
        if (g_pAuxWindow != NULL)
        {
            // SDL allows us to share contexts, so we can just call display on g_app
            // and use all of the VAOs resident in the HMD window's context.
            SDL_GL_MakeCurrent(g_pAuxWindow, g_HMDglContext);

            // Get window size from SDL - is this worth caching?
            int w, h;
            SDL_GetWindowSize(g_pAuxWindow, &w, &h);

            glPushAttrib(GL_VIEWPORT_BIT);
            glViewport(0, 0, w, h);
            g_app.display_buffered(false);
            glPopAttrib(); // GL_VIEWPORT_BIT - if this is not misused!

#ifdef USE_ANTTWEAKBAR
            TwRefreshBar(g_pTweakbar);
            TwDraw(); ///@todo Should this go first? Will it write to a depth buffer?
#endif

            SDL_GL_SwapWindow(g_pAuxWindow);

            // Set context to Rift window when done
            SDL_GL_MakeCurrent(g_pHMDWindow, g_HMDglContext);
        }
    }

    SDL_GL_DeleteContext(g_HMDglContext);
    SDL_DestroyWindow(g_pHMDWindow);

    g_app.exitVR();

    if (g_pJoy != NULL)
        SDL_JoystickClose(g_pJoy);

    SDL_Quit();
}
