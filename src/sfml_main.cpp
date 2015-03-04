// sfml_main.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#include <sstream>

#include <GL/glew.h>

#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

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

sf::Window* g_window = NULL;

int m_keyStates[4096];

// mouse motion internal state
int oldx, oldy, newx, newy;
int which_button = -1;
int modifier_mode = 0;

//ShaderWithVariables g_auxPresent;
int g_auxWindow_w = 600;
int g_auxWindow_h = 600;

float g_fpsSmoothingFactor = 0.02f;
float g_fpsDeltaThreshold = 5.0f;
bool g_dynamicallyScaleFBO = false;
int g_targetFPS = 100;
bool g_drawToAuxWindow = false;
bool g_allowPitch = false;
bool g_allowRoll = false;

#ifdef USE_ANTTWEAKBAR
TwBar* g_pTweakbar = NULL;
#endif

// Set VSync is framework-dependent and has to come before the include
static void SetVsync(int state)
{
    LOG_INFO("SetVsync(%d)", state);
    g_window->setVerticalSyncEnabled(state==1);
}

#include "main_include.cpp"

void timestep()
{
    const double absT = g_timer.seconds();
    const double dt = absT - g_lastFrameTime;
    g_lastFrameTime = absT;
    g_app.timestep(absT, dt);
}

void keyboard(const sf::Event& event)
{
    const sf::Event::KeyEvent& kevent = event.key;
    const float f = 0.9f;
    const float ff = 0.99f;

    if (event.type == sf::Event::KeyPressed)
    {
        switch(kevent.code)
        {
        default:
            g_app.DismissHealthAndSafetyWarning();
            break;

        case sf::Keyboard::F1:
            if (kevent.control)
                g_renderMode.toggleRenderingTypeReverse();
            else
                g_renderMode.toggleRenderingType();
            break;

        case sf::Keyboard::F2:
            g_renderMode.toggleRenderingTypeMono();
            break;

        case sf::Keyboard::F3:
            g_renderMode.toggleRenderingTypeHMD();
            break;

        case sf::Keyboard::F4:
            g_renderMode.toggleRenderingTypeDistortion();
            break;

        case sf::Keyboard::F5: g_dynamicallyScaleFBO = false; g_app.SetFBOScale(f * g_app.GetFBOScale()); break;
        case sf::Keyboard::F6: g_dynamicallyScaleFBO = false; g_app.SetFBOScale(ff * g_app.GetFBOScale()); break;
        case sf::Keyboard::F7: g_dynamicallyScaleFBO = false; g_app.SetFBOScale((1.f/ff) * g_app.GetFBOScale()); break;
        case sf::Keyboard::F8: g_dynamicallyScaleFBO = false; g_app.SetFBOScale((1.f/f) * g_app.GetFBOScale()); break;

        case sf::Keyboard::F9: SetVsync(0); break;
        case sf::Keyboard::F10: SetVsync(1); break;
        case sf::Keyboard::F11: SetVsync(-1); break;

        case sf::Keyboard::Escape:
            g_window->close();
            g_app.exitVR();
            break;
        }
    }
}

void mouseDown(int button, int state, int x, int y)
{
    which_button = button;
    oldx = newx = x;
    oldy = newy = y;
    //if (state == SDL_RELEASED)
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

#if 0
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
#endif
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
#if 0 //def USE_ANTTWEAKBAR
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
        g_window->display();
        break;

    case RenderingMode::Mono_Buffered:
        g_app.display_buffered();
        g_window->display();
        break;

    case RenderingMode::SideBySide_Undistorted:
        g_app.display_stereo_undistorted();
        g_window->display();
        break;

    case RenderingMode::OVR_SDK:
        g_app.display_sdk();
        // OVR will do its own swap
        break;

    case RenderingMode::OVR_Client:
        g_app.display_client();
        g_window->display();
        break;

    default:
        LOG_ERROR("Unknown display type: %d", g_renderMode.outputType);
        break;
    }
}

void PollEvents()
{
    // Process events
    sf::Event event;
    while (g_window->pollEvent(event))
    {
        // Close window: exit
        if (event.type == sf::Event::Closed)
            g_window->close();

        if (event.type == sf::Event::KeyPressed)
            keyboard(event);

        // Resize event: adjust the viewport
        if (event.type == sf::Event::Resized)
            glViewport(0, 0, event.size.width, event.size.height);
    }
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


    sf::ContextSettings contextSettings;
    contextSettings.depthBits = 32;


#ifdef USE_OCULUSSDK
    g_app.initHMD();
    const ovrSizei sz = g_app.getHmdResolution();
    const ovrVector2i pos = g_app.getHmdWindowPos();
    std::string windowTitle = "";

    if (g_app.UsingDebugHmd() == true)
    {
        // Create a normal, decorated application window
        LOG_INFO("Using Debug HMD mode.");
        windowTitle = "RiftSkeleton-SFML2-DebugHMD";

        g_window = new sf::Window(
            sf::VideoMode(sz.w, sz.h),
            windowTitle.c_str(),
            sf::Style::Default,
            contextSettings);
        g_window->setActive();
    }
    else
    {
        // HMD active - position undecorated window to fill HMD viewport
        if (g_app.UsingDirectMode())
        {
            LOG_INFO("Using Direct to Rift mode.");
            windowTitle = "RiftSkeleton-SFML2-Direct";

            g_window = new sf::Window(
                sf::VideoMode(sz.w, sz.h),
                windowTitle.c_str(),
                sf::Style::None);
            g_window->setPosition(sf::Vector2i(pos.x, pos.y));

#if defined(_WIN32)
            sf::WindowHandle winh = g_window->getSystemHandle();
            g_app.AttachToWindow(winh);
#endif
        }
        else
        {
            LOG_INFO("Using Extended desktop mode.");
            windowTitle = "RiftSkeleton-SFML2-Extended";

            g_window = new sf::Window(
                sf::VideoMode(sz.w, sz.h),
                windowTitle.c_str(),
                sf::Style::Default,
                contextSettings);
            g_window->setPosition(sf::Vector2i(pos.x, pos.y));
        }
        g_renderMode = renderMode;
    }
#else
    g_window = sf::Window(
        sf::VideoMode(sz.w, sz.h),
        windowTitle.c_str(),
        sf::Style::Default,
        contextSettings);
#endif //USE_OCULUSSDK



#ifdef USE_OCULUSSDK
    sf::WindowHandle winh = g_window->getSystemHandle();

  #if defined(OVR_OS_WIN32)
    g_app.setWindow(winh);
  #elif defined(OVR_OS_LINUX)
    g_app.setWindow(info.info.x11.display);
  #endif
#endif //USE_OCULUSSDK


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


    while (g_window->isOpen())
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
            oss << windowTitle
                << " "
                << static_cast<int>(g_fps.GetFPS())
                << " fps";
            //SDL_SetWindowTitle(g_pHMDWindow, oss.str().c_str());
        }
#endif

        const float dumpInterval = 1.f;
        if (g_logDumpTimer.seconds() > dumpInterval)
        {
            LOG_INFO("Frame rate: %d fps", static_cast<int>(g_fps.GetFPS()));
            g_logDumpTimer.reset();
        }

#if 0
        // Optionally display to auxiliary mono view
        if (g_pAuxWindow != NULL)
        {
            // SDL allows us to share contexts, so we can just call display on g_app
            // and use all of the VAOs resident in the HMD window's context.
            SDL_GL_MakeCurrent(g_pAuxWindow, g_HMDglContext);

            // Get window size from SDL - is this worth caching?
            int w, h;
            SDL_GetWindowSize(g_pAuxWindow, &w, &h);

            if (g_drawToAuxWindow)
            {
                glPushAttrib(GL_VIEWPORT_BIT);
                glViewport(0, 0, w, h);
                g_app.display_buffered(false);
                glPopAttrib(); // GL_VIEWPORT_BIT - if this is not misused!
            }
            else
            {
                glClearColor(0.f, 0.f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

#ifdef USE_ANTTWEAKBAR
            TwRefreshBar(g_pTweakbar);
            TwDraw(); ///@todo Should this go first? Will it write to a depth buffer?
#endif

            SDL_GL_SwapWindow(g_pAuxWindow);

            // Set context to Rift window when done
            SDL_GL_MakeCurrent(g_pHMDWindow, g_HMDglContext);
        }
#endif
    }

    g_app.exitVR();

    return EXIT_SUCCESS;
}
