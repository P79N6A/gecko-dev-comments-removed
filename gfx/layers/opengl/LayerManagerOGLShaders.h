


static const char sLayerVS[] = "/* sLayerVS */\n\
\n\
uniform mat4 uMatrixProj;\n\
uniform mat4 uLayerQuadTransform;\n\
uniform mat4 uLayerTransform;\n\
uniform vec4 uRenderTargetOffset;\n\
attribute vec4 aVertexCoord;\n\
attribute vec2 aTexCoord;\n\
#ifdef GL_ES 





















static const char sLayerMaskVS[] = "/* sLayerMaskVS */\n\
\n\
uniform mat4 uMatrixProj;\n\
uniform mat4 uLayerQuadTransform;\n\
uniform mat4 uLayerTransform;\n\
uniform vec4 uRenderTargetOffset;\n\
attribute vec4 aVertexCoord;\n\
attribute vec2 aTexCoord;\n\
#ifdef GL_ES 
























static const char sLayerMask3DVS[] = "/* sLayerMask3DVS */\n\
\n\
uniform mat4 uMatrixProj;\n\
uniform mat4 uLayerQuadTransform;\n\
uniform mat4 uLayerTransform;\n\
uniform vec4 uRenderTargetOffset;\n\
attribute vec4 aVertexCoord;\n\
attribute vec2 aTexCoord;\n\
#ifdef GL_ES 



























static const char sSolidColorLayerFS[] = "/* sSolidColorLayerFS */\n\
#define NO_LAYER_OPACITY 1\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 














static const char sSolidColorLayerMaskFS[] = "/* sSolidColorLayerMaskFS */\n\
#define NO_LAYER_OPACITY 1\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 

















static const char sRGBATextureLayerFS[] = "/* sRGBATextureLayerFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 














static const char sRGBATextureLayerMaskFS[] = "/* sRGBATextureLayerMaskFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 

















static const char sRGBATextureLayerMask3DFS[] = "/* sRGBATextureLayerMask3DFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 


















static const char sRGBARectTextureLayerFS[] = "/* sRGBARectTextureLayerFS */\n\
#extension GL_ARB_texture_rectangle : enable\n\
/* Fragment Shader */\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES // for tiling, texcoord can be greater than the lowfp range\n\
varying mediump vec2 vTexCoord;\n\
#else\n\
varying vec2 vTexCoord;\n\
#endif\n\
\n\
/* This should not be used on GL ES */\n\
#ifndef GL_ES\n\
uniform sampler2DRect uTexture;\n\
uniform vec2 uTexCoordMultiplier;\n\
void main()\n\
{\n\
float mask = 1.0;\n\
\n\
gl_FragColor = texture2DRect(uTexture, vec2(vTexCoord * uTexCoordMultiplier)) * uLayerOpacity * mask;\n\
}\n\
#else\n\
void main()\n\
{\n\
gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n\
}\n\
#endif\n\
";

static const char sRGBARectTextureLayerMaskFS[] = "/* sRGBARectTextureLayerMaskFS */\n\
#extension GL_ARB_texture_rectangle : enable\n\
/* Fragment Shader */\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES // for tiling, texcoord can be greater than the lowfp range\n\
varying mediump vec2 vTexCoord;\n\
#else\n\
varying vec2 vTexCoord;\n\
#endif\n\
\n\
varying vec2 vMaskCoord;\n\
uniform sampler2D uMaskTexture;\n\
\n\
/* This should not be used on GL ES */\n\
#ifndef GL_ES\n\
uniform sampler2DRect uTexture;\n\
uniform vec2 uTexCoordMultiplier;\n\
void main()\n\
{\n\
float mask = texture2D(uMaskTexture, vMaskCoord).a;\n\
\n\
gl_FragColor = texture2DRect(uTexture, vec2(vTexCoord * uTexCoordMultiplier)) * uLayerOpacity * mask;\n\
}\n\
#else\n\
void main()\n\
{\n\
gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n\
}\n\
#endif\n\
";

static const char sRGBARectTextureLayerMask3DFS[] = "/* sRGBARectTextureLayerMask3DFS */\n\
#extension GL_ARB_texture_rectangle : enable\n\
/* Fragment Shader */\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES // for tiling, texcoord can be greater than the lowfp range\n\
varying mediump vec2 vTexCoord;\n\
#else\n\
varying vec2 vTexCoord;\n\
#endif\n\
\n\
varying vec3 vMaskCoord;\n\
uniform sampler2D uMaskTexture;\n\
\n\
/* This should not be used on GL ES */\n\
#ifndef GL_ES\n\
uniform sampler2DRect uTexture;\n\
uniform vec2 uTexCoordMultiplier;\n\
void main()\n\
{\n\
vec2 maskCoords = vMaskCoord.xy / vMaskCoord.z;\n\
float mask = texture2D(uMaskTexture, maskCoords).a;\n\
\n\
gl_FragColor = texture2DRect(uTexture, vec2(vTexCoord * uTexCoordMultiplier)) * uLayerOpacity * mask;\n\
}\n\
#else\n\
void main()\n\
{\n\
gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n\
}\n\
#endif\n\
";

static const char sBGRATextureLayerFS[] = "/* sBGRATextureLayerFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 














static const char sBGRATextureLayerMaskFS[] = "/* sBGRATextureLayerMaskFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 

















static const char sRGBXTextureLayerFS[] = "/* sRGBXTextureLayerFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 














static const char sRGBXTextureLayerMaskFS[] = "/* sRGBXTextureLayerMaskFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 

















static const char sBGRXTextureLayerFS[] = "/* sBGRXTextureLayerFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 














static const char sBGRXTextureLayerMaskFS[] = "/* sBGRXTextureLayerMaskFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 

















static const char sYCbCrTextureLayerFS[] = "/* sYCbCrTextureLayerFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 




























static const char sYCbCrTextureLayerMaskFS[] = "/* sYCbCrTextureLayerMaskFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 































static const char sComponentPass1FS[] = "/* sComponentPass1FS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 


















static const char sComponentPassMask1FS[] = "/* sComponentPassMask1FS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 





















static const char sComponentPass2FS[] = "/* sComponentPass2FS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 


















static const char sComponentPassMask2FS[] = "/* sComponentPassMask2FS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
#ifndef NO_LAYER_OPACITY\n\
uniform float uLayerOpacity;\n\
#endif\n\
#ifdef GL_ES 





















static const char sCopyVS[] = "/* sCopyVS */\n\
\n\
attribute vec4 aVertexCoord;\n\
attribute vec2 aTexCoord;\n\
varying vec2 vTexCoord;\n\
void main()\n\
{\n\
gl_Position = aVertexCoord;\n\
vTexCoord = aTexCoord;\n\
}\n\
";

static const char sCopy2DFS[] = "/* sCopy2DFS */\n\
\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
varying vec2 vTexCoord;\n\
uniform sampler2D uTexture;\n\
void main()\n\
{\n\
gl_FragColor = texture2D(uTexture, vTexCoord);\n\
}\n\
";

static const char sCopy2DRectFS[] = "/* sCopy2DRectFS */\n\
#extension GL_ARB_texture_rectangle : enable\n\
/* Fragment Shader */\n\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
\n\
varying vec2 vTexCoord;\n\
uniform vec2 uTexCoordMultiplier;\n\
#ifndef GL_ES\n\
uniform sampler2DRect uTexture;\n\
void main()\n\
{\n\
gl_FragColor = texture2DRect(uTexture, vTexCoord * uTexCoordMultiplier);\n\
}\n\
#else\n\
void main()\n\
{\n\
gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n\
}\n\
#endif\n\
";

