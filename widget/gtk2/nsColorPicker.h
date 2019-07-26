




#ifndef nsColorPicker_h__
#define nsColorPicker_h__

#include <gtk/gtk.h>

#include "nsCOMPtr.h"
#include "nsIColorPicker.h"
#include "nsString.h"

class nsIWidget;

class nsColorPicker MOZ_FINAL : public nsIColorPicker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOLORPICKER

  nsColorPicker() {};

private:
  ~nsColorPicker() {};

  static void OnResponse(GtkWidget* dialog, gint response_id,
                         gpointer user_data);
  static void OnDestroy(GtkWidget* dialog, gpointer user_data);

  
  static int convertGdkColorComponent(guint16 color_component);
  static guint16 convertToGdkColorComponent(int color_component);
  static GdkColor convertToGdkColor(nscolor color);
  static nsString ToHexString(int n);

  void Done(GtkWidget* dialog, gint response_id);
  void ReadValueFromColorChooser(GtkWidget* color_chooser);

  nsCOMPtr<nsIWidget> mParentWidget;
  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
  nsString mTitle;
  nsString mColor;
  GdkColor mDefaultColor;
};

#endif 
