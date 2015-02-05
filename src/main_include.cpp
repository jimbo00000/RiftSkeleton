// main_include.cpp
// This cpp file is #included to glfw_main.cpp and sdl_main.cpp
// to avoid duplication of code.
// It depends on global variables defined in each respective main.
// Yes, this is an ugly technique - but not as bad as repeating the code.


#ifdef USE_ANTTWEAKBAR
static void TW_CALL EnableVSyncCB(void*) { SetVsync(1); }
static void TW_CALL DisableVSyncCB(void*) { SetVsync(0); }
static void TW_CALL AdaptiveVSyncCB(void*) { SetVsync(-1); }
static void TW_CALL RecenterPoseCB(void*) { g_app.RecenterPose(); }
static void TW_CALL StandingCB(void*) { glm::vec3 p(0,1.78f,2); g_app.SetChassisPosition(p); }
static void TW_CALL SittingCB(void*) { glm::vec3 p(0,1.27f,2); g_app.SetChassisPosition(p); }

static void TW_CALL GetDisplayFPS(void* value, void*)
{
    *(unsigned int *)value = static_cast<unsigned int>(g_fps.GetFPS());
}

void InitializeBar()
{
    ///@note Bad size errors will be thrown if this is not called before bar creation.
    TwWindowSize(g_auxWindow_w, g_auxWindow_h);

    // Create a tweak bar
    g_pTweakbar = TwNewBar("TweakBar");

    TwDefine(" GLOBAL fontsize=3 ");
    TwDefine(" TweakBar size='300 520' ");

    TwAddButton(g_pTweakbar, "Disable VSync", DisableVSyncCB, NULL, " group='VSync' ");
    TwAddButton(g_pTweakbar, "Enable VSync", EnableVSyncCB, NULL, " group='VSync' ");
    TwAddButton(g_pTweakbar, "Adaptive VSync", AdaptiveVSyncCB, NULL, " group='VSync' ");

    TwAddVarCB(g_pTweakbar, "Display FPS", TW_TYPE_UINT32, NULL, GetDisplayFPS, NULL, " group='Performance' ");

    TwAddVarRW(g_pTweakbar, "Target FPS", TW_TYPE_INT32, &g_targetFPS,
               " min=45 max=200 group='Performance' ");

    TwAddVarRW(g_pTweakbar, "FBO Scale", TW_TYPE_FLOAT, g_app.GetFBOScalePointer(),
               " min=0.05 max=1.0 step=0.005 group='Performance' ");
    TwAddVarRW(g_pTweakbar, "Dynamic FBO Scale", TW_TYPE_BOOLCPP, &g_dynamicallyScaleFBO,
               "  group='Performance' ");
    TwAddVarRW(g_pTweakbar, "DynFBO Smooth", TW_TYPE_FLOAT, &g_fpsSmoothingFactor,
               " min=0.001 max=1.0 step=0.001 group='Performance' ");
    TwAddVarRW(g_pTweakbar, "FPS Delta Threshold", TW_TYPE_FLOAT, &g_fpsDeltaThreshold,
               " min=0.0 max=100.0 step=1.0 group='Performance' ");
    TwAddVarRW(g_pTweakbar, "Draw to Aux Window", TW_TYPE_BOOLCPP, &g_drawToAuxWindow,
               "  group='Performance' ");


    TwAddButton(g_pTweakbar, "Recenter Pose", RecenterPoseCB, NULL, " group='Position' ");
    TwAddButton(g_pTweakbar, "Standing", StandingCB, NULL, " group='Position' ");
    TwAddButton(g_pTweakbar, "Sitting", SittingCB, NULL, " group='Position' ");


    TwAddVarRW(g_pTweakbar, "Draw Scene", TW_TYPE_BOOLCPP, &g_app.m_scene.m_bDraw,
               "  group='Scene' ");
    TwAddVarRW(g_pTweakbar, "amplitude", TW_TYPE_FLOAT, &g_app.m_scene.m_amplitude,
               " min=0 max=2 step=0.01 group='Scene' ");

    TwAddVarRW(g_pTweakbar, "Draw HydraScene", TW_TYPE_BOOLCPP, &g_app.m_hydraScene.m_bDraw,
               "  group='HydraScene' ");
    TwAddVarRW(g_pTweakbar, "Hydra Location x", TW_TYPE_FLOAT, &g_app.m_fm.m_baseOffset.x,
               " min=-10 max=10 step=0.05 group='HydraScene' ");
    TwAddVarRW(g_pTweakbar, "Hydra Location y", TW_TYPE_FLOAT, &g_app.m_fm.m_baseOffset.y,
               " min=-10 max=10 step=0.05 group='HydraScene' ");
    TwAddVarRW(g_pTweakbar, "Hydra Location z", TW_TYPE_FLOAT, &g_app.m_fm.m_baseOffset.z,
               " min=-10 max=10 step=0.05 group='HydraScene' ");
}
#endif


// Try to adjust the FBO scale on the fly to match rendering performance.
// Depends on globals g_fps, g_app, g_fpsSmoothingFactor, g_fpsDeltaThreshold, and g_targetFPS.
void DynamicallyScaleFBO()
{
    // Emergency condition: if we drop below a hard lower limit in any two successive frames,
    // immediately drop the FBO resolution to minimum.
    if (g_fps.GetInstantaneousFPS() < 45.0f)
    {
        g_app.SetFBOScale(0.0f); // bounds checks will choose minimum resolution
        return;
    }

    const float targetFPS = static_cast<float>(g_targetFPS);
    const float fps = g_fps.GetFPS();
    if (fabs(fps-targetFPS) < g_fpsDeltaThreshold)
        return;

    const float oldScale = g_app.GetFBOScale();
    // scale*scale*fps = targetscale*targetscale*targetfps
    const float scale2Fps = oldScale * oldScale * fps;
    const float targetScale = sqrt(scale2Fps / targetFPS);

    // Use smoothing here to avoid oscillation - blend old and new
    const float t = g_fpsSmoothingFactor;
    const float scale = (1.0f-t)*oldScale + t*targetScale;
    g_app.SetFBOScale(scale);
}
