









#ifndef WEBRTC_BASE_WINDOWPICKER_H_
#define WEBRTC_BASE_WINDOWPICKER_H_

#include <string>
#include <vector>

#include "webrtc/base/window.h"

namespace rtc {

class WindowDescription {
 public:
  WindowDescription() : id_() {}
  WindowDescription(const WindowId& id, const std::string& title)
      : id_(id), title_(title) {
  }
  const WindowId& id() const { return id_; }
  void set_id(const WindowId& id) { id_ = id; }
  const std::string& title() const { return title_; }
  void set_title(const std::string& title) { title_ = title; }

 private:
  WindowId id_;
  std::string title_;
};

class DesktopDescription {
 public:
  DesktopDescription() : id_() {}
  DesktopDescription(const DesktopId& id, const std::string& title)
      : id_(id), title_(title), primary_(false) {
  }
  const DesktopId& id() const { return id_; }
  void set_id(const DesktopId& id) { id_ = id; }
  const std::string& title() const { return title_; }
  void set_title(const std::string& title) { title_ = title; }
  
  bool primary() const { return primary_; }
  void set_primary(bool primary) { primary_ = primary; }

 private:
  DesktopId id_;
  std::string title_;
  bool primary_;
};

typedef std::vector<WindowDescription> WindowDescriptionList;
typedef std::vector<DesktopDescription> DesktopDescriptionList;

class WindowPicker {
 public:
  virtual ~WindowPicker() {}
  virtual bool Init() = 0;

  
  
  virtual bool IsVisible(const WindowId& id) = 0;
  virtual bool MoveToFront(const WindowId& id) = 0;

  
  
  virtual bool GetWindowList(WindowDescriptionList* descriptions) = 0;
  
  
  virtual bool GetDesktopList(DesktopDescriptionList* descriptions) = 0;
  
  
  virtual bool GetDesktopDimensions(const DesktopId& id, int* width,
                                    int* height) = 0;
};

}  

#endif  
