








































#include <QWidget>
#include "nsMFilePicker.h"



#include <QtSparql/QSparqlConnection>
#include <QtSparql/QSparqlQuery>
#include <QtSparql/QSparqlResult>

#include <MApplication>
#include <MApplicationPage>
#include <MApplicationWindow>
#include <MSceneManager>

#include <QFileInfo>
#include <QPointer>

#include <SelectSingleContentItemPage.h>
#include <SelectMultipleContentItemsPage.h>

MeegoFileDialog::MeegoFileDialog(QObject* aParent)
  : QObject(aParent)
  , mMode(Mode_Unknown)
{
}

MeegoFileDialog::~MeegoFileDialog()
{
}

int
MeegoFileDialog::exec()
{
    MApplicationPage* page = NULL;
    switch(mMode) {
    case Mode_OpenFile:
        page = createOpenFilePage();
        break;
    case Mode_OpenFiles:
        page = createOpenFilesPage();
        break;
    case Mode_OpenDirectory:
        page = createOpenDirectoryPage();
        break;
    default:
        return 0;
    }

    page->setTitle(mCaption);

    QPointer<MApplicationWindow> appWindow = new MApplicationWindow(MApplication::activeWindow());
    QObject::connect(MApplication::activeWindow(), SIGNAL(switcherEntered()), this, SLOT(backButtonClicked()));
    appWindow->show();

    
    
    MSceneWindow* fakeWindow = new MSceneWindow();
    
    connect(page, SIGNAL(backButtonClicked()), fakeWindow, SLOT(disappear()));
    
    appWindow->sceneManager()->appearSceneWindowNow(page);

    
    QList<MSceneWindow*> sceneWindowHistory = appWindow->sceneManager()->pageHistory();
    sceneWindowHistory.insert(0, fakeWindow);
    appWindow->sceneManager()->setPageHistory(sceneWindowHistory);

    
    QEventLoop eventLoop;
    mEventLoop = &eventLoop;
    QPointer<MeegoFileDialog> guard = this;
    (void) eventLoop.exec(QEventLoop::DialogExec);
    if (guard.isNull()) {
        return 0;
    }
    if (page) {
        page->disappear();
        delete page;
    }
    if (appWindow) {
        delete appWindow;
    }
    mEventLoop = 0;
    return 0;
}

MApplicationPage*
MeegoFileDialog::createOpenFilePage()
{
    QStringList itemType("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#FileDataObject");

    SelectSingleContentItemPage* page =
        new SelectSingleContentItemPage(QString(),
                                        itemType);

    connect(page, SIGNAL(contentItemSelected(const QString&)),
            this, SLOT  (contentItemSelected(const QString&)));

    connect(page, SIGNAL(backButtonClicked()),
            this, SLOT  (backButtonClicked()));

    return page;
}

MApplicationPage*
MeegoFileDialog::createOpenFilesPage()
{
    QStringList itemType("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Folder");

    SelectMultipleContentItemsPage* page = new SelectMultipleContentItemsPage(QString(), itemType);
    connect(page, SIGNAL(contentItemsSelected(const QStringList&)),
            this, SLOT  (contentItemsSelected(const QStringList&)));

    connect(page, SIGNAL(backButtonClicked()),
            this, SLOT  (backButtonClicked()));

    return page;
}

MApplicationPage*
MeegoFileDialog::createOpenDirectoryPage()
{
    QStringList itemType("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Folder");

    SelectSingleContentItemPage* page = new SelectSingleContentItemPage(QString(), itemType);
    connect(page, SIGNAL(contentItemSelected(const QString&)),
            this, SLOT  (contentItemSelected(const QString&)));

    connect(page, SIGNAL(backButtonClicked()),
            this, SLOT  (backButtonClicked()));

    return page;
}

QStringList
MeegoFileDialog::selectedFileNames() const
{
    return mSelectedFileNames;
}

bool
MeegoFileDialog::hasSelectedFileNames() const
{
    return !mSelectedFileNames.isEmpty();
}

void
MeegoFileDialog::contentItemSelected(const QString& aContentItem)
{
    mSelectedFileNames.clear();

    if (aContentItem.isEmpty()) {
        finishEventLoop();
        return;
    }

    QUrl fileUrl = trackerIdToFilename(aContentItem);
    QFileInfo fileInfo(fileUrl.toLocalFile());
    if (fileInfo.isFile()) {
        mSelectedFileNames << fileInfo.canonicalFilePath();
    }

    finishEventLoop();
}

void
MeegoFileDialog::contentItemsSelected(const QStringList& aContentItems)
{
    mSelectedFileNames.clear();

    foreach (QString contentItem, aContentItems) {
        QUrl fileUrl = trackerIdToFilename(contentItem);
        QFileInfo fileInfo(fileUrl.toLocalFile());
        if (fileInfo.isFile()) {
            mSelectedFileNames << fileInfo.canonicalFilePath();
        }
    }

    finishEventLoop();
}

void
MeegoFileDialog::backButtonClicked()
{
    mSelectedFileNames.clear();
    finishEventLoop();
}

 QString
MeegoFileDialog::getOpenFileName(QWidget* parent,
                                 const QString& caption,
                                 const QString& dir,
                                 const QString& filter)
{
    QPointer<MeegoFileDialog> picker = new MeegoFileDialog(parent);

    QString result;
    picker->mMode = MeegoFileDialog::Mode_OpenFile;
    picker->mCaption = caption;
    picker->exec();

    
    if (picker) {
        if (picker->hasSelectedFileNames()) {
            result = picker->selectedFileNames().first();
        }
        delete picker;
    }

    return result;
}

 QString
MeegoFileDialog::getExistingDirectory(QWidget* parent,const QString& caption, const QString& dir)
{
    QPointer<MeegoFileDialog> picker = new MeegoFileDialog(parent);

    QString result;
    picker->mMode = MeegoFileDialog::Mode_OpenDirectory;
    picker->mCaption = caption;
    picker->exec();

    
    if (picker) {
        if (picker->hasSelectedFileNames()) {
            result = picker->selectedFileNames().first();
        }
        delete picker;
    }

    return result;
}

 QStringList
MeegoFileDialog::getOpenFileNames(QWidget* parent,
                                  const QString& caption,
                                  const QString& dir,
                                  const QString& filter)
{
    QPointer<MeegoFileDialog> picker = new MeegoFileDialog(parent);

    QStringList result;
    picker->mMode = MeegoFileDialog::Mode_OpenFiles;
    picker->mCaption = caption;
    picker->exec();

    
    if (picker) {
        if (picker->hasSelectedFileNames()) {
            result = picker->selectedFileNames();
        }
        delete picker;
    }

    return result;
}

QUrl
MeegoFileDialog::trackerIdToFilename(const QString& trackerId)
{
    QSparqlQuery query("SELECT ?u WHERE { ?:tUri nie:url ?u . }");
    query.bindValue("tUri", QUrl(trackerId)); 
    QSparqlConnection connection("QTRACKER"); 
    QSparqlResult* result = connection.exec(query);

    result->waitForFinished();
    result->first();

    QUrl resultFile;
    if (result->isValid()) {
        resultFile = QUrl(result->binding(0).value().toString());
    }
    delete result;

    return resultFile;
}
