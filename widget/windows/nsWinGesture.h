





































#ifndef WinGesture_h__
#define WinGesture_h__





#include "nsdefs.h"
#include <winuser.h>
#include "nsPoint.h"
#include "nsGUIEvent.h"

#ifndef HGESTUREINFO  

DECLARE_HANDLE(HGESTUREINFO);




#define GF_BEGIN                        0x00000001
#define GF_INERTIA                      0x00000002
#define GF_END                          0x00000004








typedef struct tagGESTURECONFIG {
    DWORD dwID;                     
    DWORD dwWant;                   
    DWORD dwBlock;                  
} GESTURECONFIG, *PGESTURECONFIG;









typedef struct tagGESTUREINFO {
    UINT cbSize;                    
    DWORD dwFlags;                  
    DWORD dwID;                     
    HWND hwndTarget;                
    POINTS ptsLocation;             
    DWORD dwInstanceID;             
    DWORD dwSequenceID;             
    ULONGLONG ullArguments;         
    UINT cbExtraArgs;               
} GESTUREINFO, *PGESTUREINFO;
typedef GESTUREINFO const * PCGESTUREINFO;








typedef struct tagGESTURENOTIFYSTRUCT {
    UINT cbSize;                    
    DWORD dwFlags;                  
    HWND hwndTarget;                
    POINTS ptsLocation;             
    DWORD dwInstanceID;             
} GESTURENOTIFYSTRUCT, *PGESTURENOTIFYSTRUCT;







#define GID_ROTATE_ANGLE_TO_ARGUMENT(_arg_)     ((USHORT)((((_arg_) + 2.0 * 3.14159265) / (4.0 * 3.14159265)) * 65535.0))
#define GID_ROTATE_ANGLE_FROM_ARGUMENT(_arg_)   ((((double)(_arg_) / 65535.0) * 4.0 * 3.14159265) - 2.0 * 3.14159265)




#define GC_ALLGESTURES                              0x00000001

#define GC_ZOOM                                     0x00000001

#define GC_PAN                                      0x00000001
#define GC_PAN_WITH_SINGLE_FINGER_VERTICALLY        0x00000002
#define GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY      0x00000004
#define GC_PAN_WITH_GUTTER                          0x00000008
#define GC_PAN_WITH_INERTIA                         0x00000010

#define GC_ROTATE                                   0x00000001

#define GC_TWOFINGERTAP                             0x00000001

#define GC_PRESSANDTAP                              0x00000001




#define GID_BEGIN                       1
#define GID_END                         2
#define GID_ZOOM                        3
#define GID_PAN                         4
#define GID_ROTATE                      5
#define GID_TWOFINGERTAP                6
#define GID_PRESSANDTAP                 7



#define GESTURECONFIGMAXCOUNT           256



#define GCF_INCLUDE_ANCESTORS           0x00000001


#define WM_TABLET_QUERYSYSTEMGESTURESTATUS 0x02CC
#define WM_GESTURE                         0x0119
#define WM_GESTURENOTIFY                   0x011A


#define TABLET_ROTATE_GESTURE_ENABLE    0x02000000

#endif 

#ifndef HTOUCHINPUT 

typedef struct _TOUCHINPUT {
  LONG      x;
  LONG      y;
  HANDLE    hSource;
  DWORD     dwID;
  DWORD     dwFlags;
  DWORD     dwMask;
  DWORD     dwTime;
  ULONG_PTR dwExtraInfo;
  DWORD     cxContact;
  DWORD     cyContact;
} TOUCHINPUT, *PTOUCHINPUT;

typedef HANDLE HTOUCHINPUT;

#define WM_TOUCH 0x0240

#define TOUCHEVENTF_MOVE       0x0001
#define TOUCHEVENTF_DOWN       0x0002
#define TOUCHEVENTF_UP         0x0004
#define TOUCHEVENTF_INRANGE    0x0008
#define TOUCHEVENTF_PRIMARY    0x0010
#define TOUCHEVENTF_NOCOALESCE 0x0020
#define TOUCHEVENTF_PEN        0x0040
#define TOUCHEVENTF_PALM       0x0080

#define TOUCHINPUTMASKF_TIMEFROMSYSTEM 0x0001
#define TOUCHINPUTMASKF_EXTRAINFO      0x0002
#define TOUCHINPUTMASKF_CONTACTAREA    0x0004

#define TOUCH_COORD_TO_PIXEL(C) (C/100)

#define TWF_FINETOUCH          0x0001
#define TWF_WANTPALM           0x0002

#endif 

class nsPointWin : public nsIntPoint
{
public:
   nsPointWin& operator=(const POINTS& aPoint) {
     x = aPoint.x; y = aPoint.y;
     return *this;
   }
   nsPointWin& operator=(const POINT& aPoint) {
     x = aPoint.x; y = aPoint.y;
     return *this;
   }
   nsPointWin& operator=(int val) {
     x = y = val;
     return *this;
   }
   void ScreenToClient(HWND hWnd) {
     POINT tmp;
     tmp.x = x; tmp.y = y;
     ::ScreenToClient(hWnd, &tmp);
     *this = tmp;
   }
};

class nsWinGesture
{
public:
  nsWinGesture();

public:
  bool SetWinGestureSupport(HWND hWnd, nsGestureNotifyEvent::ePanDirection aDirection);
  bool ShutdownWinGestureSupport();
  bool RegisterTouchWindow(HWND hWnd);
  bool UnregisterTouchWindow(HWND hWnd);
  bool GetTouchInputInfo(HTOUCHINPUT hTouchInput, PRUint32 cInputs, PTOUCHINPUT pInputs);
  bool CloseTouchInputHandle(HTOUCHINPUT hTouchInput);
  bool IsAvailable();
  
  
  bool ProcessGestureMessage(HWND hWnd, WPARAM wParam, LPARAM lParam, nsSimpleGestureEvent& evt);

  
  bool IsPanEvent(LPARAM lParam);
  bool ProcessPanMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);
  bool PanDeltaToPixelScrollX(nsMouseScrollEvent& evt);
  bool PanDeltaToPixelScrollY(nsMouseScrollEvent& evt);
  void UpdatePanFeedbackX(HWND hWnd, PRInt32 scrollOverflow, bool& endFeedback);
  void UpdatePanFeedbackY(HWND hWnd, PRInt32 scrollOverflow, bool& endFeedback);
  void PanFeedbackFinalize(HWND hWnd, bool endFeedback);
  
public:
  
  bool GetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
  bool CloseGestureInfoHandle(HGESTUREINFO hGestureInfo);
  bool GetGestureExtraArgs(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs);
  bool SetGestureConfig(HWND hWnd, UINT cIDs, PGESTURECONFIG pGestureConfig);
  bool GetGestureConfig(HWND hWnd, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig);
  bool BeginPanningFeedback(HWND hWnd);
  bool EndPanningFeedback(HWND hWnd);
  bool UpdatePanningFeedback(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia);

protected:

private:
  
  typedef BOOL (WINAPI * GetGestureInfoPtr)(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
  typedef BOOL (WINAPI * CloseGestureInfoHandlePtr)(HGESTUREINFO hGestureInfo);
  typedef BOOL (WINAPI * GetGestureExtraArgsPtr)(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs);
  typedef BOOL (WINAPI * SetGestureConfigPtr)(HWND hwnd, DWORD dwReserved, UINT cIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
  typedef BOOL (WINAPI * GetGestureConfigPtr)(HWND hwnd, DWORD dwReserved, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
  typedef BOOL (WINAPI * BeginPanningFeedbackPtr)(HWND hWnd);
  typedef BOOL (WINAPI * EndPanningFeedbackPtr)(HWND hWnd, BOOL fAnimateBack);
  typedef BOOL (WINAPI * UpdatePanningFeedbackPtr)(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia);
  typedef BOOL (WINAPI * RegisterTouchWindowPtr)(HWND hWnd, ULONG flags);
  typedef BOOL (WINAPI * UnregisterTouchWindowPtr)(HWND hWnd);
  typedef BOOL (WINAPI * GetTouchInputInfoPtr)(HTOUCHINPUT hTouchInput, PRUint32 cInputs, PTOUCHINPUT pInputs, PRInt32 cbSize);
  typedef BOOL (WINAPI * CloseTouchInputHandlePtr)(HTOUCHINPUT hTouchInput);

  
  static GetGestureInfoPtr getGestureInfo;
  static CloseGestureInfoHandlePtr closeGestureInfoHandle;
  static GetGestureExtraArgsPtr getGestureExtraArgs;
  static SetGestureConfigPtr setGestureConfig;
  static GetGestureConfigPtr getGestureConfig;
  static BeginPanningFeedbackPtr beginPanningFeedback;
  static EndPanningFeedbackPtr endPanningFeedback;
  static UpdatePanningFeedbackPtr updatePanningFeedback;
  static RegisterTouchWindowPtr registerTouchWindow;
  static UnregisterTouchWindowPtr unregisterTouchWindow;
  static GetTouchInputInfoPtr getTouchInputInfo;
  static CloseTouchInputHandlePtr closeTouchInputHandle;

  
  bool InitLibrary();

  static HMODULE sLibraryHandle;
  static const PRUnichar kGestureLibraryName[];

  
  nsPointWin mPanIntermediate;
  nsPointWin mPanRefPoint;
  nsPointWin mPixelScrollDelta;
  bool mPanActive;
  bool mFeedbackActive;
  bool mXAxisFeedback;
  bool mYAxisFeedback;
  bool mPanInertiaActive;
  nsPointWin mPixelScrollOverflow;

  
  double mZoomIntermediate;

  
  double mRotateIntermediate;
};

#endif 


