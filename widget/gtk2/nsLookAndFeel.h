






































#ifndef __nsLookAndFeel
#define __nsLookAndFeel
#include "nsXPLookAndFeel.h"
#include "nsCOMPtr.h"
#include <gtk/gtk.h>

class nsLookAndFeel: public nsXPLookAndFeel {
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual nsresult NativeGetColor(ColorID aID, nscolor &aResult);
    virtual nsresult GetIntImpl(IntID aID, PRInt32 &aResult);
    virtual nsresult GetFloatImpl(FloatID aID, float &aResult);
    virtual void RefreshImpl();
    virtual PRUnichar GetPasswordCharacterImpl();
    virtual bool GetEchoPasswordImpl();

protected:
    GtkStyle *mStyle;

    
    

    static nscolor sInfoBackground;
    static nscolor sInfoText;
    static nscolor sMenuBackground;
    static nscolor sMenuBarText;
    static nscolor sMenuBarHoverText;
    static nscolor sMenuText;
    static nscolor sMenuHover;
    static nscolor sMenuHoverText;
    static nscolor sButtonBackground;
    static nscolor sButtonText;
    static nscolor sButtonOuterLightBorder;
    static nscolor sButtonInnerDarkBorder;
    static nscolor sOddCellBackground;
    static nscolor sNativeHyperLinkText;
    static nscolor sComboBoxText;
    static nscolor sComboBoxBackground;
    static PRUnichar sInvisibleCharacter;
    static float   sCaretRatio;
    static bool    sMenuSupportsDrag;

    static void InitLookAndFeel();
    void InitWidget() {
        NS_ASSERTION(!mStyle, "already initialized");
        
        
        
        
        
        
        
        
        
        
        
        GtkWidget *widget = gtk_invisible_new();
        g_object_ref_sink(widget); 

        gtk_widget_ensure_style(widget);
        mStyle = gtk_style_copy(gtk_widget_get_style(widget));

        gtk_widget_destroy(widget);
        g_object_unref(widget);
    }
};

#endif
