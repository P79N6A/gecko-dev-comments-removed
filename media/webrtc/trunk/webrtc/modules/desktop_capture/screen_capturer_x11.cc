









#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include <string.h>
#include <set>

#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/differ.h"
#include "webrtc/modules/desktop_capture/mouse_cursor_shape.h"
#include "webrtc/modules/desktop_capture/screen_capture_frame_queue.h"
#include "webrtc/modules/desktop_capture/screen_capturer_helper.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"


#if defined(NDEBUG)
#define DCHECK(condition) (void)(condition)
#else  
#define DCHECK(condition) if (!(condition)) {abort();}
#endif

namespace webrtc {

namespace {


class ScreenCapturerLinux : public ScreenCapturer {
 public:
  ScreenCapturerLinux();
  virtual ~ScreenCapturerLinux();

  
  bool Init(bool use_x_damage);

  
  virtual void Start(Callback* delegate) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

  
  virtual void SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) OVERRIDE;

 private:
  void InitXDamage();

  
  
  
  
  
  void ProcessPendingXEvents();

  
  void CaptureCursor();

  
  
  
  
  
  DesktopFrame* CaptureScreen();

  
  
  void ScreenConfigurationChanged(const DesktopSize& root_window_size);

  
  
  
  
  
  void SynchronizeFrame();

  void DeinitXlib();

  
  
  void CaptureRect(const DesktopRect& rect,
                   DesktopFrame* frame);

  
  
  void FastBlit(uint8_t* image,
                const DesktopRect& rect,
                DesktopFrame* frame);
  void SlowBlit(uint8_t* image,
                const DesktopRect& rect,
                DesktopFrame* frame);

  
  
  
  static uint32_t GetRgbShift(uint32_t mask);

  Callback* callback_;
  MouseShapeObserver* mouse_shape_observer_;

  
  Display* display_;
  GC gc_;
  Window root_window_;

  
  DesktopSize root_window_size_;

  
  bool has_xfixes_;
  int xfixes_event_base_;
  int xfixes_error_base_;

  
  bool use_damage_;
  Damage damage_handle_;
  int damage_event_base_;
  int damage_error_base_;
  XserverRegion damage_region_;

  
  XServerPixelBuffer x_server_pixel_buffer_;

  
  
  ScreenCapturerHelper helper_;

  
  ScreenCaptureFrameQueue queue_;

  
  
  DesktopRegion last_invalid_region_;

  
  scoped_ptr<Differ> differ_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCapturerLinux);
};

ScreenCapturerLinux::ScreenCapturerLinux()
    : callback_(NULL),
      mouse_shape_observer_(NULL),
      display_(NULL),
      gc_(NULL),
      root_window_(BadValue),
      has_xfixes_(false),
      xfixes_event_base_(-1),
      xfixes_error_base_(-1),
      use_damage_(false),
      damage_handle_(0),
      damage_event_base_(-1),
      damage_error_base_(-1),
      damage_region_(0) {
  helper_.SetLogGridSize(4);
}

ScreenCapturerLinux::~ScreenCapturerLinux() {
  DeinitXlib();
}

bool ScreenCapturerLinux::Init(bool use_x_damage) {
  use_x_damage = true;

  
  
  display_ = XOpenDisplay(NULL);
  if (!display_) {
    LOG(LS_ERROR) << "Unable to open display";
    return false;
  }

  root_window_ = RootWindow(display_, DefaultScreen(display_));
  if (root_window_ == BadValue) {
    LOG(LS_ERROR) << "Unable to get the root window";
    DeinitXlib();
    return false;
  }

  gc_ = XCreateGC(display_, root_window_, 0, NULL);
  if (gc_ == NULL) {
    LOG(LS_ERROR) << "Unable to get graphics context";
    DeinitXlib();
    return false;
  }

  
  
  if (XFixesQueryExtension(display_, &xfixes_event_base_,
                           &xfixes_error_base_)) {
    has_xfixes_ = true;
  } else {
    LOG(LS_INFO) << "X server does not support XFixes.";
  }

  
  XSelectInput(display_, root_window_, StructureNotifyMask);

  root_window_size_ = XServerPixelBuffer::GetRootWindowSize(display_);
  x_server_pixel_buffer_.Init(display_, root_window_size_);

  if (has_xfixes_) {
    
    XFixesSelectCursorInput(display_, root_window_,
                            XFixesDisplayCursorNotifyMask);
  }

  if (use_x_damage) {
    InitXDamage();
  }

  return true;
}

void ScreenCapturerLinux::InitXDamage() {
  
  if (!has_xfixes_) {
    return;
  }

  
  if (!XDamageQueryExtension(display_, &damage_event_base_,
                             &damage_error_base_)) {
    LOG(LS_INFO) << "X server does not support XDamage.";
    return;
  }

  
  
  
  

  
  damage_handle_ = XDamageCreate(display_, root_window_,
                                 XDamageReportNonEmpty);
  if (!damage_handle_) {
    LOG(LS_ERROR) << "Unable to initialize XDamage.";
    return;
  }

  
  damage_region_ = XFixesCreateRegion(display_, 0, 0);
  if (!damage_region_) {
    XDamageDestroy(display_, damage_handle_);
    LOG(LS_ERROR) << "Unable to create XFixes region.";
    return;
  }

  use_damage_ = true;
  LOG(LS_INFO) << "Using XDamage extension.";
}

void ScreenCapturerLinux::Start(Callback* callback) {
  DCHECK(!callback_);
  DCHECK(callback);

  callback_ = callback;
}

void ScreenCapturerLinux::Capture(const DesktopRegion& region) {
  TickTime capture_start_time = TickTime::Now();

  queue_.MoveToNextFrame();

  
  ProcessPendingXEvents();

  
  
  
  if (!queue_.current_frame()) {
    scoped_ptr<DesktopFrame> frame(
        new BasicDesktopFrame(root_window_size_));
    queue_.ReplaceCurrentFrame(frame.release());
  }

  
  DesktopFrame* frame = queue_.current_frame();
  if (!use_damage_ && (
      !differ_.get() ||
      (differ_->width() != frame->size().width()) ||
      (differ_->height() != frame->size().height()) ||
      (differ_->bytes_per_row() != frame->stride()))) {
    differ_.reset(new Differ(frame->size().width(), frame->size().height(),
                             DesktopFrame::kBytesPerPixel,
                             frame->stride()));
  }

  DesktopFrame* result = CaptureScreen();
  last_invalid_region_ = result->updated_region();
  result->set_capture_time_ms(
      (TickTime::Now() - capture_start_time).Milliseconds());
  callback_->OnCaptureCompleted(result);
}

void ScreenCapturerLinux::SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) {
  DCHECK(!mouse_shape_observer_);
  DCHECK(mouse_shape_observer);

  mouse_shape_observer_ = mouse_shape_observer;
}

void ScreenCapturerLinux::ProcessPendingXEvents() {
  
  
  int events_to_process = XPending(display_);
  XEvent e;

  for (int i = 0; i < events_to_process; i++) {
    XNextEvent(display_, &e);
    if (use_damage_ && (e.type == damage_event_base_ + XDamageNotify)) {
      XDamageNotifyEvent* event = reinterpret_cast<XDamageNotifyEvent*>(&e);
      DCHECK(event->level == XDamageReportNonEmpty);
    } else if (e.type == ConfigureNotify) {
      const XConfigureEvent& event = e.xconfigure;
      ScreenConfigurationChanged(
          DesktopSize(event.width, event.height));
    } else if (has_xfixes_ &&
               e.type == xfixes_event_base_ + XFixesCursorNotify) {
      XFixesCursorNotifyEvent* cne;
      cne = reinterpret_cast<XFixesCursorNotifyEvent*>(&e);
      if (cne->subtype == XFixesDisplayCursorNotify) {
        CaptureCursor();
      }
    } else {
      LOG(LS_WARNING) << "Got unknown event type: " << e.type;
    }
  }
}

void ScreenCapturerLinux::CaptureCursor() {
  DCHECK(has_xfixes_);

  XFixesCursorImage* img = XFixesGetCursorImage(display_);
  if (!img) {
    return;
  }

  scoped_ptr<MouseCursorShape> cursor(new MouseCursorShape());
  cursor->size = DesktopSize(img->width, img->height);
  cursor->hotspot = DesktopVector(img->xhot, img->yhot);

  int total_bytes = cursor->size.width ()* cursor->size.height() *
      DesktopFrame::kBytesPerPixel;
  cursor->data.resize(total_bytes);

  
  unsigned long* src = img->pixels;
  uint32_t* dst = reinterpret_cast<uint32_t*>(&*(cursor->data.begin()));
  uint32_t* dst_end = dst + (img->width * img->height);
  while (dst < dst_end) {
    *dst++ = static_cast<uint32_t>(*src++);
  }
  XFree(img);

  if (mouse_shape_observer_)
    mouse_shape_observer_->OnCursorShapeChanged(cursor.release());
}

DesktopFrame* ScreenCapturerLinux::CaptureScreen() {
  DesktopFrame* frame = queue_.current_frame()->Share();

  
  
  helper_.set_size_most_recent(frame->size());

  
  
  
  
  if (use_damage_ && queue_.previous_frame())
    SynchronizeFrame();

  DesktopRegion* updated_region = frame->mutable_updated_region();

  x_server_pixel_buffer_.Synchronize();
  if (use_damage_ && queue_.previous_frame()) {
    
    XDamageSubtract(display_, damage_handle_, None, damage_region_);
    int rects_num = 0;
    XRectangle bounds;
    XRectangle* rects = XFixesFetchRegionAndBounds(display_, damage_region_,
                                                   &rects_num, &bounds);
    for (int i = 0; i < rects_num; ++i) {
      updated_region->AddRect(DesktopRect::MakeXYWH(
          rects[i].x, rects[i].y, rects[i].width, rects[i].height));
    }
    XFree(rects);
    helper_.InvalidateRegion(*updated_region);

    
    helper_.TakeInvalidRegion(updated_region);

    
    
    
    updated_region->IntersectWith(
        DesktopRect::MakeSize(root_window_size_));

    for (DesktopRegion::Iterator it(*updated_region);
         !it.IsAtEnd(); it.Advance()) {
      CaptureRect(it.rect(), frame);
    }
  } else {
    
    
    DesktopRect screen_rect =
        DesktopRect::MakeSize(frame->size());
    CaptureRect(screen_rect, frame);

    if (queue_.previous_frame()) {
      
      
      DCHECK(differ_.get() != NULL);
      DCHECK(queue_.previous_frame()->data());
      differ_->CalcDirtyRegion(queue_.previous_frame()->data(),
                               frame->data(), updated_region);
    } else {
      
      
      
      
      updated_region->SetRect(screen_rect);
    }
  }

  return frame;
}

void ScreenCapturerLinux::ScreenConfigurationChanged(
    const DesktopSize& root_window_size) {
  root_window_size_ = root_window_size;

  
  queue_.Reset();

  helper_.ClearInvalidRegion();
  x_server_pixel_buffer_.Init(display_, root_window_size_);
}

void ScreenCapturerLinux::SynchronizeFrame() {
  
  
  
  

  
  
  
  DCHECK(queue_.previous_frame());

  DesktopFrame* current = queue_.current_frame();
  DesktopFrame* last = queue_.previous_frame();
  DCHECK(current != last);
  for (DesktopRegion::Iterator it(last_invalid_region_);
       !it.IsAtEnd(); it.Advance()) {
    const DesktopRect& r = it.rect();
    int offset = r.top() * current->stride() +
        r.left() * DesktopFrame::kBytesPerPixel;
    for (int i = 0; i < r.height(); ++i) {
      memcpy(current->data() + offset, last->data() + offset,
             r.width() * DesktopFrame::kBytesPerPixel);
      offset += current->size().width() * DesktopFrame::kBytesPerPixel;
    }
  }
}

void ScreenCapturerLinux::DeinitXlib() {
  if (gc_) {
    XFreeGC(display_, gc_);
    gc_ = NULL;
  }

  x_server_pixel_buffer_.Release();

  if (display_) {
    if (damage_handle_)
      XDamageDestroy(display_, damage_handle_);
    if (damage_region_)
      XFixesDestroyRegion(display_, damage_region_);
    XCloseDisplay(display_);
    display_ = NULL;
    damage_handle_ = 0;
    damage_region_ = 0;
  }
}

void ScreenCapturerLinux::CaptureRect(const DesktopRect& rect,
                                      DesktopFrame* frame) {
  uint8_t* image = x_server_pixel_buffer_.CaptureRect(rect);
  int depth = x_server_pixel_buffer_.GetDepth();
  if ((depth == 24 || depth == 32) &&
      x_server_pixel_buffer_.GetBitsPerPixel() == 32 &&
      x_server_pixel_buffer_.GetRedMask() == 0xff0000 &&
      x_server_pixel_buffer_.GetGreenMask() == 0xff00 &&
      x_server_pixel_buffer_.GetBlueMask() == 0xff) {
    FastBlit(image, rect, frame);
  } else {
    SlowBlit(image, rect, frame);
  }
}

void ScreenCapturerLinux::FastBlit(uint8_t* image,
                                   const DesktopRect& rect,
                                   DesktopFrame* frame) {
  uint8_t* src_pos = image;
  int src_stride = x_server_pixel_buffer_.GetStride();
  int dst_x = rect.left(), dst_y = rect.top();

  uint8_t* dst_pos = frame->data() + frame->stride() * dst_y;
  dst_pos += dst_x * DesktopFrame::kBytesPerPixel;

  int height = rect.height();
  int row_bytes = rect.width() * DesktopFrame::kBytesPerPixel;
  for (int y = 0; y < height; ++y) {
    memcpy(dst_pos, src_pos, row_bytes);
    src_pos += src_stride;
    dst_pos += frame->stride();
  }
}

void ScreenCapturerLinux::SlowBlit(uint8_t* image,
                                   const DesktopRect& rect,
                                   DesktopFrame* frame) {
  int src_stride = x_server_pixel_buffer_.GetStride();
  int dst_x = rect.left(), dst_y = rect.top();
  int width = rect.width(), height = rect.height();

  uint32_t red_mask = x_server_pixel_buffer_.GetRedMask();
  uint32_t green_mask = x_server_pixel_buffer_.GetGreenMask();
  uint32_t blue_mask = x_server_pixel_buffer_.GetBlueMask();

  uint32_t red_shift = GetRgbShift(red_mask);
  uint32_t green_shift = GetRgbShift(green_mask);
  uint32_t blue_shift = GetRgbShift(blue_mask);

  unsigned int bits_per_pixel = x_server_pixel_buffer_.GetBitsPerPixel();

  uint8_t* dst_pos = frame->data() + frame->stride() * dst_y;
  uint8_t* src_pos = image;
  dst_pos += dst_x * DesktopFrame::kBytesPerPixel;
  
  
  for (int y = 0; y < height; y++) {
    uint32_t* dst_pos_32 = reinterpret_cast<uint32_t*>(dst_pos);
    uint32_t* src_pos_32 = reinterpret_cast<uint32_t*>(src_pos);
    uint16_t* src_pos_16 = reinterpret_cast<uint16_t*>(src_pos);
    for (int x = 0; x < width; x++) {
      
      uint32_t pixel;
      if (bits_per_pixel == 32) {
        pixel = src_pos_32[x];
      } else if (bits_per_pixel == 16) {
        pixel = src_pos_16[x];
      } else {
        pixel = src_pos[x];
      }
      uint32_t r = (pixel & red_mask) << red_shift;
      uint32_t g = (pixel & green_mask) << green_shift;
      uint32_t b = (pixel & blue_mask) << blue_shift;

      
      dst_pos_32[x] = ((r >> 8) & 0xff0000) | ((g >> 16) & 0xff00) |
          ((b >> 24) & 0xff);
    }
    dst_pos += frame->stride();
    src_pos += src_stride;
  }
}


uint32_t ScreenCapturerLinux::GetRgbShift(uint32_t mask) {
  int shift = 0;
  if ((mask & 0xffff0000u) == 0) {
    mask <<= 16;
    shift += 16;
  }
  if ((mask & 0xff000000u) == 0) {
    mask <<= 8;
    shift += 8;
  }
  if ((mask & 0xf0000000u) == 0) {
    mask <<= 4;
    shift += 4;
  }
  if ((mask & 0xc0000000u) == 0) {
    mask <<= 2;
    shift += 2;
  }
  if ((mask & 0x80000000u) == 0)
    shift += 1;

  return shift;
}

}  


ScreenCapturer* ScreenCapturer::Create() {
  scoped_ptr<ScreenCapturerLinux> capturer(new ScreenCapturerLinux());
  if (!capturer->Init(false))
    capturer.reset();
  return capturer.release();
}


ScreenCapturer* ScreenCapturer::CreateWithXDamage(bool use_x_damage) {
  scoped_ptr<ScreenCapturerLinux> capturer(new ScreenCapturerLinux());
  if (!capturer->Init(use_x_damage))
    capturer.reset();
  return capturer.release();
}

}  
