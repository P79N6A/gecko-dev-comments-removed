





































#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIObserver.h"
#include "nsNativeTheme.h"

#include <gtk/gtkwidget.h>
#include "gtkdrawing.h"

class nsNativeThemeGTK: private nsNativeTheme,
                        public nsITheme,
                        public nsIObserver {
public:
  NS_DECL_ISUPPORTS

  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD DrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame, PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);

  NS_IMETHOD GetWidgetBorder(nsIDeviceContext* aContext, nsIFrame* aFrame,
                             PRUint8 aWidgetType, nsMargin* aResult);

  virtual NS_HIDDEN_(PRBool) GetWidgetPadding(nsIDeviceContext* aContext,
                                              nsIFrame* aFrame,
                                              PRUint8 aWidgetType,
                                              nsMargin* aResult);

  virtual NS_HIDDEN_(PRBool) GetWidgetOverflow(nsIDeviceContext* aContext,
                                               nsIFrame* aFrame,
                                               PRUint8 aWidgetType,
                                               nsRect* aResult);

  NS_IMETHOD GetMinimumWidgetSize(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame, PRUint8 aWidgetType,
                                  nsSize* aResult, PRBool* aIsOverridable);

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                nsIAtom* aAttribute, PRBool* aShouldRepaint);

  NS_IMETHOD ThemeChanged();

  NS_IMETHOD_(PRBool) ThemeSupportsWidget(nsPresContext* aPresContext,
                                          nsIFrame* aFrame,
                                          PRUint8 aWidgetType);

  NS_IMETHOD_(PRBool) WidgetIsContainer(PRUint8 aWidgetType);

  nsNativeThemeGTK();
  virtual ~nsNativeThemeGTK();

private:
  PRBool GetGtkWidgetAndState(PRUint8 aWidgetType, nsIFrame* aFrame,
                              GtkThemeWidgetType& aGtkWidgetType,
                              GtkWidgetState* aState, gint* aWidgetFlags);

  void RefreshWidgetWindow(nsIFrame* aFrame);

  PRUint8 mDisabledWidgetTypes[32];
  PRUint8 mSafeWidgetStates[1024];    
  static const char* sDisabledEngines[];
};
