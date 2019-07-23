






































#ifndef __nsLookAndFeel
#define __nsLookAndFeel
#include "nsXPLookAndFeel.h"
#include "nsCOMPtr.h"
#include <gtk/gtk.h>

class nsLookAndFeel: public nsXPLookAndFeel {
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    nsresult NativeGetColor(const nsColorID aID, nscolor &aColor);
    NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
    NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);
    NS_IMETHOD LookAndFeelChanged();
    virtual PRUnichar GetPasswordCharacter();

protected:
    GtkStyle *mStyle;
    GtkWidget *mWidget;

    
    

    static nscolor sInfoBackground;
    static nscolor sInfoText;
    static nscolor sMenuBackground;
    static nscolor sMenuText;
    static nscolor sMenuHover;
    static nscolor sMenuHoverText;
    static nscolor sButtonBackground;
    static nscolor sButtonText;
    static nscolor sButtonOuterLightBorder;
    static nscolor sButtonInnerDarkBorder;
    static PRUnichar sInvisibleCharacter;

    static void InitLookAndFeel();
    void InitWidget() {
        mWidget = gtk_invisible_new();
        gtk_object_ref(GTK_OBJECT(mWidget));
        gtk_object_sink(GTK_OBJECT(mWidget));
        gtk_widget_ensure_style(mWidget);
        mStyle = gtk_widget_get_style(mWidget);
    }
};

#endif
