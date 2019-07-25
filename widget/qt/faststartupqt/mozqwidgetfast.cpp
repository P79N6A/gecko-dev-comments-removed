





































#include <QtCore/QUrl>
#include "mozqwidgetfast.h"
#include "nsFastStartupQt.h"
#include "nsILocalFile.h"
#include "BinaryPath.h"

#define TOOLBAR_SPLASH "toolbar_splash.png"
#define FAVICON_SPLASH "favicon32.png"
#define DRAWABLE_PATH "res/drawable/"

MozQWidgetFast::MozQWidgetFast(nsWindow* aReceiver, QGraphicsItem* aParent)
{
  setParentItem(aParent);
  char exePath[MAXPATHLEN];
  QStringList arguments = qApp->arguments();
  nsresult rv =
    mozilla::BinaryPath::Get(arguments.at(0).toLocal8Bit().constData(),
                             exePath);
  if (NS_FAILED(rv)) {
    printf("Cannot read default path\n");
    return;
  }
  char *lastSlash = strrchr(exePath, XPCOM_FILE_PATH_SEPARATOR[0]);
  if (!lastSlash ||
      (lastSlash - exePath > int(MAXPATHLEN - sizeof(XPCOM_DLL) - 1))) {
     return;
  }
  strcpy(++lastSlash, "/");
  QString resourcePath(QString((const char*)&exePath) + DRAWABLE_PATH);
  mToolbar.load(resourcePath + TOOLBAR_SPLASH);
  mIcon.load(resourcePath + FAVICON_SPLASH);
  for (int i = 1; i < arguments.size(); i++) {
    QUrl url = QUrl::fromUserInput(arguments.at(i));
    if (url.isValid()) {
      mUrl = url.toString();
    }
  }
}

void MozQWidgetFast::paint(QPainter* aPainter,
                           const QStyleOptionGraphicsItem*,
                           QWidget*)
{
  
  int toolbarHeight = 80;
  
  int faviconOffset = 25;
  
  int faviconSize = 32;
  
  
  
  float toolbarPartWidth = 77;
  
  
  int tileWidth = 2;
  
  aPainter->drawPixmap(QRect(0, 0, toolbarPartWidth, toolbarHeight),
                       mToolbar, QRect(0, 0, toolbarPartWidth, toolbarHeight));

  
  QPixmap tile(tileWidth, toolbarHeight);
  QPainter p(&tile);
  p.drawPixmap(QRect(0, 0, tileWidth, toolbarHeight), mToolbar,
               QRect(toolbarPartWidth, 0, tileWidth, toolbarHeight));
  aPainter->drawTiledPixmap(QRect(toolbarPartWidth, 0, rect().width() - toolbarPartWidth * 2,
                                  toolbarHeight),
                            tile);
  
  aPainter->drawPixmap(QRect(faviconOffset, faviconOffset,
                             faviconSize, faviconSize),
                       mIcon);
  if (!mUrl.isEmpty()) {
    
    float urlHeight = 24.0f;
    
    int urlOffsetX = 80;
    int urlOffsetY = 48;
    QFont font = aPainter->font();
    font.setPixelSize(urlHeight);
    font.setFamily(QString("Nokia Sans"));
    font.setKerning(true);
    aPainter->setFont(font);
    aPainter->setRenderHint(QPainter::TextAntialiasing, true);
    aPainter->drawText(urlOffsetX, urlOffsetY,
                       aPainter->fontMetrics().elidedText(mUrl, Qt::ElideRight, rect().width() - urlOffsetX * 2));
  }

  
  aPainter->drawPixmap(QRect(rect().width() - toolbarPartWidth,
                             0, toolbarPartWidth,
                             toolbarHeight),
                       mToolbar,
                       QRect(mToolbar.width() - toolbarPartWidth, 0,
                             toolbarPartWidth, toolbarHeight));

  nsFastStartup::GetSingleton()->painted();
}
