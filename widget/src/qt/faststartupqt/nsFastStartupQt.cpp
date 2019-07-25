





































#include <QtGui/QApplication>
#include "nsQAppInstance.h"
#include <QtOpenGL/QGLWidget>
#include <QThread>
#if defined MOZ_ENABLE_MEEGOTOUCH
#include <MScene>
#endif
#include "moziqwidget.h"
#include "mozqwidgetfast.h"
#include "nsFastStartupQt.h"
#include "nsXPCOMGlue.h"
#include "nsXULAppAPI.h"


static nsFastStartup* sFastStartup = NULL;

void
GeckoThread::run()
{
  emit symbolsLoadingFinished(mFunc(mExecPath));
}

void
nsFastStartup::symbolsLoadingFinished(bool preloaded)
{
  mSymbolsLoaded = preloaded;
  if (mWidgetPainted && mSymbolsLoaded) {
    qApp->quit();
  }
}

void nsFastStartup::painted()
{
  mWidgetPainted = true;
  if (mWidgetPainted && mSymbolsLoaded) {
    qApp->quit();
  }
}

MozGraphicsView*
nsFastStartup::GetStartupGraphicsView(QWidget* parentWidget, IMozQWidget* aTopChild)
{
  MozGraphicsView* view = NULL;
  if (sFastStartup && sFastStartup->mGraphicsView) {
    view = sFastStartup->mGraphicsView;
  } else {
    view = new MozGraphicsView(parentWidget);
    Qt::WindowFlags flags = Qt::Widget;
    view->setWindowFlags(flags);
#if MOZ_PLATFORM_MAEMO == 6
    view->setViewport(new QGLWidget());
#endif
  }
  view->SetTopLevel(aTopChild, parentWidget);

  return view;
}

nsFastStartup*
nsFastStartup::GetSingleton()
{
  return sFastStartup;
}

nsFastStartup::nsFastStartup()
 : mGraphicsView(0)
 , mFakeWidget(0)
 , mSymbolsLoaded(false)
 , mWidgetPainted(false)
 , mThread(0)

{
  sFastStartup = this;
}

nsFastStartup::~nsFastStartup()
{
  nsQAppInstance::Release();
  sFastStartup = 0;
}

void
nsFastStartup::RemoveFakeLayout()
{
  if (mFakeWidget && mGraphicsView) {
    mGraphicsView->scene()->removeItem(mFakeWidget);
    mFakeWidget->deleteLater();
    mFakeWidget = 0;
    
    mGraphicsView = 0;
  }
}

bool
nsFastStartup::CreateFastStartup(int& argc, char ** argv,
                                 const char* execPath,
                                 GeckoLoaderFunc aFunc)
{
  gArgc = argc;
  gArgv = argv;
  
  nsQAppInstance::AddRef(argc, argv, true);
  
  mThread = new GeckoThread();
  
  connect(mThread, SIGNAL(symbolsLoadingFinished(bool)),
          this, SLOT(symbolsLoadingFinished(bool)));
  mThread->SetLoader(aFunc, execPath);
  
  IMozQWidget* fakeWidget = new MozQWidgetFast(NULL, NULL);
  mGraphicsView = GetStartupGraphicsView(NULL, fakeWidget);
  mFakeWidget = fakeWidget;

  mThread->start();
#ifdef MOZ_PLATFORM_MAEMO
  mGraphicsView->showFullScreen();
#else
  mGraphicsView->showNormal();
#endif

  
  
  
  qApp->exec();

  return true;
}
