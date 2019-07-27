




#ifndef _GTK_NSNATIVETHEMEGTK_H_
#define _GTK_NSNATIVETHEMEGTK_H_

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIObserver.h"
#include "nsNativeTheme.h"

#include <gtk/gtk.h>
#include "gtkdrawing.h"

class nsNativeThemeGTK: private nsNativeTheme,
                        public nsITheme,
                        public nsIObserver {
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD DrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame, uint8_t aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect) MOZ_OVERRIDE;

  NS_IMETHOD GetWidgetBorder(nsDeviceContext* aContext, nsIFrame* aFrame,
                             uint8_t aWidgetType,
                             nsIntMargin* aResult) MOZ_OVERRIDE;

  virtual bool GetWidgetPadding(nsDeviceContext* aContext,
                                nsIFrame* aFrame,
                                uint8_t aWidgetType,
                                nsIntMargin* aResult) MOZ_OVERRIDE;

  virtual bool GetWidgetOverflow(nsDeviceContext* aContext,
                                 nsIFrame* aFrame,
                                 uint8_t aWidgetType,
                                 nsRect* aOverflowRect) MOZ_OVERRIDE;

  NS_IMETHOD GetMinimumWidgetSize(nsPresContext* aPresContext,
                                  nsIFrame* aFrame, uint8_t aWidgetType,
                                  nsIntSize* aResult,
                                  bool* aIsOverridable) MOZ_OVERRIDE;

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, uint8_t aWidgetType, 
                                nsIAtom* aAttribute,
                                bool* aShouldRepaint) MOZ_OVERRIDE;

  NS_IMETHOD ThemeChanged() MOZ_OVERRIDE;

  NS_IMETHOD_(bool) ThemeSupportsWidget(nsPresContext* aPresContext,
                                        nsIFrame* aFrame,
                                        uint8_t aWidgetType) MOZ_OVERRIDE;

  NS_IMETHOD_(bool) WidgetIsContainer(uint8_t aWidgetType) MOZ_OVERRIDE;
  
  NS_IMETHOD_(bool) ThemeDrawsFocusForWidget(uint8_t aWidgetType) MOZ_OVERRIDE;

  virtual bool ThemeNeedsComboboxDropmarker() MOZ_OVERRIDE;

  virtual Transparency GetWidgetTransparency(nsIFrame* aFrame,
                                             uint8_t aWidgetType) MOZ_OVERRIDE;

  nsNativeThemeGTK();

protected:
  virtual ~nsNativeThemeGTK();

private:
  gint GetTabMarginPixels(nsIFrame* aFrame);
  bool GetGtkWidgetAndState(uint8_t aWidgetType, nsIFrame* aFrame,
                              GtkThemeWidgetType& aGtkWidgetType,
                              GtkWidgetState* aState, gint* aWidgetFlags);
  bool GetExtraSizeForWidget(nsIFrame* aFrame, uint8_t aWidgetType,
                               nsIntMargin* aExtra);

  void RefreshWidgetWindow(nsIFrame* aFrame);

  uint8_t mDisabledWidgetTypes[32];
  uint8_t mSafeWidgetStates[1024];    
  static const char* sDisabledEngines[];
};

#endif
