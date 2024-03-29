








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_WIN_D3D_RENDERER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_WIN_D3D_RENDERER_H_

#include <Windows.h>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")       // located in DirectX SDK

#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/test/video_renderer.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class D3dRenderer : public VideoRenderer {
 public:
  static D3dRenderer* Create(const char* window_title, size_t width,
                             size_t height);
  virtual ~D3dRenderer();

  virtual void RenderFrame(const webrtc::I420VideoFrame& frame, int delta)
      OVERRIDE;
 private:
  D3dRenderer(size_t width, size_t height);

  static LRESULT WINAPI WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                   LPARAM lparam);
  bool Init(const char* window_title);
  void Resize(size_t width, size_t height);
  void Destroy();

  size_t width_, height_;

  HWND hwnd_;
  scoped_refptr<IDirect3D9> d3d_;
  scoped_refptr<IDirect3DDevice9> d3d_device_;

  scoped_refptr<IDirect3DTexture9> texture_;
  scoped_refptr<IDirect3DVertexBuffer9> vertex_buffer_;
};
}  
}  

#endif  
