// IScene.h

#pragma once

///@brief Interface to a 3D Scene
class IScene
{
public:
    IScene() : m_bDraw(true) {}

    virtual void initGL() = 0;

    virtual void timestep(double absTime, double dt) = 0;

    virtual void RenderForOneEye(
        const float* pMview,
        const float* pPersp) const = 0;

    virtual bool RayIntersects(
        const float* pRayOrigin,
        const float* pRayDirection,
        float* pTParameter, // [inout]
        float* pHitLocation, // [inout]
        float* pHitNormal // [inout]
        ) const { return false; }

    bool m_bDraw;
};
