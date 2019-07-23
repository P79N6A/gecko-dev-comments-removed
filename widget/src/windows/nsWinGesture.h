





































#ifndef WinGesture_h__
#define WinGesture_h__





#include "nsdefs.h"
#include <winuser.h>
#include "nsPoint.h"
#include "nsGUIEvent.h"

#if !defined(NTDDI_WIN7) ||  NTDDI_VERSION < NTDDI_WIN7

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
  PRBool SetWinGestureSupport(HWND hWnd, nsGestureNotifyEvent::ePanDirection aDirection);
  PRBool ShutdownWinGestureSupport();
  PRBool IsAvailable();
  
  
  PRBool ProcessGestureMessage(HWND hWnd, WPARAM wParam, LPARAM lParam, nsSimpleGestureEvent& evt);

  
  PRBool IsPanEvent(LPARAM lParam);
  PRBool ProcessPanMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);
  PRBool PanDeltaToPixelScrollX(nsMouseScrollEvent& evt);
  PRBool PanDeltaToPixelScrollY(nsMouseScrollEvent& evt);
  void UpdatePanFeedbackX(HWND hWnd, PRInt32 scrollOverflow, PRBool& endFeedback);
  void UpdatePanFeedbackY(HWND hWnd, PRInt32 scrollOverflow, PRBool& endFeedback);
  void PanFeedbackFinalize(HWND hWnd, PRBool endFeedback);
  
public:
  
  PRBool GetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
  PRBool CloseGestureInfoHandle(HGESTUREINFO hGestureInfo);
  PRBool GetGestureExtraArgs(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs);
  PRBool SetGestureConfig(HWND hWnd, UINT cIDs, PGESTURECONFIG pGestureConfig);
  PRBool GetGestureConfig(HWND hWnd, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig);
  PRBool BeginPanningFeedback(HWND hWnd);
  PRBool EndPanningFeedback(HWND hWnd);
  PRBool UpdatePanningFeedback(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia);

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

  
  static GetGestureInfoPtr getGestureInfo;
  static CloseGestureInfoHandlePtr closeGestureInfoHandle;
  static GetGestureExtraArgsPtr getGestureExtraArgs;
  static SetGestureConfigPtr setGestureConfig;
  static GetGestureConfigPtr getGestureConfig;
  static BeginPanningFeedbackPtr beginPanningFeedback;
  static EndPanningFeedbackPtr endPanningFeedback;
  static UpdatePanningFeedbackPtr updatePanningFeedback;

  
  PRBool InitLibrary();

  static HMODULE sLibraryHandle;
  static const PRUnichar kGestureLibraryName[];
  static const PRUnichar kThemeLibraryName[];

  
  nsPointWin mPanIntermediate;
  nsPointWin mPanRefPoint;
  nsPointWin mPixelScrollDelta;
  PRPackedBool mPanActive;
  PRPackedBool mFeedbackActive;
  PRPackedBool mXAxisFeedback;
  PRPackedBool mYAxisFeedback;
  PRPackedBool mPanInertiaActive;
  nsPointWin mPixelScrollOverflow;

  
  double mZoomIntermediate;

  
  double mRotateIntermediate;
};

#endif 


