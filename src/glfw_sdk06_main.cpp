// glfw_sdk06_main.cpp
// With humongous thanks to cThrough 2014 (Daniel Dekkers)

#include <GL/glew.h>

#if defined(_WIN32)
#  include <Windows.h>
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#endif

#include <GLFW/glfw3.h>

#if !defined(__APPLE__)
#  include <GLFW/glfw3native.h>
#endif

#include <glm/gtc/type_ptr.hpp>

#ifdef USE_ANTTWEAKBAR
#  include <AntTweakBar.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <algorithm>

#if defined(OVRSDK07)
#  include "OVRSDK07AppSkeleton.h"
#elif defined(OVRSDK06)
#  include "OVRSDK06AppSkeleton.h"
#else
#include "AppSkeleton.h"
#endif

#include "RenderingMode.h"
#include "Timer.h"
#include "FPSTimer.h"
#include "Logger.h"

#if defined(OVRSDK07)
OVRSDK07AppSkeleton g_app;
#elif defined(OVRSDK06)
OVRSDK06AppSkeleton g_app;
#else
AppSkeleton g_app;
#endif

#ifndef PROJECT_NAME
// This macro should be defined in CMakeLists.txt
#define PROJECT_NAME "RiftSkeleton"
#endif

RenderingMode g_renderMode;
Timer g_timer;
double g_lastFrameTime = 0.0;
FPSTimer g_fps;
Timer g_logDumpTimer;

int m_keyStates[GLFW_KEY_LAST];

// mouse motion internal state
int oldx, oldy, newx, newy;
int which_button = -1;
int modifier_mode = 0;

GLFWwindow* g_pMirrorWindow = NULL;
int g_auxWindow_w = 1920 / 2;
int g_auxWindow_h = 587;

int g_joystickIdx = -1;

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

GLFWwindow* initializeAuxiliaryWindow(GLFWwindow* pRiftWindow);
void destroyAuxiliaryWindow(GLFWwindow* pAuxWindow);

// Set VSync is framework-dependent and has to come before the include
///@param state 0=off, 1=on, -1=adaptive
// Set vsync for both contexts.
static void SetVsync(int state)
{
    LOG_INFO("SetVsync(%d)", state);
    glfwMakeContextCurrent(g_pMirrorWindow);
    glfwSwapInterval(state);
}

#include "main_include.cpp"

static void ErrorCallback(int p_Error, const char* p_Description)
{
    (void)p_Error;
    (void)p_Description;
    LOG_INFO("ERROR: %d, %s", p_Error, p_Description);
}


void keyboard(GLFWwindow* pWindow, int key, int codes, int action, int mods)
{
    (void)pWindow;
    (void)codes;

    if ((key > -1) && (key <= GLFW_KEY_LAST))
    {
        m_keyStates[key] = action;
        //printf("key ac  %d %d\n", key, action);
    }

    const float f = 0.9f;
    const float ff = 0.99f;

    if (action == GLFW_PRESS)
    {
    switch (key)
    {
        default:
            g_app.DismissHealthAndSafetyWarning();
            break;

        case GLFW_KEY_F1:
            if (mods & GLFW_MOD_CONTROL)
            {
                g_renderMode.toggleRenderingTypeReverse();
            }
            else
            {
                g_renderMode.toggleRenderingType();
            }
            LOG_INFO("Render Type: %d", g_renderMode.outputType);
            break;

        case GLFW_KEY_F2:
            g_renderMode.toggleRenderingTypeMono();
            LOG_INFO("Render Type: %d", g_renderMode.outputType);
            break;

        case GLFW_KEY_F3:
            g_renderMode.toggleRenderingTypeHMD();
            LOG_INFO("Render Type: %d", g_renderMode.outputType);
            break;

        case GLFW_KEY_F4:
            g_renderMode.toggleRenderingTypeDistortion();
            LOG_INFO("Render Type: %d", g_renderMode.outputType);
            break;

        case GLFW_KEY_F5: g_dynamicallyScaleFBO = false; g_app.SetFBOScale(f * g_app.GetFBOScale()); break;
        case GLFW_KEY_F6: g_dynamicallyScaleFBO = false; g_app.SetFBOScale(ff * g_app.GetFBOScale()); break;
        case GLFW_KEY_F7: g_dynamicallyScaleFBO = false; g_app.SetFBOScale((1.f/ff) * g_app.GetFBOScale()); break;
        case GLFW_KEY_F8: g_dynamicallyScaleFBO = false; g_app.SetFBOScale((1.f/f) * g_app.GetFBOScale()); break;

        case GLFW_KEY_F9: SetVsync(0); break;
        case GLFW_KEY_F10: SetVsync(1); break;
        case GLFW_KEY_F11: SetVsync(-1); break;

        case GLFW_KEY_DELETE: g_dynamicallyScaleFBO = !g_dynamicallyScaleFBO; break;

        case GLFW_KEY_SPACE:
            g_app.RecenterPose();
            break;

        case GLFW_KEY_TAB:
            g_app.ToggleQuadInWorld();
            break;

        case 'R':
            g_app.ResetChassisTransformations();
            break;

#ifdef USE_OCULUSSDK
        case 'V': g_app.ToggleVignette(); break;
        case 'T': g_app.ToggleTimeWarp(); break;
        case 'O': g_app.ToggleOverdrive(); break;
        case 'L': g_app.ToggleLowPersistence(); break;
        case 'M': g_app.ToggleMirroringType(); break;
        case 'P': g_app.ToggleDynamicPrediction(); break;
#endif

        case GLFW_KEY_ESCAPE:
            {
                g_app.exitVR();
                glfwDestroyWindow(g_pMirrorWindow);
                glfwTerminate();
                exit(0);
            }
            break;
        }
    }

    //g_app.keyboard(key, action, 0,0);

    const glm::vec3 forward(0.f, 0.f, -1.f);
    const glm::vec3 up(0.f, 1.f, 0.f);
    const glm::vec3 right(1.f, 0.f, 0.f);
    // Handle keyboard movement(WASD keys)
    glm::vec3 keyboardMove(0.0f, 0.0f, 0.0f);
    if (m_keyStates['W'] != GLFW_RELEASE) { keyboardMove += forward; }
    if (m_keyStates['S'] != GLFW_RELEASE) { keyboardMove -= forward; }
    if (m_keyStates['A'] != GLFW_RELEASE) { keyboardMove -= right; }
    if (m_keyStates['D'] != GLFW_RELEASE) { keyboardMove += right; }
    if (m_keyStates['Q'] != GLFW_RELEASE) { keyboardMove -= up; }
    if (m_keyStates['E'] != GLFW_RELEASE) { keyboardMove += up; }
    if (m_keyStates[GLFW_KEY_UP] != GLFW_RELEASE) { keyboardMove += forward; }
    if (m_keyStates[GLFW_KEY_DOWN] != GLFW_RELEASE) { keyboardMove -= forward; }
    if (m_keyStates[GLFW_KEY_LEFT] != GLFW_RELEASE) { keyboardMove -= right; }
    if (m_keyStates[GLFW_KEY_RIGHT] != GLFW_RELEASE) { keyboardMove += right; }

    float mag = 1.0f;
    if (m_keyStates[GLFW_KEY_LEFT_SHIFT] != GLFW_RELEASE)
        mag *= 0.1f;
    if (m_keyStates[GLFW_KEY_LEFT_CONTROL] != GLFW_RELEASE)
        mag *= 10.0f;

    // Yaw keys
    g_app.m_keyboardYaw = 0.0f;
    const float dyaw = 0.5f * mag; // radians at 60Hz timestep
    if (m_keyStates['1'] != GLFW_RELEASE) { g_app.m_keyboardYaw = -dyaw; }
    if (m_keyStates['3'] != GLFW_RELEASE) { g_app.m_keyboardYaw = dyaw; }

    // Pitch and roll controls - if yaw is VR poison, this is torture and death!
    g_app.m_keyboardDeltaPitch = 0.0f;
    g_app.m_keyboardDeltaRoll = 0.0f;
    if (g_allowPitch)
    {
        if (m_keyStates['2'] != GLFW_RELEASE) { g_app.m_keyboardDeltaPitch = -dyaw; }
        if (m_keyStates['X'] != GLFW_RELEASE) { g_app.m_keyboardDeltaPitch = dyaw; }
    }
    if (g_allowRoll)
    {
        if (m_keyStates['Z'] != GLFW_RELEASE) { g_app.m_keyboardDeltaRoll = -dyaw; }
        if (m_keyStates['C'] != GLFW_RELEASE) { g_app.m_keyboardDeltaRoll = dyaw; }
    }

    g_app.m_keyboardMove = mag * keyboardMove;
}

void joystick()
{
    if (g_joystickIdx == -1)
        return;

    ///@todo Do these calls take time? We can move them out if so
    int joyStick1Present = GL_FALSE;
    joyStick1Present = glfwJoystickPresent(g_joystickIdx);
    if (joyStick1Present != GL_TRUE)
        return;

    // Poll joystick
    int numAxes = 0;
    const float* pAxisStates = glfwGetJoystickAxes(g_joystickIdx, &numAxes);
    if (numAxes < 2)
        return;

    int numButtons = 0;
    const unsigned char* pButtonStates = glfwGetJoystickButtons(g_joystickIdx, &numButtons);
    if (numButtons < 8)
        return;

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

    // Xbox controller layout in glfw:
    // numAxes 5, numButtons 14
    // 0 A (down position)
    // 1 B (right position)
    // 2 X (left position)
    // 3 Y (up position)
    // 4 L bumper
    // 5 R bumper
    // 6 Back (left center)
    // 7 Start (right center)
    // Axis 0 1 Left stick x y
    // Axis 2 triggers, left positive right negative
    // Axis 3 4 right stick x y
    const glm::vec3 moveDirsXboxController[8] = {
        glm::vec3( 0.f,  0.f,  1.f),
        glm::vec3( 1.f,  0.f,  0.f),
        glm::vec3(-1.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f, -1.f),
        glm::vec3( 0.f, -1.f,  0.f),
        glm::vec3( 0.f,  1.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
        glm::vec3( 0.f,  0.f,  0.f),
    };

    ///@todo Different mappings for different controllers.
    const glm::vec3* moveDirs = moveDirsGravisGamepadPro;
    // Take an educated guess that this is an Xbox controller - glfw's
    // id string says "Microsoft PC Joystick" for most gamepad types.
    if (numAxes == 5 && numButtons == 14)
    {
        moveDirs = moveDirsXboxController;
    }

    glm::vec3 joystickMove(0.0f, 0.0f, 0.0f);
    for (int i=0; i<std::min(8,numButtons); ++i)
    {
        if (pButtonStates[i] == GLFW_PRESS)
        {
            joystickMove += moveDirs[i];
        }
    }

    float mag = 1.f;
    if (numAxes > 2)
    {
        mag = pow(10.f, pAxisStates[2]);
    }
    g_app.m_joystickMove = mag * joystickMove;

    float x_move = pAxisStates[0];
    const float deadzone = 0.2f;
    if (fabs(x_move) < deadzone)
        x_move = 0.0f;
    g_app.m_joystickYaw = 0.5f * static_cast<float>(x_move);
}

void mouseDown(GLFWwindow* pWindow, int button, int action, int mods)
{
    (void)mods;

    double xd, yd;
    glfwGetCursorPos(pWindow, &xd, &yd);
    const int x = static_cast<int>(xd);
    const int y = static_cast<int>(yd);

    which_button = button;
    oldx = newx = x;
    oldy = newy = y;
    if (action == GLFW_RELEASE)
    {
        which_button = -1;
    }
    g_app.OnMouseButton(button, action);
}

void mouseMove(GLFWwindow* pWindow, double xd, double yd)
{
    glfwGetCursorPos(pWindow, &xd, &yd);
    const int x = static_cast<int>(xd);
    const int y = static_cast<int>(yd);

    oldx = newx;
    oldy = newy;
    newx = x;
    newy = y;
    const int mmx = x-oldx;
    const int mmy = y-oldy;

    g_app.m_mouseDeltaYaw = 0.0f;
    g_app.m_mouseMove = glm::vec3(0.0f);

    if (which_button == GLFW_MOUSE_BUTTON_1)
    {
        const float spinMagnitude = 0.05f;
        g_app.m_mouseDeltaYaw += static_cast<float>(mmx) * spinMagnitude;
    }
    else if (which_button == GLFW_MOUSE_BUTTON_2) // Right click
    {
        const float moveMagnitude = 0.5f;
        g_app.m_mouseMove.x += static_cast<float>(mmx) * moveMagnitude;
        g_app.m_mouseMove.z += static_cast<float>(mmy) * moveMagnitude;
    }
    else if (which_button == GLFW_MOUSE_BUTTON_3) // Middle click
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

void mouseWheel(GLFWwindow* pWindow, double x, double y)
{
    (void)pWindow;
    (void)x;

    const int delta = static_cast<int>(y);
    const float curscale = g_app.GetFBOScale();
    const float incr = 1.05f;
    g_app.SetFBOScale(curscale * pow(incr, static_cast<float>(delta)));
    if (fabs(x) > 0.)
    {
        g_app.OnMouseWheel(x,y);
    }
}

void resize(GLFWwindow* pWindow, int w, int h)
{
    (void)pWindow;
    g_app.resize(w,h);
}

void keyboard_Aux(GLFWwindow* pWindow, int key, int codes, int action, int mods)
{
#ifdef USE_ANTTWEAKBAR
    int ant = TwEventKeyGLFW(key, action);
    if (ant != 0)
        return;
#endif
    keyboard(pWindow, key, codes, action, mods);
}

void mouseDown_Aux(GLFWwindow* pWindow, int button, int action, int mods)
{
    (void)pWindow;
    (void)mods;

#ifdef USE_ANTTWEAKBAR
    int ant = TwEventMouseButtonGLFW(button, action);
    if (ant != 0)
        return;
#endif
    mouseDown(pWindow, button, action, mods);
}

void mouseMove_Aux(GLFWwindow* pWindow, double xd, double yd)
{
    (void)pWindow;

#ifdef USE_ANTTWEAKBAR
    int ant = TwEventMousePosGLFW(static_cast<int>(xd), static_cast<int>(yd));
    if (ant != 0)
        return;
#endif
    mouseMove(pWindow, xd, yd);
}

void mouseWheel_Aux(GLFWwindow* pWindow, double x, double y)
{
#ifdef USE_ANTTWEAKBAR
    static int scrollpos = 0;
    scrollpos += static_cast<int>(y);
    int ant = TwEventMouseWheelGLFW(scrollpos);
    if (ant != 0)
        return;
#endif
    mouseWheel(pWindow, x, y);
}

void resize_Aux(GLFWwindow* pWindow, int w, int h)
{
    (void)pWindow;
    g_auxWindow_w = w;
    g_auxWindow_h = h;

#ifdef USE_ANTTWEAKBAR
    ///@note This will break PaneScene's tweakbar positioning
    TwWindowSize(w, h);
#endif

    ovrSizei sz = { w, h };
    g_app.SetAppWindowSize(sz);
}

void timestep()
{
    const double absT = g_timer.seconds();
    const double dt = absT - g_lastFrameTime;
    g_lastFrameTime = absT;
    g_app.timestep(absT, dt);
}

void printGLContextInfo(GLFWwindow* pW)
{
    // Print some info about the OpenGL context...
    const int l_Major = glfwGetWindowAttrib(pW, GLFW_CONTEXT_VERSION_MAJOR);
    const int l_Minor = glfwGetWindowAttrib(pW, GLFW_CONTEXT_VERSION_MINOR);
    const int l_Profile = glfwGetWindowAttrib(pW, GLFW_OPENGL_PROFILE);
    if (l_Major >= 3) // Profiles introduced in OpenGL 3.0...
    {
        if (l_Profile == GLFW_OPENGL_COMPAT_PROFILE)
        {
            LOG_INFO("GLFW_OPENGL_COMPAT_PROFILE");
        }
        else
        {
            LOG_INFO("GLFW_OPENGL_CORE_PROFILE");
        }
    }
    (void)l_Minor;
    LOG_INFO("OpenGL: %d.%d", l_Major, l_Minor);
    LOG_INFO("Vendor: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    LOG_INFO("Renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
}

void displayToHMD()
{
    switch(g_renderMode.outputType)
    {
    case RenderingMode::Mono_Raw:
        g_app.display_raw();
        glfwSwapBuffers(g_pMirrorWindow);
        break;

    case RenderingMode::Mono_Buffered:
        g_app.display_buffered();
        glfwSwapBuffers(g_pMirrorWindow);
        break;

#if defined(USE_OCULUSSDK)
    case RenderingMode::SideBySide_Undistorted:
    case RenderingMode::OVR_SDK:
    case RenderingMode::OVR_Client:
        g_app.display_sdk();
#ifdef USE_ANTTWEAKBAR
        TwDraw(); ///@todo Should this go first? Will it write to a depth buffer?
#endif
        glfwSwapBuffers(g_pMirrorWindow);
        break;
#endif //USE_OCULUSSDK

    default:
        LOG_ERROR("Unknown display type: %d", g_renderMode.outputType);
        break;
    }
}

// OpenGL debug callback
void GLAPIENTRY myCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *msg,
    const void *data)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM:
    case GL_DEBUG_SEVERITY_LOW:
        LOG_INFO("[[GL Debug]] %x %x %x %x %s", source, type, id, severity, msg);
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        break;
    }
}

int main(int argc, char** argv)
{
#if defined(_WIN32)
    LOG_INFO("Windows build.");
#elif defined(_LINUX)
    LOG_INFO("Linux build.");
#elif defined(_MACOS)
    LOG_INFO("MacOS build.");
#endif

    bool useOpenGLCoreContext = false;
#ifdef USE_CORE_CONTEXT
    useOpenGLCoreContext = true;
#endif

    g_renderMode.outputType = RenderingMode::OVR_SDK;

    LOG_INFO("Using GLFW3 backend.");
    LOG_INFO("Compiled against GLFW %i.%i.%i",
        GLFW_VERSION_MAJOR,
        GLFW_VERSION_MINOR,
        GLFW_VERSION_REVISION);
    int major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);
    LOG_INFO("Running against GLFW %i.%i.%i", major, minor, revision);
    LOG_INFO("glfwGetVersionString: %s", glfwGetVersionString());

    // Command line options
    for (int i=0; i<argc; ++i)
    {
        const std::string a = argv[i];
        LOG_INFO("argv[%d]: %s", i, a.c_str());
        if (!a.compare("-sdk"))
        {
            g_renderMode.outputType = RenderingMode::OVR_SDK;
        }
        else if (!a.compare("-client"))
        {
            g_renderMode.outputType = RenderingMode::OVR_Client;
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

#ifdef USE_OCULUSSDK
    g_app.initHMD();
#else
    g_renderMode.outputType = RenderingMode::Mono_Buffered;
#endif

    GLFWwindow* l_Window = NULL;
    glfwSetErrorCallback(ErrorCallback);
    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
    }

    // Log system monitor information
    const GLFWmonitor* pPrimary = glfwGetPrimaryMonitor();
    int monitorCount = 0;
    GLFWmonitor** ppMonitors = glfwGetMonitors(&monitorCount);
    for (int i=0; i<monitorCount; ++i)
    {
        GLFWmonitor* pCur = ppMonitors[i];
        const GLFWvidmode* mode = glfwGetVideoMode(pCur);
        if (mode != NULL)
        {
            (void)pPrimary;
            LOG_INFO("Monitor #%d: %dx%d @ %dHz %s",
                i,
                mode->width,
                mode->height,
                mode->refreshRate,
                pCur==pPrimary ? "Primary":"");
        }
    }

    bool swapBackBufferDims = false;

    // Context setup - before window creation
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, useOpenGLCoreContext ? GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_COMPAT_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

#if defined(OVRSDK06)
    std::string windowTitle = "";

    LOG_INFO("Using SDK 0.6.0.0's direct mode.");
    windowTitle = PROJECT_NAME "-GLFW-06-Direct";

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    ovrSizei sz;
    ovrVector2i pos;
    sz.w = mode->width/2;
    sz.h = mode->height/2;
    pos.x = 100;
    pos.y = 100;
    // Create just a regular window for presenting OVR SDK 0.6's mirror texture
    ///@todo Is it any faster with no mirror at all?
    LOG_INFO("Creating window %dx%d@%d,%d", sz.w, sz.h, pos.x, pos.y);
    l_Window = glfwCreateWindow(sz.w, sz.h, windowTitle.c_str(), NULL, NULL);
    g_app.SetAppWindowSize(sz);
    g_auxWindow_w = sz.w;
    g_auxWindow_h = sz.h;
#else
    l_Window = glfwCreateWindow(800, 600, "GLFW Oculus Rift Test", NULL, NULL);
    std::string windowTitle = PROJECT_NAME;
#endif

    if (!l_Window)
    {
        LOG_INFO("Glfw failed to create a window. Exiting.");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(l_Window);
    glfwSetWindowSizeCallback(l_Window, resize_Aux);
    glfwSetMouseButtonCallback(l_Window, mouseDown_Aux);
    glfwSetCursorPosCallback(l_Window, mouseMove_Aux);
    glfwSetScrollCallback(l_Window, mouseWheel_Aux);
    glfwSetKeyCallback(l_Window, keyboard_Aux);

    memset(m_keyStates, 0, GLFW_KEY_LAST*sizeof(int));

    // joysticks
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; ++i)
    {
        if (GL_FALSE == glfwJoystickPresent(i))
            continue;

        const char* pJoyName = glfwGetJoystickName(i);
        if (pJoyName == NULL)
            continue;

        int numAxes = 0;
        int numButtons = 0;
        glfwGetJoystickAxes(i, &numAxes);
        glfwGetJoystickButtons(i, &numButtons);

        LOG_INFO("Glfw opened Joystick #%d: %s w/ %d axes, %d buttons", i, pJoyName, numAxes, numButtons);
        if (g_joystickIdx == -1)
            g_joystickIdx = i;
    }

    printGLContextInfo(l_Window);
    glfwMakeContextCurrent(l_Window);
    g_pMirrorWindow = l_Window;


    // Don't forget to initialize Glew, turn glewExperimental on to
    // avoid problems fetching function pointers...
    glewExperimental = GL_TRUE;
    const GLenum l_Result = glewInit();
    if (l_Result != GLEW_OK)
    {
        LOG_INFO("glewInit() error.");
        exit(EXIT_FAILURE);
    }

#ifdef _DEBUG
    // Debug callback initialization
    // Must be done *after* glew initialization.
    glDebugMessageCallback(myCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
        GL_DEBUG_SEVERITY_NOTIFICATION, -1 , "Start debugging");
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

#ifdef USE_ANTTWEAKBAR
    LOG_INFO("Using AntTweakbar.");
    TwInit(useOpenGLCoreContext ? TW_OPENGL_CORE : TW_OPENGL, NULL);
    InitializeBar();
#endif

    LOG_INFO("Calling initGL...");
    g_app.initGL();
    LOG_INFO("Calling initVR...");
    g_app.initVR(swapBackBufferDims);
    LOG_INFO("initVR(%d) complete.", swapBackBufferDims);

    SetVsync(0); // SDK 0.6 requires vsync OFF

    TwWindowSize(g_auxWindow_w, g_auxWindow_h);
    while (!glfwWindowShouldClose(l_Window))
    {
        g_app.CheckForTapToDismissHealthAndSafetyWarning();
        glfwPollEvents();
        joystick();
        timestep();
        g_fps.OnFrame();
        if (g_dynamicallyScaleFBO)
        {
            DynamicallyScaleFBO();
        }

#ifdef USE_ANTTWEAKBAR
        TwRefreshBar(g_pTweakbar);
#endif

        displayToHMD();

#ifndef _LINUX
        // Indicate FPS in window title
        // This is absolute death for performance in Ubuntu Linux 12.04
        {
            std::ostringstream oss;
            oss << windowTitle
                << " "
                << static_cast<int>(g_fps.GetFPS())
                << " fps";
            glfwSetWindowTitle(l_Window, oss.str().c_str());
        }
#endif
        const float dumpInterval = 1.f;
        if (g_logDumpTimer.seconds() > dumpInterval)
        {
            LOG_INFO("Frame rate: %d fps", static_cast<int>(g_fps.GetFPS()));
            g_logDumpTimer.reset();
        }
    }

    g_app.exitVR();
    glfwDestroyWindow(l_Window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}
