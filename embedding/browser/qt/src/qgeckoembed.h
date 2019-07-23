






































#ifndef QECKOEMBED_H
#define QECKOEMBED_H

#include <qwidget.h>

class nsIDOMKeyEvent;
class nsIDOMMouseEvent;
class nsIDOMUIEvent;
class nsModuleComponentInfo;
class nsIDirectoryServiceProvider;
class nsIAppShell;
class nsVoidArray;
class nsProfileDirServiceProvider;
class nsISupports;
class EmbedWindow;
class EmbedEventListener;
class EmbedProgress;
class nsIWebNavigation;
class nsISHistory;
class EmbedContentListener;
class EmbedStream;
class QHBox;
class nsIDOMDocument;
class nsPIDOMWindow;
class QPaintEvent;

class QGeckoEmbedPrivate;

class QGeckoEmbed : public QWidget
{
    Q_OBJECT
public:
    static void initialize(const char *aDir, const char *aName);
public:
    enum ReloadFlags
    {
        Normal,
        BypassCache,
        BypassProxy,
        BypassProxyAndCache,
        CharsetChange
    };
public:
    QGeckoEmbed(QWidget *parent, const char *name);
    ~QGeckoEmbed();

    bool canGoBack() const;
    bool canGoForward() const;

    void setIsChrome(bool);
    int chromeMask() const;

    nsIDOMDocument *document() const;
    QString url() const;
    QString resolvedUrl(const QString &relativepath) const;

public slots:
    void loadURL(const QString &url);
    void stopLoad();
    void goForward();
    void goBack();

    void renderData(const QCString &data, const QString &baseURI,
                    const QString &mimeType);

    int  openStream(const QString &baseURI, const QString &mimeType);
    int  appendData(const QCString &data);
    int  closeStream();

    void reload(ReloadFlags flags = Normal);

    void setChromeMask(int);

signals:
    void linkMessage(const QString &message);
    void jsStatusMessage(const QString &message);
    void locationChanged(const QString &location);
    void windowTitleChanged(const QString &title);

    void progress(int current, int max);
    void progressAll(const QString &url, int current, int max);

    void netState(int state, int status);
    void netStateAll(const QString &url, int state, int status);

    void netStart();
    void netStop();

    void newWindow(QGeckoEmbed *newWindow, int chromeMask);
    void visibilityChanged(bool visible);
    void destroyBrowser();
    void openURI(const QString &url);
    void sizeTo(int width, int height);

    void securityChange(void *request, int status, void *message);
    void statusChange(void *request, int status, void *message);

    void showContextMenu(const QPoint &p, const QString &url);

    



    void domKeyDown(nsIDOMKeyEvent *keyEvent);
    void domKeyPress(nsIDOMKeyEvent *keyEvent);
    void domKeyUp(nsIDOMKeyEvent *keyEvent);
    void domMouseDown(nsIDOMMouseEvent *mouseEvent);
    void domMouseUp(nsIDOMMouseEvent *mouseEvent);
    void domMouseClick(nsIDOMMouseEvent *mouseEvent);
    void domMouseDblClick(nsIDOMMouseEvent *mouseEvent);
    void domMouseOver(nsIDOMMouseEvent *mouseEvent);
    void domMouseOut(nsIDOMMouseEvent *mouseEvent);
    void domActivate(nsIDOMUIEvent *event);
    void domFocusIn(nsIDOMUIEvent *event);
    void domFocusOut(nsIDOMUIEvent *event);


    void startURIOpen(const QString &url, bool &abort);

protected:
    friend class EmbedEventListener;
    friend class EmbedContentListener;
    





    virtual bool domKeyDownEvent(nsIDOMKeyEvent *keyEvent);
    virtual bool domKeyPressEvent(nsIDOMKeyEvent *keyEvent);
    virtual bool domKeyUpEvent(nsIDOMKeyEvent *keyEvent);

    virtual bool domMouseDownEvent(nsIDOMMouseEvent *mouseEvent);
    virtual bool domMouseUpEvent(nsIDOMMouseEvent *mouseEvent);
    virtual bool domMouseClickEvent(nsIDOMMouseEvent *mouseEvent);
    virtual bool domMouseDblClickEvent(nsIDOMMouseEvent *mouseEvent);
    virtual bool domMouseOverEvent(nsIDOMMouseEvent *mouseEvent);
    virtual bool domMouseOutEvent(nsIDOMMouseEvent *mouseEvent);

    virtual bool domActivateEvent(nsIDOMUIEvent *event);
    virtual bool domFocusInEvent(nsIDOMUIEvent *event);
    virtual bool domFocusOutEvent(nsIDOMUIEvent *event);


protected:
    friend class EmbedWindow;
    friend class EmbedWindowCreator;
    friend class EmbedProgress;
    friend class EmbedContextMenuListener;
    friend class EmbedStream;
    friend class QGeckoGlobals;
    void emitScriptStatus(const QString &str);
    void emitLinkStatus(const QString &str);
    void contentStateChanged();
    void contentFinishedLoading();

    bool isChrome() const;
    bool chromeLoaded() const;

protected:
    void resizeEvent(QResizeEvent *e);

    void setupListener();
    void attachListeners();

    EmbedWindow *window() const;
    int GetPIDOMWindow(nsPIDOMWindow **aPIWin);

protected:
    QGeckoEmbedPrivate *d;
};

#endif
