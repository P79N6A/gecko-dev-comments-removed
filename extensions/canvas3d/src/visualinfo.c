






























#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
#include <AGL/agl.h>
#else
#include <GL/glxew.h>
#endif

#ifdef GLEW_MX
GLEWContext _glewctx;
#  define glewGetContext() (&_glewctx)
#  ifdef _WIN32
WGLEWContext _wglewctx;
#    define wglewGetContext() (&_wglewctx)
#  elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
GLXEWContext _glxewctx;
#    define glxewGetContext() (&_glxewctx)
#  endif
#endif 

typedef struct GLContextStruct
{
#ifdef _WIN32
  HWND wnd;
  HDC dc;
  HGLRC rc;
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
  AGLContext ctx, octx;
#else
  Display* dpy;
  XVisualInfo* vi;
  GLXContext ctx;
  Window wnd;
  Colormap cmap;
#endif
} GLContext;

void InitContext (GLContext* ctx);
GLboolean CreateContext (GLContext* ctx);
void DestroyContext (GLContext* ctx);
void VisualInfo (GLContext* ctx);
void PrintExtensions (const char* s);
GLboolean ParseArgs (int argc, char** argv);

int showall = 0;
int displaystdout = 0;
int verbose = 0;
int drawableonly = 0;

char* display = NULL;
int visual = -1;

FILE* file = 0;
GLContext ctx;

int 
main (int argc, char** argv)
{
  GLenum err;

  
  
  if (GL_TRUE == ParseArgs(argc-1, argv+1))
  {
#if defined(_WIN32)
    fprintf(stderr, "Usage: visualinfo [-a] [-s] [-h] [-pf <id>]\n");
    fprintf(stderr, "        -a: show all visuals\n");
    fprintf(stderr, "        -s: display to stdout instead of visualinfo.txt\n");
    fprintf(stderr, "        -pf <id>: use given pixelformat\n");
    fprintf(stderr, "        -h: this screen\n");
#else
    fprintf(stderr, "Usage: visualinfo [-h] [-display <display>] [-visual <id>]\n");
    fprintf(stderr, "        -h: this screen\n");
    fprintf(stderr, "        -display <display>: use given display\n");
    fprintf(stderr, "        -visual <id>: use given visual\n");
#endif
    return 1;
  }

  
  
  InitContext(&ctx);
  if (GL_TRUE == CreateContext(&ctx))
  {
    fprintf(stderr, "Error: CreateContext failed\n");
    DestroyContext(&ctx);
    return 1;
  }

  
  
  glewExperimental = GL_TRUE;
#ifdef GLEW_MX
  err = glewContextInit(glewGetContext());
#  ifdef _WIN32
  err = err || wglewContextInit(wglewGetContext());
#  elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
  err = err || glxewContextInit(glxewGetContext());
#  endif
#else
  err = glewInit();
#endif
  if (GLEW_OK != err)
  {
    fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
    DestroyContext(&ctx);
    return 1;
  }

  
  
#if defined(_WIN32)
  if (!displaystdout) 
    file = fopen("visualinfo.txt", "w");
  if (file == NULL)
    file = stdout;
#else
  file = stdout;
#endif

  
  
  
  fprintf(file, "OpenGL vendor string: %s\n", glGetString(GL_VENDOR));
  fprintf(file, "OpenGL renderer string: %s\n", glGetString(GL_RENDERER));
  fprintf(file, "OpenGL version string: %s\n", glGetString(GL_VERSION));
  fprintf(file, "OpenGL extensions (GL_): \n");
  PrintExtensions((char*)glGetString(GL_EXTENSIONS));
  
  fprintf(file, "GLU version string: %s\n", gluGetString(GLU_VERSION));
  fprintf(file, "GLU extensions (GLU_): \n");
  PrintExtensions((char*)gluGetString(GLU_EXTENSIONS));

  
  
#if defined(_WIN32)
  
  if (WGLEW_ARB_extensions_string || WGLEW_EXT_extensions_string)
  {
    fprintf(file, "WGL extensions (WGL_): \n");
    PrintExtensions(wglGetExtensionsStringARB ? 
                    (char*)wglGetExtensionsStringARB(ctx.dc) :
		    (char*)wglGetExtensionsStringEXT());
  }
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
  
#else
  
  fprintf(file, "GLX extensions (GLX_): \n");
  PrintExtensions(glXQueryExtensionsString(glXGetCurrentDisplay(), 
                                           DefaultScreen(glXGetCurrentDisplay())));
#endif

  
  
  VisualInfo(&ctx);

  
  
  DestroyContext(&ctx);
  if (file != stdout)
    fclose(file);
  return 0;
}



void PrintExtensions (const char* s)
{
  char t[80];
  int i=0;
  char* p=0;

  t[79] = '\0';
  while (*s)
  {
    t[i++] = *s;
    if(*s == ' ')
    {
      if (*(s+1) != '\0') {
	t[i-1] = ',';
	t[i] = ' ';
	p = &t[i++];
      }
      else 
      {
	t[i-1] = '\0';
      }
    }
    if(i > 80 - 5)
    {
      *p = t[i] = '\0';
      fprintf(file, "    %s\n", t);
      p++;
      i = (int)strlen(p);
      strcpy(t, p);
    }
    s++;
  }
  t[i] = '\0';
  fprintf(file, "    %s.\n", t);
}



#if defined(_WIN32)

void
VisualInfoARB (GLContext* ctx)
{
  int attrib[32], value[32], n_attrib, n_pbuffer=0, n_float=0;
  int i, pf, maxpf;
  unsigned int c;

  
  attrib[0] = WGL_DRAW_TO_PBUFFER_ARB;
  attrib[1] = GL_TRUE;
  attrib[2] = 0;
  wglChoosePixelFormatARB(ctx->dc, attrib, 0, 1, &pf, &c);
  
  attrib[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
  wglGetPixelFormatAttribivARB(ctx->dc, 0, 0, 1, attrib, value);
  maxpf = value[0];
  for (i=0; i<32; i++)
    value[i] = 0;

  attrib[0] = WGL_SUPPORT_OPENGL_ARB;
  attrib[1] = WGL_DRAW_TO_WINDOW_ARB;
  attrib[2] = WGL_DRAW_TO_BITMAP_ARB;
  attrib[3] = WGL_ACCELERATION_ARB;
  
  attrib[4] = WGL_SWAP_METHOD_ARB;
  
  attrib[5] = WGL_DOUBLE_BUFFER_ARB;
  attrib[6] = WGL_STEREO_ARB;
  attrib[7] = WGL_PIXEL_TYPE_ARB;
  

  
  attrib[8] = WGL_COLOR_BITS_ARB;
  attrib[9] = WGL_RED_BITS_ARB;
  attrib[10] = WGL_GREEN_BITS_ARB;
  attrib[11] = WGL_BLUE_BITS_ARB;
  attrib[12] = WGL_ALPHA_BITS_ARB;
  
  attrib[13] = WGL_ACCUM_BITS_ARB;
  attrib[14] = WGL_ACCUM_RED_BITS_ARB;
  attrib[15] = WGL_ACCUM_GREEN_BITS_ARB;
  attrib[16] = WGL_ACCUM_BLUE_BITS_ARB;
  attrib[17] = WGL_ACCUM_ALPHA_BITS_ARB;
  
  attrib[18] = WGL_DEPTH_BITS_ARB;
  attrib[19] = WGL_STENCIL_BITS_ARB;
  attrib[20] = WGL_AUX_BUFFERS_ARB;
  
  attrib[21] = WGL_NUMBER_OVERLAYS_ARB;
  attrib[22] = WGL_NUMBER_UNDERLAYS_ARB;
  attrib[23] = WGL_SWAP_LAYER_BUFFERS_ARB;
  attrib[24] = WGL_SAMPLES_ARB;
  attrib[25] = WGL_SUPPORT_GDI_ARB;
  n_attrib = 26;
  if (WGLEW_ARB_pbuffer)
  {
    attrib[n_attrib] = WGL_DRAW_TO_PBUFFER_ARB;
    n_pbuffer = n_attrib;
    n_attrib++;
  }
  if (WGLEW_NV_float_buffer)
  {
    attrib[n_attrib] = WGL_FLOAT_COMPONENTS_NV;
    n_float = n_attrib;
    n_attrib++;
  }
  
  if (!verbose)
  {
    
    fprintf(file, " +-----+-------------------------+-----------------+----------+-----------------+----------+\n");
    fprintf(file, " |     |          visual         |      color      | ax dp st |      accum      |   layer  |\n");
    fprintf(file, " |  id | tp ac gd fm db sw st ms |  sz  r  g  b  a | bf th cl |  sz  r  g  b  a | ov un sw |\n");
    fprintf(file, " +-----+-------------------------+-----------------+----------+-----------------+----------+\n");
    
    for(i = 1; i <= maxpf; i++)
    {
      wglGetPixelFormatAttribivARB(ctx->dc, i, 0, n_attrib, attrib, value);
      
      if (!value[0]) continue;
      
      if (!showall
	  && ((value[2] && !value[1])
	  || (!WGLEW_ARB_pbuffer || !value[n_pbuffer])
	  || (value[3] != WGL_FULL_ACCELERATION_ARB))) continue;
      
      
      fprintf(file, " |% 4d | ", i);
      
      if (value[1])
      {
	if (WGLEW_ARB_pbuffer && value[n_pbuffer]) fprintf(file, "wp ");
	else fprintf(file, "wn ");
      }
      else
      {
	if (value[2]) fprintf(file, "bm ");
	else if (WGLEW_ARB_pbuffer && value[n_pbuffer]) fprintf(file, "pb ");
      }
      
      fprintf(file, "%s ", value[3] == WGL_FULL_ACCELERATION_ARB ? "fu" : 
	      value[3] == WGL_GENERIC_ACCELERATION_ARB ? "ge" :
	      value[3] == WGL_NO_ACCELERATION_ARB ? "no" : ". ");
      
      fprintf(file, " %c ", value[25] ? 'y' : '.');
      
      if (WGLEW_NV_float_buffer && value[n_float]) fprintf(file, " f ");
      else if (WGLEW_ATI_pixel_format_float && value[7] == WGL_TYPE_RGBA_FLOAT_ATI) fprintf(file, " f ");
      else if (value[7] == WGL_TYPE_RGBA_ARB) fprintf(file, " i ");
      else if (value[7] == WGL_TYPE_COLORINDEX_ARB) fprintf(file, " c ");
      
      fprintf(file, " %c ", value[5] ? 'y' : '.');
      
      if (value[4] == WGL_SWAP_EXCHANGE_ARB) fprintf(file, " x ");
      else if (value[4] == WGL_SWAP_COPY_ARB) fprintf(file, " c ");
      else if (value[4] == WGL_SWAP_UNDEFINED_ARB) fprintf(file, " . ");
      else fprintf(file, " . ");
      
      fprintf(file, " %c ", value[6] ? 'y' : '.');
      
      if (value[24] > 0)
	fprintf(file, "%2d | ", value[24]);
      else
	fprintf(file, " . | ");
      
      if (value[8]) fprintf(file, "%3d ", value[8]);
      else fprintf(file, "  . ");
      
      if (value[9]) fprintf(file, "%2d ", value[9]); 
      else fprintf(file, " . ");
      
      if (value[10]) fprintf(file, "%2d ", value[10]); 
      else fprintf(file, " . ");
      
      if (value[11]) fprintf(file, "%2d ", value[11]);
      else fprintf(file, " . ");
      
      if (value[12]) fprintf(file, "%2d | ", value[12]); 
      else fprintf(file, " . | ");
      
      if (value[20]) fprintf(file, "%2d ", value[20]);
      else fprintf(file, " . ");
      
      if (value[18]) fprintf(file, "%2d ", value[18]);
      else fprintf(file, " . ");
      
      if (value[19]) fprintf(file, "%2d | ", value[19]);
      else fprintf(file, " . | ");
      
      if (value[13]) fprintf(file, "%3d ", value[13]);
      else fprintf(file, "  . ");
      
      if (value[14]) fprintf(file, "%2d ", value[14]);
      else fprintf(file, " . ");
      
      if (value[15]) fprintf(file, "%2d ", value[15]);
      else fprintf(file, " . ");
      
      if (value[16]) fprintf(file, "%2d ", value[16]);
      else fprintf(file, " . ");
      
      if (value[17]) fprintf(file, "%2d | ", value[17]);
      else fprintf(file, " . | ");
      
      if (value[21]) fprintf(file, "%2d ", value[21]);
      else fprintf(file, " . ");
      
      if (value[22]) fprintf(file, "%2d ", value[22]);
      else fprintf(file, " . ");
      
      if (value[23]) fprintf(file, "y ");
      else fprintf(file, " . ");
      fprintf(file, "|\n");
    }
    
    fprintf(file, " +-----+-------------------------+-----------------+----------+-----------------+----------+\n");
    fprintf(file, " |     |          visual         |      color      | ax dp st |      accum      |   layer  |\n");
    fprintf(file, " |  id | tp ac gd fm db sw st ms |  sz  r  g  b  a | bf th cl |  sz  r  g  b  a | ov un sw |\n");
    fprintf(file, " +-----+-------------------------+-----------------+----------+-----------------+----------+\n");
  }
  else 
  {
#if 0
    fprintf(file, "\n");
    
    for(i = 1; i <= maxpf; i++)
    {	    
      DescribePixelFormat(ctx->dc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
      
      if(!(pfd.dwFlags & PFD_SUPPORT_OPENGL)
	 || (drawableonly && !(pfd.dwFlags & PFD_DRAW_TO_WINDOW))) continue;
      fprintf(file, "Visual ID: %2d  depth=%d  class=%s\n", i, pfd.cDepthBits, 
	     pfd.cColorBits <= 8 ? "PseudoColor" : "TrueColor");
      fprintf(file, "    bufferSize=%d level=%d renderType=%s doubleBuffer=%d stereo=%d\n", pfd.cColorBits, pfd.bReserved, pfd.iPixelType == PFD_TYPE_RGBA ? "rgba" : "ci", pfd.dwFlags & PFD_DOUBLEBUFFER, pfd.dwFlags & PFD_STEREO);
      fprintf(file, "    generic=%d generic accelerated=%d\n", (pfd.dwFlags & PFD_GENERIC_FORMAT) == PFD_GENERIC_FORMAT, (pfd.dwFlags & PFD_GENERIC_ACCELERATED) == PFD_GENERIC_ACCELERATED);
      fprintf(file, "    rgba: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n", pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits);
      fprintf(file, "    auxBuffers=%d depthSize=%d stencilSize=%d\n", pfd.cAuxBuffers, pfd.cDepthBits, pfd.cStencilBits);
      fprintf(file, "    accum: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n", pfd.cAccumRedBits, pfd.cAccumGreenBits, pfd.cAccumBlueBits, pfd.cAccumAlphaBits);
      fprintf(file, "    multiSample=%d multisampleBuffers=%d\n", 0, 0);
      fprintf(file, "    Opaque.\n");
    }
#endif
  }
}

void
VisualInfoGDI (GLContext* ctx)
{
  int i, maxpf;
  PIXELFORMATDESCRIPTOR pfd;

  

  maxpf = DescribePixelFormat(ctx->dc, 1, 0, NULL);

  if (!verbose)
  {
    fprintf(file, "-----------------------------------------------------------------------------\n");
    fprintf(file, "   visual   x  bf  lv rg d st ge ge  r  g  b a  ax dp st   accum buffs    ms \n");
    fprintf(file, " id  dep tp sp sz  l  ci b ro ne ac sz sz sz sz bf th cl  sz  r  g  b  a ns b\n");
    fprintf(file, "-----------------------------------------------------------------------------\n");

    
    for(i = 1; i <= maxpf; i++)
    {
      DescribePixelFormat(ctx->dc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
      
      if(!(pfd.dwFlags & PFD_SUPPORT_OPENGL)
	 || (drawableonly && (pfd.dwFlags & PFD_DRAW_TO_BITMAP))) continue;
      















      
      fprintf(file, "0x%02x ", i);
      fprintf(file, "%3d ", pfd.cColorBits);
      if(pfd.dwFlags & PFD_DRAW_TO_WINDOW) fprintf(file, "wn ");
      else if(pfd.dwFlags & PFD_DRAW_TO_BITMAP) fprintf(file, "bm ");
      else fprintf(file, "pb ");
      
      fprintf(file, " . "); 
      fprintf(file, "%3d ", pfd.cColorBits);
      
      if(pfd.bReserved) fprintf(file, " %d ", pfd.bReserved);
      else fprintf(file, " . "); 
      fprintf(file, " %c ", pfd.iPixelType == PFD_TYPE_RGBA ? 'r' : 'c');
      fprintf(file, "%c ", pfd.dwFlags & PFD_DOUBLEBUFFER ? 'y' : '.');
      fprintf(file, " %c ", pfd.dwFlags & PFD_STEREO ? 'y' : '.');
      
      fprintf(file, " %c ", pfd.dwFlags & PFD_GENERIC_FORMAT ? 'y' : '.');
      fprintf(file, " %c ", pfd.dwFlags & PFD_GENERIC_ACCELERATED ? 'y' : '.');
      if(pfd.cRedBits && pfd.iPixelType == PFD_TYPE_RGBA) 
	fprintf(file, "%2d ", pfd.cRedBits);
      else fprintf(file, " . ");
      if(pfd.cGreenBits && pfd.iPixelType == PFD_TYPE_RGBA) 
	fprintf(file, "%2d ", pfd.cGreenBits);
      else fprintf(file, " . ");
      if(pfd.cBlueBits && pfd.iPixelType == PFD_TYPE_RGBA) 
	fprintf(file, "%2d ", pfd.cBlueBits);
      else fprintf(file, " . ");
      if(pfd.cAlphaBits && pfd.iPixelType == PFD_TYPE_RGBA) 
	fprintf(file, "%2d ", pfd.cAlphaBits);
      else fprintf(file, " . ");
      if(pfd.cAuxBuffers)     fprintf(file, "%2d ", pfd.cAuxBuffers);
      else fprintf(file, " . ");
      if(pfd.cDepthBits)      fprintf(file, "%2d ", pfd.cDepthBits);
      else fprintf(file, " . ");
      if(pfd.cStencilBits)    fprintf(file, "%2d ", pfd.cStencilBits);
      else fprintf(file, " . ");
      if(pfd.cAccumBits)   fprintf(file, "%3d ", pfd.cAccumBits);
      else fprintf(file, "  . ");
      if(pfd.cAccumRedBits)   fprintf(file, "%2d ", pfd.cAccumRedBits);
      else fprintf(file, " . ");
      if(pfd.cAccumGreenBits) fprintf(file, "%2d ", pfd.cAccumGreenBits);
      else fprintf(file, " . ");
      if(pfd.cAccumBlueBits)  fprintf(file, "%2d ", pfd.cAccumBlueBits);
      else fprintf(file, " . ");
      if(pfd.cAccumAlphaBits) fprintf(file, "%2d ", pfd.cAccumAlphaBits);
      else fprintf(file, " . ");
      
      fprintf(file, " . .\n");
    }
    
    fprintf(file, "-----------------------------------------------------------------------------\n");
    fprintf(file, "   visual   x  bf  lv rg d st ge ge  r  g  b a  ax dp st   accum buffs    ms \n");
    fprintf(file, " id  dep tp sp sz  l  ci b ro ne ac sz sz sz sz bf th cl  sz  r  g  b  a ns b\n");
    fprintf(file, "-----------------------------------------------------------------------------\n");
  }
  else 
  {
    fprintf(file, "\n");
    
    for(i = 1; i <= maxpf; i++)
    {	    
      DescribePixelFormat(ctx->dc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
      
      if(!(pfd.dwFlags & PFD_SUPPORT_OPENGL)
	 || (drawableonly && !(pfd.dwFlags & PFD_DRAW_TO_WINDOW))) continue;
      fprintf(file, "Visual ID: %2d  depth=%d  class=%s\n", i, pfd.cDepthBits, 
	     pfd.cColorBits <= 8 ? "PseudoColor" : "TrueColor");
      fprintf(file, "    bufferSize=%d level=%d renderType=%s doubleBuffer=%ld stereo=%ld\n", pfd.cColorBits, pfd.bReserved, pfd.iPixelType == PFD_TYPE_RGBA ? "rgba" : "ci", pfd.dwFlags & PFD_DOUBLEBUFFER, pfd.dwFlags & PFD_STEREO);
      fprintf(file, "    generic=%d generic accelerated=%d\n", (pfd.dwFlags & PFD_GENERIC_FORMAT) == PFD_GENERIC_FORMAT, (pfd.dwFlags & PFD_GENERIC_ACCELERATED) == PFD_GENERIC_ACCELERATED);
      fprintf(file, "    rgba: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n", pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits);
      fprintf(file, "    auxBuffers=%d depthSize=%d stencilSize=%d\n", pfd.cAuxBuffers, pfd.cDepthBits, pfd.cStencilBits);
      fprintf(file, "    accum: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n", pfd.cAccumRedBits, pfd.cAccumGreenBits, pfd.cAccumBlueBits, pfd.cAccumAlphaBits);
      fprintf(file, "    multiSample=%d multisampleBuffers=%d\n", 0, 0);
      fprintf(file, "    Opaque.\n");
    }
  }
}

void
VisualInfo (GLContext* ctx)
{
  if (WGLEW_ARB_pixel_format)
    VisualInfoARB(ctx);
  else
    VisualInfoGDI(ctx);
}



#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)

void
VisualInfo (GLContext* ctx)
{












}

#else 

void
VisualInfo (GLContext* ctx)
{
  int n_fbc;
  GLXFBConfig* fbc;
  int value, ret, i;

  fbc = glXGetFBConfigs(ctx->dpy, DefaultScreen(ctx->dpy), &n_fbc);

  if (fbc)
  {
    if (!verbose)
    {
      
      fprintf(file, " +-----+-------------------------+-----------------+----------+-------------+-------+------+\n");
      fprintf(file, " |     |        visual           |      color      | ax dp st |    accum    |   ms  |  cav |\n");
      fprintf(file, " |  id | tp xr cl fm db st lv xp |  sz  r  g  b  a | bf th cl | r  g  b  a  | ns  b |  eat |\n");
      fprintf(file, " +-----+-------------------------+-----------------+----------+-------------+-------+------+\n");
      
      for (i=0; i<n_fbc; i++)
      {
        
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_FBCONFIG_ID, &value);
        if (ret != Success)
        {
          fprintf(file, "|  ?  |");
        }
        else
        {
          fprintf(file, " |% 4d | ", value);
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_DRAWABLE_TYPE, &value);
        if (ret != Success)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value & GLX_WINDOW_BIT)
          {
            if (value & GLX_PBUFFER_BIT)
            {
              fprintf(file, "wp ");
            }
            else
            {
              fprintf(file, "wn ");
            }
          }
          else
          {
            if (value & GLX_PBUFFER_BIT)
            {
              fprintf(file, "pb ");
            }
            else if (value & GLX_PIXMAP_BIT)
            {
              fprintf(file, "pm ");
            }
            else
            {
              fprintf(file, " ? ");
            }
          }
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_X_RENDERABLE, &value);
        if (ret != Success)
        {
          fprintf(file, " ? ");
        }
        else
        {
          fprintf(file, value ? " y " : " n ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_X_VISUAL_TYPE, &value);
        if (ret != Success)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (GLX_TRUE_COLOR == value)
            fprintf(file, "tc ");
          else if (GLX_DIRECT_COLOR == value)
            fprintf(file, "dc ");
          else if (GLX_PSEUDO_COLOR == value)
            fprintf(file, "pc ");
          else if (GLX_STATIC_COLOR == value)
            fprintf(file, "sc ");
          else if (GLX_GRAY_SCALE == value)
            fprintf(file, "gs ");
          else if (GLX_STATIC_GRAY == value)
            fprintf(file, "sg ");
          else if (GLX_X_VISUAL_TYPE == value)
            fprintf(file, " . ");
          else
            fprintf(file, " ? ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_RENDER_TYPE, &value);
        if (ret != Success)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (GLXEW_NV_float_buffer)
          {
            int ret2, value2;
            ret2 = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_FLOAT_COMPONENTS_NV, &value2);
            if (Success == ret2 && GL_TRUE == value2)
            {
              fprintf(file, " f ");
            }
            else if (value & GLX_RGBA_BIT)
              fprintf(file, " i ");
            else if (value & GLX_COLOR_INDEX_BIT)
              fprintf(file, " c ");
            else
              fprintf(file, " ? ");
          }
          else
          {
            if (value & GLX_RGBA_FLOAT_ATI_BIT)
              fprintf(file, " f ");
            else if (value & GLX_RGBA_BIT)
              fprintf(file, " i ");
            else if (value & GLX_COLOR_INDEX_BIT)
              fprintf(file, " c ");
            else
              fprintf(file, " ? ");
          }
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_DOUBLEBUFFER, &value);
        fprintf(file, " %c ", Success != ret ? '?' : (value ? 'y' : '.'));
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_STEREO, &value);
        fprintf(file, " %c ", Success != ret ? '?' : (value ? 'y' : '.'));
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_LEVEL, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          fprintf(file, "%2d ", value);
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_TRANSPARENT_TYPE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? | ");
        }
        else
        {
          if (GLX_TRANSPARENT_RGB == value)
            fprintf(file, " r | ");
          else if (GLX_TRANSPARENT_INDEX == value)
            fprintf(file, " i | ");
          else if (GLX_NONE == value)
            fprintf(file, " . | ");
          else
            fprintf(file, " ? | ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_BUFFER_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, "  ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%3d ", value);
          else
            fprintf(file, "  . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_RED_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_GREEN_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_BLUE_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_ALPHA_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? | ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d | ", value);
          else
            fprintf(file, " . | ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_AUX_BUFFERS, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_DEPTH_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_STENCIL_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? | ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d | ", value);
          else
            fprintf(file, " . | ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_ACCUM_RED_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_ACCUM_GREEN_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_ACCUM_BLUE_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d ", value);
          else
            fprintf(file, " . ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_ACCUM_ALPHA_SIZE, &value);
        if (Success != ret)
        {
          fprintf(file, " ? | ");
        }
        else
        {
          if (value)
            fprintf(file, "%2d | ", value);
          else
            fprintf(file, " . | ");
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_SAMPLES, &value);
        if (Success != ret)
        {
          fprintf(file, " ? ");
        }
        else
        {
          fprintf(file, "%2d ", value);
        }
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_SAMPLE_BUFFERS, &value);
        if (Success != ret)
        {
          fprintf(file, " ? | ");
        }
        else
        {
          fprintf(file, "%2d | ", value);
        }
        
        ret = glXGetFBConfigAttrib(ctx->dpy, fbc[i], GLX_CONFIG_CAVEAT, &value);
        if (Success != ret)
        {
          fprintf(file, "???? |");
        }
        else
        {
          if (GLX_NONE == value)
            fprintf(file, "none |\n");
          else if (GLX_SLOW_CONFIG == value)
            fprintf(file, "slow |\n");
          else if (GLX_NON_CONFORMANT_CONFIG == value)
            fprintf(file, "ncft |\n");
          else
            fprintf(file, "???? |\n");
        }
      }
      
      fprintf(file, " +-----+-------------------------+-----------------+----------+-------------+-------+------+\n");
      fprintf(file, " |  id | tp xr cl fm db st lv xp |  sz  r  g  b  a | bf th cl | r  g  b  a  | ns  b |  eat |\n");
      fprintf(file, " |     |        visual           |      color      | ax dp st |    accum    |   ms  |  cav |\n");
      fprintf(file, " +-----+-------------------------+-----------------+----------+-------------+-------+------+\n");
    }
  }
}

#endif



#if defined(_WIN32)

void InitContext (GLContext* ctx)
{
  ctx->wnd = NULL;
  ctx->dc = NULL;
  ctx->rc = NULL;
}

GLboolean CreateContext (GLContext* ctx)
{
  WNDCLASS wc;
  PIXELFORMATDESCRIPTOR pfd;
  
  if (NULL == ctx) return GL_TRUE;
  
  ZeroMemory(&wc, sizeof(WNDCLASS));
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpfnWndProc = DefWindowProc;
  wc.lpszClassName = "GLEW";
  if (0 == RegisterClass(&wc)) return GL_TRUE;
  
  ctx->wnd = CreateWindow("GLEW", "GLEW", 0, CW_USEDEFAULT, CW_USEDEFAULT, 
                          CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, 
                          GetModuleHandle(NULL), NULL);
  if (NULL == ctx->wnd) return GL_TRUE;
  
  ctx->dc = GetDC(ctx->wnd);
  if (NULL == ctx->dc) return GL_TRUE;
  
  ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
  if (visual == -1) 
  {
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    visual = ChoosePixelFormat(ctx->dc, &pfd);
    if (0 == visual) return GL_TRUE;
  }
  
  if (FALSE == SetPixelFormat(ctx->dc, visual, &pfd)) return GL_TRUE;
  
  ctx->rc = wglCreateContext(ctx->dc);
  if (NULL == ctx->rc) return GL_TRUE;
  if (FALSE == wglMakeCurrent(ctx->dc, ctx->rc)) return GL_TRUE;
  return GL_FALSE;
}

void DestroyContext (GLContext* ctx)
{
  if (NULL == ctx) return;
  if (NULL != ctx->rc) wglMakeCurrent(NULL, NULL);
  if (NULL != ctx->rc) wglDeleteContext(wglGetCurrentContext());
  if (NULL != ctx->wnd && NULL != ctx->dc) ReleaseDC(ctx->wnd, ctx->dc);
  if (NULL != ctx->wnd) DestroyWindow(ctx->wnd);
  UnregisterClass("GLEW", GetModuleHandle(NULL));
}



#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)

void InitContext (GLContext* ctx)
{
  ctx->ctx = NULL;
  ctx->octx = NULL;
}

GLboolean CreateContext (GLContext* ctx)
{
  int attrib[] = { AGL_RGBA, AGL_NONE };
  AGLPixelFormat pf;
  
  if (NULL == ctx) return GL_TRUE;
  



  pf = aglChoosePixelFormat(NULL, 0, attrib);
  if (NULL == pf) return GL_TRUE;
  ctx->ctx = aglCreateContext(pf, NULL);
  if (NULL == ctx->ctx || AGL_NO_ERROR != aglGetError()) return GL_TRUE;
  aglDestroyPixelFormat(pf);
  
  ctx->octx = aglGetCurrentContext();
  if (NULL == aglSetCurrentContext(ctx->ctx)) return GL_TRUE;
  return GL_FALSE;
}

void DestroyContext (GLContext* ctx)
{
  if (NULL == ctx) return;
  aglSetCurrentContext(ctx->octx);
  if (NULL != ctx->ctx) aglDestroyContext(ctx->ctx);
}



#else 

void InitContext (GLContext* ctx)
{
  ctx->dpy = NULL;
  ctx->vi = NULL;
  ctx->ctx = NULL;
  ctx->wnd = 0;
  ctx->cmap = 0;
}

GLboolean CreateContext (GLContext* ctx)
{
  int attrib[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
  int erb, evb;
  XSetWindowAttributes swa;
  
  if (NULL == ctx) return GL_TRUE;
  
  ctx->dpy = XOpenDisplay(display);
  if (NULL == ctx->dpy) return GL_TRUE;
  
  if (!glXQueryExtension(ctx->dpy, &erb, &evb)) return GL_TRUE;
  
  ctx->vi = glXChooseVisual(ctx->dpy, DefaultScreen(ctx->dpy), attrib);
  if (NULL == ctx->vi) return GL_TRUE;
  
  ctx->ctx = glXCreateContext(ctx->dpy, ctx->vi, None, True);
  if (NULL == ctx->ctx) return GL_TRUE;
  
  
  ctx->cmap = XCreateColormap(ctx->dpy, RootWindow(ctx->dpy, ctx->vi->screen),
                              ctx->vi->visual, AllocNone);
  swa.border_pixel = 0;
  swa.colormap = ctx->cmap;
  ctx->wnd = XCreateWindow(ctx->dpy, RootWindow(ctx->dpy, ctx->vi->screen), 
                           0, 0, 1, 1, 0, ctx->vi->depth, InputOutput, ctx->vi->visual, 
                           CWBorderPixel | CWColormap, &swa);
  
  if (!glXMakeCurrent(ctx->dpy, ctx->wnd, ctx->ctx)) return GL_TRUE;
  return GL_FALSE;
}

void DestroyContext (GLContext* ctx)
{
  if (NULL != ctx->dpy && NULL != ctx->ctx) glXDestroyContext(ctx->dpy, ctx->ctx);
  if (NULL != ctx->dpy && 0 != ctx->wnd) XDestroyWindow(ctx->dpy, ctx->wnd);
  if (NULL != ctx->dpy && 0 != ctx->cmap) XFreeColormap(ctx->dpy, ctx->cmap);
  if (NULL != ctx->vi) XFree(ctx->vi);
  if (NULL != ctx->dpy) XCloseDisplay(ctx->dpy);
}

#endif 

GLboolean ParseArgs (int argc, char** argv)
{
  int p = 0;
  while (p < argc)
  {
#if defined(_WIN32)
    if (!strcmp(argv[p], "-pf") || !strcmp(argv[p], "-pixelformat"))
    {
      if (++p >= argc) return GL_TRUE;
      display = NULL;
      visual = strtol(argv[p], NULL, 0);
    }
    else if (!strcmp(argv[p], "-a"))
    {
      showall = 1;
    }
    else if (!strcmp(argv[p], "-s"))
    {
      displaystdout = 1;
    }
    else if (!strcmp(argv[p], "-h"))
    {
      return GL_TRUE;
    }
    else
      return GL_TRUE;
#else
    if (!strcmp(argv[p], "-display"))
    {
      if (++p >= argc) return GL_TRUE;
      display = argv[p];
    }
    else if (!strcmp(argv[p], "-visual"))
    {
      if (++p >= argc) return GL_TRUE;
      visual = (int)strtol(argv[p], NULL, 0);
    }
    else if (!strcmp(argv[p], "-h"))
    {
      return GL_TRUE;
    }
    else
      return GL_TRUE;
#endif
    p++;
  }
  return GL_FALSE;
}
