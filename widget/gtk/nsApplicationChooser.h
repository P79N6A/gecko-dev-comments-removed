




#ifndef nsApplicationChooser_h__
#define nsApplicationChooser_h__

#include <gtk/gtk.h>
#include "nsIApplicationChooser.h"

class nsApplicationChooser : public nsIApplicationChooser
{
public:
  nsApplicationChooser();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONCHOOSER
  void Done(GtkWidget* chooser, gint response);

private:
  ~nsApplicationChooser();
  nsCOMPtr<nsIWidget> mParentWidget;
  nsCString mWindowTitle;
  nsCOMPtr<nsIApplicationChooserFinishedCallback> mCallback;
  static void OnResponse(GtkWidget* chooser, gint response_id, gpointer user_data);
  static void OnDestroy(GtkWidget* chooser, gpointer user_data);
};
#endif
