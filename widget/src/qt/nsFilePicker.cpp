







































#include <qfile.h>
#include <qstringlist.h>
#include <qapplication.h>
#include <qgraphicsproxywidget.h>
#include <qgraphicswidget.h>
#include <qgraphicsscene.h>

#include "nsFilePicker.h"

#include "nsILocalFile.h"
#include "nsIURI.h"
#include "nsISupportsArray.h"
#include "nsMemory.h"
#include "nsEnumeratorUtils.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsIWidget.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* sFilePickerLog = nsnull;
#endif


NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

nsFilePicker::nsFilePicker()
    : mDialog(0),
      mMode(nsIFilePicker::modeOpen)
{
#ifdef PR_LOGGING
    if (!sFilePickerLog)
        sFilePickerLog = PR_NewLogModule("nsQtFilePicker");
#endif
}

nsFilePicker::~nsFilePicker()
{
    delete mDialog;
}

NS_IMETHODIMP
nsFilePicker::Init(nsIDOMWindow *parent, const nsAString & title, PRInt16 mode)
{
    return nsBaseFilePicker::Init(parent, title, mode);
}


NS_IMETHODIMP
nsFilePicker::AppendFilters(PRInt32 filterMask)
{
    return nsBaseFilePicker::AppendFilters(filterMask);
}


NS_IMETHODIMP
nsFilePicker::AppendFilter(const nsAString & aTitle, const nsAString & aFilter)
{
    if (aFilter.Equals(NS_LITERAL_STRING("..apps"))) {
        
        return NS_OK;
    }

    nsCAutoString filter, name;
    CopyUTF16toUTF8(aFilter, filter);
    CopyUTF16toUTF8(aTitle, name);

    mFilters.AppendElement(filter);
    mFilterNames.AppendElement(name);

    return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::GetDefaultString(nsAString & aDefaultString)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFilePicker::SetDefaultString(const nsAString & aDefaultString)
{
    mDefault = aDefaultString;

    return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::GetDefaultExtension(nsAString & aDefaultExtension)
{
    aDefaultExtension = mDefaultExtension;

    return NS_OK;
}
NS_IMETHODIMP
nsFilePicker::SetDefaultExtension(const nsAString & aDefaultExtension)
{
    mDefaultExtension = aDefaultExtension;

    return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
    *aFilterIndex = mSelectedType;

    return NS_OK;
}
NS_IMETHODIMP
nsFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
    mSelectedType = aFilterIndex;

    return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::GetFile(nsILocalFile * *aFile)
{
    NS_ENSURE_ARG_POINTER(aFile);

    *aFile = nsnull;
    if (mFile.IsEmpty()) {
        return NS_OK;
    }

    nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

    file->InitWithNativePath(mFile);

    NS_ADDREF(*aFile = file);

    return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::GetFileURL(nsIURI * *aFileURL)
{
    nsCOMPtr<nsILocalFile> file;
    GetFile(getter_AddRefs(file));

    nsCOMPtr<nsIURI> uri;
    NS_NewFileURI(getter_AddRefs(uri), file);
    NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

    return CallQueryInterface(uri, aFileURL);
}


NS_IMETHODIMP
nsFilePicker::GetFiles(nsISimpleEnumerator * *aFiles)
{
    NS_ENSURE_ARG_POINTER(aFiles);

    if (mMode == nsIFilePicker::modeOpenMultiple) {
        return NS_NewArrayEnumerator(aFiles, mFiles);
    }

    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsFilePicker::Show(PRInt16 *aReturn)
{
    nsCAutoString directory;
    if (mDisplayDirectory) {
        mDisplayDirectory->GetNativePath(directory);
    }

    switch (mMode) {
    case nsIFilePicker::modeOpen:
        break;
    case nsIFilePicker::modeOpenMultiple:
        mDialog->setFileMode(QFileDialog::ExistingFiles);
        break;
    case nsIFilePicker::modeSave:
        mDialog->setFileMode(QFileDialog::AnyFile);
        mDialog->setAcceptMode(QFileDialog::AcceptSave);
        break;
    case nsIFilePicker::modeGetFolder:
        mDialog->setFileMode(QFileDialog::DirectoryOnly);
        break;
    default:
        break;
    }

    mDialog->selectFile(QString::fromUtf16(mDefault.get()));

    mDialog->setDirectory(directory.get());

    QStringList filters;
    PRUint32 count = mFilters.Length();
    for (PRUint32 i = 0; i < count; ++i) {
        filters.append( mFilters[i].get() );
    }
    mDialog->setFilters(filters);

    switch (mDialog->exec()) {
    case QDialog::Accepted: {
        QStringList files = mDialog->selectedFiles();
        QString selected;
        if (!files.isEmpty())
        {
            selected = files[0];
        }

        QString path = QFile::encodeName(selected);
        PR_LOG(sFilePickerLog, PR_LOG_DEBUG, ("path is '%s'", path.toAscii().data()));
        mFile.Assign(path.toUtf8().data());
        *aReturn = nsIFilePicker::returnOK;
        if (mMode == modeSave) {
            nsCOMPtr<nsILocalFile> file;
            GetFile(getter_AddRefs(file));
            if (file) {
                PRBool exists = PR_FALSE;
                file->Exists(&exists);
                if (exists) {
                    *aReturn = nsIFilePicker::returnReplace;
                }
            }
        }
    }
        break;
    case QDialog::Rejected: {
        *aReturn = nsIFilePicker::returnCancel;
    }
        break;
    default:
        *aReturn = nsIFilePicker::returnCancel;
        break;
    }


    return NS_OK;
}

void nsFilePicker::InitNative(nsIWidget *parent, const nsAString &title, PRInt16 mode)
{
    PR_LOG(sFilePickerLog, PR_LOG_DEBUG, ("nsFilePicker::InitNative"));

    nsAutoString str(title);
    mDialog = new QFileDialog(0, QString::fromUtf16(str.get()));

    QGraphicsWidget *parentWidget = static_cast<QGraphicsWidget*>(parent->GetNativeData(NS_NATIVE_WIDGET));
    if (parentWidget && parentWidget->scene()) {
        parentWidget->scene()->addWidget(mDialog);
    }

    mMode = mode;
}
