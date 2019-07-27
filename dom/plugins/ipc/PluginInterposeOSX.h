




























#ifndef DOM_PLUGINS_IPC_PLUGININTERPOSEOSX_H
#define DOM_PLUGINS_IPC_PLUGININTERPOSEOSX_H

#include "base/basictypes.h"
#include "nsPoint.h"
#include "npapi.h"


#ifndef __OBJC__
class NSCursor;
#else
#import <Cocoa/Cocoa.h>
#endif






#if !defined(__QUICKDRAWAPI__)

typedef short Bits16[16];
struct Cursor {
  Bits16  data;
  Bits16  mask;
  Point   hotSpot;
};
typedef struct Cursor Cursor;

#endif 

namespace mac_plugin_interposing {


class NSCursorInfo {
public:
  enum Type {
    TypeCustom,
    TypeArrow,
    TypeClosedHand,
    TypeContextualMenu,   
    TypeCrosshair,
    TypeDisappearingItem,
    TypeDragCopy,         
    TypeDragLink,         
    TypeIBeam,
    TypeNotAllowed,       
    TypeOpenHand,
    TypePointingHand,
    TypeResizeDown,
    TypeResizeLeft,
    TypeResizeLeftRight,
    TypeResizeRight,
    TypeResizeUp,
    TypeResizeUpDown,
    TypeTransparent       
  };

  NSCursorInfo();
  explicit NSCursorInfo(NSCursor* aCursor);
  explicit NSCursorInfo(const Cursor* aCursor);
  ~NSCursorInfo();

  NSCursor* GetNSCursor() const;
  Type GetType() const;
  const char* GetTypeName() const;
  nsPoint GetHotSpot() const;
  uint8_t* GetCustomImageData() const;
  uint32_t GetCustomImageDataLength() const;

  void SetType(Type aType);
  void SetHotSpot(nsPoint aHotSpot);
  void SetCustomImageData(uint8_t* aData, uint32_t aDataLength);

  static bool GetNativeCursorsSupported();

private:
  NSCursor* GetTransparentCursor() const;

  Type mType;
  
  
  nsPoint mHotSpot;
  uint8_t* mCustomImageData;
  uint32_t mCustomImageDataLength;
  static int32_t mNativeCursorsSupported;
};

namespace parent {

void OnPluginShowWindow(uint32_t window_id, CGRect window_bounds, bool modal);
void OnPluginHideWindow(uint32_t window_id, pid_t aPluginPid);
void OnSetCursor(const NSCursorInfo& cursorInfo);
void OnShowCursor(bool show);
void OnPushCursor(const NSCursorInfo& cursorInfo);
void OnPopCursor();

}

namespace child {

void SetUpCocoaInterposing();

}

}

#endif
