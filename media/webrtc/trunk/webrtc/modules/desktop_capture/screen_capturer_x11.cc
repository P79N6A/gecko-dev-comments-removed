









#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include <string.h>
#include <set>

#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
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


class ScreenCapturerLinux : public ScreenCapturer,
                            public SharedXDisplay::XEventHandler {
 public:
  ScreenCapturerLinux();
  virtual ~ScreenCapturerLinux();

  
  bool Init(const DesktopCaptureOptions& options);

  
  virtual void Start(Callback* delegate) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

  
  virtual void SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) OVERRIDE;
  virtual bool GetScreenList(ScreenList* screens) OVERRIDE;
  virtual bool SelectScreen(ScreenId id) OVERRIDE;

 private:
  Display* display() { return options_.x_display()->display(); }

  
  virtual bool HandleXEvent(const XEvent& event) OVERRIDE;

  void InitXDamage();

  
  void CaptureCursor();

  
  
  
  
  
  DesktopFrame* CaptureScreen();

  
  void ScreenConfigurationChanged();

  
  
  
  
  
  void SynchronizeFrame();

  void DeinitXlib();

  DesktopCaptureOptions options_;

  Callback* callback_;
  MouseShapeObserver* mouse_shape_observer_;

  
  GC gc_;
  Window root_window_;

  
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
  options_.x_display()->RemoveEventHandler(ConfigureNotify, this);
  if (use_damage_) {
    options_.x_display()->RemoveEventHandler(
        damage_event_base_ + XDamageNotify, this);
  }
  if (has_xfixes_) {
    options_.x_display()->RemoveEventHandler(
        xfixes_event_base_ + XFixesCursorNotify, this);
  }
  DeinitXlib();
}

bool ScreenCapturerLinux::Init(const DesktopCaptureOptions& options) {
  options_ = options;

  root_window_ = RootWindow(display(), DefaultScreen(display()));
  if (root_window_ == BadValue) {
    LOG(LS_ERROR) << "Unable to get the root window";
    DeinitXlib();
    return false;
  }

  gc_ = XCreateGC(display(), root_window_, 0, NULL);
  if (gc_ == NULL) {
    LOG(LS_ERROR) << "Unable to get graphics context";
    DeinitXlib();
    return false;
  }

  options_.x_display()->AddEventHandler(ConfigureNotify, this);

  
  
  if (XFixesQueryExtension(display(), &xfixes_event_base_,
                           &xfixes_error_base_)) {
    has_xfixes_ = true;
  } else {
    LOG(LS_INFO) << "X server does not support XFixes.";
  }

  
  XSelectInput(display(), root_window_, StructureNotifyMask);

  if (!x_server_pixel_buffer_.Init(display(), DefaultRootWindow(display()))) {
    LOG(LS_ERROR) << "Failed to initialize pixel buffer.";
    return false;
  }

  if (has_xfixes_) {
    
    XFixesSelectCursorInput(display(), root_window_,
                            XFixesDisplayCursorNotifyMask);
    options_.x_display()->AddEventHandler(
        xfixes_event_base_ + XFixesCursorNotify, this);
  }

  if (options_.use_update_notifications()) {
    InitXDamage();
  }

  return true;
}

void ScreenCapturerLinux::InitXDamage() {
  
  if (!has_xfixes_) {
    return;
  }

  
  if (!XDamageQueryExtension(display(), &damage_event_base_,
                             &damage_error_base_)) {
    LOG(LS_INFO) << "X server does not support XDamage.";
    return;
  }

  
  
  
  

  
  damage_handle_ = XDamageCreate(display(), root_window_,
                                 XDamageReportNonEmpty);
  if (!damage_handle_) {
    LOG(LS_ERROR) << "Unable to initialize XDamage.";
    return;
  }

  
  damage_region_ = XFixesCreateRegion(display(), 0, 0);
  if (!damage_region_) {
    XDamageDestroy(display(), damage_handle_);
    LOG(LS_ERROR) << "Unable to create XFixes region.";
    return;
  }

  options_.x_display()->AddEventHandler(
      damage_event_base_ + XDamageNotify, this);

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

  
  options_.x_display()->ProcessPendingXEvents();

  
  
  
  if (!x_server_pixel_buffer_.is_initialized()) {
     
     callback_->OnCaptureCompleted(NULL);
     return;
  }

  
  
  
  if (!queue_.current_frame()) {
    scoped_ptr<DesktopFrame> frame(
        new BasicDesktopFrame(x_server_pixel_buffer_.window_size()));
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

bool ScreenCapturerLinux::GetScreenList(ScreenList* screens) {
  DCHECK(screens->size() == 0);
  
  Screen default_screen;
  default_screen.id = 0;
  screens->push_back(default_screen);
  return true;
}

bool ScreenCapturerLinux::SelectScreen(ScreenId id) {
  
  return true;
}

bool ScreenCapturerLinux::HandleXEvent(const XEvent& event) {
  if (use_damage_ && (event.type == damage_event_base_ + XDamageNotify)) {
    const XDamageNotifyEvent* damage_event =
        reinterpret_cast<const XDamageNotifyEvent*>(&event);
    if (damage_event->damage != damage_handle_)
      return false;
    DCHECK(damage_event->level == XDamageReportNonEmpty);
    return true;
  } else if (event.type == ConfigureNotify) {
    ScreenConfigurationChanged();
    return true;
  } else if (has_xfixes_ &&
             event.type == xfixes_event_base_ + XFixesCursorNotify) {
    const XFixesCursorNotifyEvent* cursor_event =
        reinterpret_cast<const XFixesCursorNotifyEvent*>(&event);
    if (cursor_event->window == root_window_ &&
        cursor_event->subtype == XFixesDisplayCursorNotify) {
      CaptureCursor();
    }
    
    
    return false;
  }
  return false;
}

void ScreenCapturerLinux::CaptureCursor() {
  DCHECK(has_xfixes_);

  XFixesCursorImage* img = XFixesGetCursorImage(display());
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
  assert(x_server_pixel_buffer_.window_size().equals(frame->size()));

  
  
  helper_.set_size_most_recent(frame->size());

  
  
  
  
  if (use_damage_ && queue_.previous_frame())
    SynchronizeFrame();

  DesktopRegion* updated_region = frame->mutable_updated_region();

  x_server_pixel_buffer_.Synchronize();
  if (use_damage_ && queue_.previous_frame()) {
    
    XDamageSubtract(display(), damage_handle_, None, damage_region_);
    int rects_num = 0;
    XRectangle bounds;
    XRectangle* rects = XFixesFetchRegionAndBounds(display(), damage_region_,
                                                   &rects_num, &bounds);
    for (int i = 0; i < rects_num; ++i) {
      updated_region->AddRect(DesktopRect::MakeXYWH(
          rects[i].x, rects[i].y, rects[i].width, rects[i].height));
    }
    XFree(rects);
    helper_.InvalidateRegion(*updated_region);

    
    helper_.TakeInvalidRegion(updated_region);

    
    
    
    updated_region->IntersectWith(
        DesktopRect::MakeSize(x_server_pixel_buffer_.window_size()));

    for (DesktopRegion::Iterator it(*updated_region);
         !it.IsAtEnd(); it.Advance()) {
      x_server_pixel_buffer_.CaptureRect(it.rect(), frame);
    }
  } else {
    
    
    DesktopRect screen_rect = DesktopRect::MakeSize(frame->size());
    x_server_pixel_buffer_.CaptureRect(screen_rect, frame);

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

void ScreenCapturerLinux::ScreenConfigurationChanged() {
  
  queue_.Reset();

  helper_.ClearInvalidRegion();
  if (!x_server_pixel_buffer_.Init(display(), DefaultRootWindow(display()))) {
    LOG(LS_ERROR) << "Failed to initialize pixel buffer after screen "
        "configuration change.";
  }
}

void ScreenCapturerLinux::SynchronizeFrame() {
  
  
  
  

  
  
  
  DCHECK(queue_.previous_frame());

  DesktopFrame* current = queue_.current_frame();
  DesktopFrame* last = queue_.previous_frame();
  DCHECK(current != last);
  for (DesktopRegion::Iterator it(last_invalid_region_);
       !it.IsAtEnd(); it.Advance()) {
    current->CopyPixelsFrom(*last, it.rect().top_left(), it.rect());
  }
}

void ScreenCapturerLinux::DeinitXlib() {
  if (gc_) {
    XFreeGC(display(), gc_);
    gc_ = NULL;
  }

  x_server_pixel_buffer_.Release();

  if (display()) {
    if (damage_handle_) {
      XDamageDestroy(display(), damage_handle_);
      damage_handle_ = 0;
    }

    if (damage_region_) {
      XFixesDestroyRegion(display(), damage_region_);
      damage_region_ = 0;
    }
  }
}

}  


ScreenCapturer* ScreenCapturer::Create(const DesktopCaptureOptions& options) {
  if (!options.x_display())
    return NULL;

  scoped_ptr<ScreenCapturerLinux> capturer(new ScreenCapturerLinux());
  if (!capturer->Init(options))
    capturer.reset();
  return capturer.release();
}

}  
