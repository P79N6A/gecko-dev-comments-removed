





#ifdef MOZ_WIDGET_QT
#include <QDesktopServices>
#include <QUrl>
#include <QString>
#include <QStringList>
#endif

#include "nsMIMEInfoQt.h"
#include "nsIURI.h"
#include "nsStringGlue.h"

nsresult
nsMIMEInfoQt::LoadUriInternal(nsIURI * aURI)
{
#ifdef MOZ_WIDGET_QT
  nsAutoCString spec;
  aURI->GetAsciiSpec(spec);
  if (QDesktopServices::openUrl(QUrl(spec.get()))) {
    return NS_OK;
  }
#endif

  return NS_ERROR_FAILURE;
}
