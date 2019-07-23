




































#ifndef nsGtkIMEHelper_h__
#define nsGtkIMEHelper_h__
#include "nsIUnicodeDecoder.h"
#include "nsString.h"
#include <gtk/gtk.h>




class nsGtkIMEHelper {
public:
  ~nsGtkIMEHelper();
  nsresult ConvertToUnicode(const char* aSrc, PRInt32* aSrcLen,
                            PRUnichar*  aDes, PRInt32* aDesLen);
  static nsGtkIMEHelper *GetSingleton();
  void ResetDecoder();
  static void Shutdown();
  PRInt32 MultiByteToUnicode(const char*, const PRInt32,
                             PRUnichar**, PRInt32*);
#if defined(USE_XIM) && defined(_AIX)
  PRUnichar* GetUnichars() const { return mUnichars; }
  PRInt32 GetUnicharsSize() const { return mUnicharsSize; }

  
  
  void SetUnichars(PRUnichar *aUnichars) { mUnichars = aUnichars; }
  void SetUnicharsSize(PRInt32 aUnicharsSize)
    { mUnicharsSize = aUnicharsSize; }
#endif

private:
  nsGtkIMEHelper();
  nsIUnicodeDecoder* mDecoder;
  void SetupUnicodeDecoder();
  static nsGtkIMEHelper *gSingleton;
#if defined(USE_XIM) && defined(_AIX)
  PRUnichar* mUnichars;
  PRInt32 mUnicharsSize;
#endif
};

#ifdef USE_XIM
class nsIMEPreedit {
 private:
  PRInt32       mCaretPosition;
  nsString*     mIMECompUnicode;
  nsCString*    mIMECompAttr;
  PRUnichar*    mCompositionUniString;
  PRInt32       mCompositionUniStringSize;
 public:
  nsIMEPreedit();
  ~nsIMEPreedit();
  void Reset();
  const PRUnichar* GetPreeditString() const;
  const char* GetPreeditFeedback() const;
  int GetPreeditLength() const;
  void SetPreeditString(const XIMText* aText,
                        const PRInt32 aChangeFirst,
                        const PRInt32 aChangeLength);
  static void IMSetTextRange(const PRInt32 aLen,
                             const char *aFeedback,
                             PRUint32 *,
                             nsTextRangeArray*);
};

class nsWindow;

class nsIMEStatus {
 private:
  Window mIMStatusWindow;
  Window mIMStatusLabel;  
  XFontSet mFontset;
  int mWidth;
  int mHeight;
  GC mGC;
  void resize(const char *);
  void remove_decoration();
  void getAtoms();
  static Bool client_filter(Display *d, Window w, XEvent *ev,
                               XPointer client_data);
  static Bool repaint_filter(Display *d, Window w, XEvent *ev,
                             XPointer client_data);
  static Bool clientmessage_filter(Display *d, Window w, XEvent *ev,
                                   XPointer client_data);
  void CreateNative();
  void DestroyNative();
 public:
  nsIMEStatus();
  nsIMEStatus(GdkFont*);
  void SetFont(GdkFont*);
  ~nsIMEStatus();
  void UnregisterClientFilter(Window);
  void RegisterClientFilter(Window);
  void setText(const char*);
  void setParentWindow(nsWindow*);
  void resetParentWindow(nsWindow*);
  void show();
  void hide();

  nsWindow *mAttachedWindow;
  GdkWindow *mParent;
};


typedef int (*XIMProc1)(XIC, XPointer, XPointer);
typedef struct {
  XPointer client_data;
  XIMProc1 callback;
} XIMCallback1;

class nsIMEGtkIC {
 private:
  static int preedit_start_cbproc(XIC, XPointer, XPointer);
  static int preedit_draw_cbproc(XIC, XPointer, XPointer);
  static int preedit_done_cbproc(XIC, XPointer, XPointer);
  static int preedit_caret_cbproc(XIC, XPointer, XPointer) { return 0; };
  static int status_draw_cbproc(XIC, XPointer, XPointer);
  static int status_start_cbproc(XIC, XPointer, XPointer) { return 0; };
  static int status_done_cbproc(XIC, XPointer, XPointer) { return 0; };
  static nsIMEStatus *gStatus;
  nsWindow *mClientWindow;
  nsWindow *mFocusWindow;
  static nsWindow *gGlobalFocusWindow;
  nsIMEGtkIC(nsWindow*, GdkFont*, GdkFont*);
  nsIMEGtkIC(nsWindow*, GdkFont*);
  GdkICPrivate *mIC;
  GdkICPrivate *mIC_backup;
  nsIMEPreedit *mPreedit;
  GdkFont      *mStatusFontset;

 public:
  nsIMEPreedit *GetPreedit() {return mPreedit;}
  ~nsIMEGtkIC();
  static nsIMEGtkIC *GetXIC(nsWindow*, GdkFont*, GdkFont*);
  static nsIMEGtkIC *GetXIC(nsWindow*, GdkFont*);
  void SetFocusWindow(nsWindow * aFocusWindow);
  nsWindow* GetFocusWindow();
  nsWindow* GetGlobalFocusWindow();
  static void UnsetFocusWindow();
  static GdkIMStyle GetInputStyle();

  GdkIMStyle mInputStyle;
  char *mStatusText;
  void SetStatusText(const char*);
  void SetPreeditFont(GdkFont*);
  void SetStatusFont(GdkFont*);
  void SetPreeditSpotLocation(unsigned long, unsigned long);
  void SetPreeditArea(int, int, int, int);
  void ResetStatusWindow(nsWindow * aWindow);
  PRBool IsPreeditComposing();
  PRInt32 ResetIC(PRUnichar **aUnichar, PRInt32 *aUnisize);
};
#endif 
#endif 
