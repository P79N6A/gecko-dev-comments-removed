






































#ifndef NSWINDOWNG_H
#define NSWINDOWNG_H

#include "nsCommonWidget.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"

class nsWindow : public nsCommonWidget,
                 public nsSupportsWeakReference
{
public:
    nsWindow();
    virtual ~nsWindow();


    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD         PreCreateWidget(nsWidgetInitData *aWidgetInitData);

    gfxASurface       *GetThebesSurface();

protected:
    QWidget  *createQWidget(QWidget *parent, nsWidgetInitData *aInitData);
    nsRefPtr<gfxASurface> mThebesSurface;
};

class ChildWindow : public nsWindow
{
public:
  ChildWindow();
  ~ChildWindow();
  virtual PRBool IsChild() const;

  PRInt32 mChildID;
};

class PopupWindow : public nsWindow
{
public:
    PopupWindow();
    ~PopupWindow();

  PRInt32 mChildID;
};

#endif
