// presentmesh.vert
#version 330

uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;
uniform mat4 EyeRotationStart;
uniform mat4 EyeRotationEnd;

in vec4 vPosition;
in vec2 vTexR;
in vec2 vTexG;
in vec2 vTexB;

out vec2 vfTexR;
out vec2 vfTexG;
out vec2 vfTexB;
out float vfColor;

vec2 TimewarpTexCoord(vec2 TexCoord, mat4 rotMat)
{
    // Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic 
    // aberration and distortion). These are now "real world" vectors in direction (x,y,1) 
    // relative to the eye of the HMD. Apply the 3x3 timewarp rotation to these vectors.
    vec3 transformed = (rotMat * vec4(TexCoord.xy, 1., 1.)).xyz;
    // Project them back onto the Z=1 plane of the rendered images.
    vec2 flattened = (transformed.xy / transformed.z);
    // Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
    return EyeToSourceUVScale * flattened + EyeToSourceUVOffset;
}

void main()
{
    float timewarpLerpFactor = vPosition.z;
    //mat4 lerpedEyeRot = mix(EyeRotationStart, EyeRotationEnd, timewarpLerpFactor);
    mat4 lerpedEyeRot = (1.-timewarpLerpFactor)*EyeRotationStart + timewarpLerpFactor*EyeRotationEnd ;

    vfTexR = TimewarpTexCoord(vTexR, lerpedEyeRot);
    vfTexG = TimewarpTexCoord(vTexG, lerpedEyeRot);
    vfTexB = TimewarpTexCoord(vTexB, lerpedEyeRot);
    vfColor = vPosition.w;
    gl_Position = vec4(vPosition.xy, 0.5, 1.0);
}
