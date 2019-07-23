





































#include "QtPromptService.h"
#include <nsString.h>
#include <nsIWindowWatcher.h>
#include <nsIWebBrowserChrome.h>
#include <nsIEmbeddingSiteWindow.h>
#include <nsCOMPtr.h>
#include <nsIServiceManager.h>
#include <nsReadableUtils.h>

#include <qmessagebox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qapplication.h>
#include <qstyle.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include "ui_alert.h"
#include "ui_confirm.h"
#include "ui_prompt.h"
#include "ui_select.h"
#include "ui_userpass.h"

#if (QT_VERSION < 0x030200)

#define SP_MessageBoxQuestion SP_MessageBoxInformation
#endif

QtPromptService::QtPromptService()
{
}

QtPromptService::~QtPromptService()
{
}

NS_IMPL_ISUPPORTS1(QtPromptService, nsIPromptService)




NS_IMETHODIMP
QtPromptService::Alert(nsIDOMWindow* aParent,
                       const PRUnichar* aDialogTitle,
                       const PRUnichar* aDialogText)
{
    return
        AlertCheck(aParent,
                   aDialogTitle, aDialogText,
                   NULL, NULL);
}





NS_IMETHODIMP
QtPromptService::AlertCheck(nsIDOMWindow* aParent,
                            const PRUnichar* aDialogTitle,
                            const PRUnichar* aDialogText,
                            const PRUnichar* aCheckMsg,
                            PRBool* aCheckValue)
{
    AlertDialog d(GetQWidgetForDOMWindow(aParent));
    d.icon->setPixmap(QApplication::style().
                      stylePixmap(QStyle::SP_MessageBoxWarning));
    if (aDialogTitle) {
        d.setCaption(QString::fromUcs2(aDialogTitle));
    }
    d.message->setText(QString::fromUcs2(aDialogText));
    if (aCheckMsg) {
        d.check->setText(QString::fromUcs2(aCheckMsg));
        d.check->setChecked(*aCheckValue);
    }
    else {
        d.check->hide();
    }
    d.adjustSize();
    d.exec();

    if (aCheckMsg) {
        *aCheckValue = d.check->isChecked();
    }

    return NS_OK;
}





NS_IMETHODIMP
QtPromptService::Confirm(nsIDOMWindow* aParent,
                         const PRUnichar* aDialogTitle,
                         const PRUnichar* aDialogText,
                         PRBool* aConfirm)
{
    return
        ConfirmCheck(aParent,
                     aDialogTitle, aDialogText,
                     NULL, NULL,
                     aConfirm);
}






NS_IMETHODIMP
QtPromptService::ConfirmCheck(nsIDOMWindow* aParent,
                              const PRUnichar* aDialogTitle,
                              const PRUnichar* aDialogText,
                              const PRUnichar* aCheckMsg,
                              PRBool* aCheckValue,
                              PRBool* aConfirm)
{
    PRInt32 ret;
    ConfirmEx(aParent,
              aDialogTitle, aDialogText,
              STD_OK_CANCEL_BUTTONS,
              NULL, NULL, NULL,
              aCheckMsg,
              aCheckValue,
              &ret);
    *aConfirm = (ret==0);

    return NS_OK;
}


























NS_IMETHODIMP
QtPromptService::ConfirmEx(nsIDOMWindow* aParent,
                           const PRUnichar* aDialogTitle,
                           const PRUnichar* aDialogText,
                           PRUint32 aButtonFlags,
                           const PRUnichar* aButton0Title,
                           const PRUnichar* aButton1Title,
                           const PRUnichar* aButton2Title,
                           const PRUnichar* aCheckMsg,
                           PRBool* aCheckValue,
                           PRInt32* aRetVal)
{
    ConfirmDialog d(GetQWidgetForDOMWindow(aParent));
    d.icon->setPixmap(QApplication::style().
                      stylePixmap(QStyle::SP_MessageBoxQuestion));
    if (aDialogTitle) {
        d.setCaption(QString::fromUcs2(aDialogTitle));
    }
    d.message->setText(QString::fromUcs2(aDialogText));

    QString l = GetButtonLabel(aButtonFlags, BUTTON_POS_0, aButton0Title);
    if (!l.isNull()) d.but1->setText(l); else d.but1->hide();
    l = GetButtonLabel(aButtonFlags, BUTTON_POS_1, aButton1Title);
    if (!l.isNull()) d.but2->setText(l); else d.but2->hide();
    l = GetButtonLabel(aButtonFlags, BUTTON_POS_2, aButton2Title);
    if (!l.isNull()) d.but3->setText(l); else d.but3->hide();

    if (aCheckMsg) {
        d.check->setText(QString::fromUcs2(aCheckMsg));
        d.check->setChecked(*aCheckValue);
    }
    else {
        d.check->hide();
    }
    d.adjustSize();
    int ret = d.exec();

    *aRetVal = ret;

    return NS_OK;
}














NS_IMETHODIMP
QtPromptService::Prompt(nsIDOMWindow* aParent,
                        const PRUnichar* aDialogTitle,
                        const PRUnichar* aDialogText,
                        PRUnichar** aValue,
                        const PRUnichar* aCheckMsg,
                        PRBool* aCheckValue,
                        PRBool* aConfirm)
{
    PromptDialog d(GetQWidgetForDOMWindow(aParent));
    d.icon->setPixmap(QApplication::style().
                      stylePixmap(QStyle::SP_MessageBoxQuestion));
    if (aDialogTitle) {
        d.setCaption(QString::fromUcs2(aDialogTitle));
    }
    d.message->setText(QString::fromUcs2(aDialogText));
    if (aValue && *aValue) {
        d.input->setText(QString::fromUcs2(*aValue));
    }
    if (aCheckMsg) {
        d.check->setText(QString::fromUcs2(aCheckMsg));
        d.check->setChecked(*aCheckValue);
    }
    else {
        d.check->hide();
    }
    d.adjustSize();
    int ret = d.exec();

    if (aCheckMsg) {
        *aCheckValue = d.check->isChecked();
    }
    *aConfirm = (ret & QMessageBox::Ok);
    if (*aConfirm) {
        if (*aValue) nsMemory::Free(*aValue);
        *aValue =
            ToNewUnicode(NS_ConvertUTF8toUTF16(d.input->text().utf8()));
    }

    return NS_OK;
}


















NS_IMETHODIMP
QtPromptService::PromptUsernameAndPassword(nsIDOMWindow* aParent,
                                           const PRUnichar* aDialogTitle,
                                           const PRUnichar* aDialogText,
                                           PRUnichar** aUsername,
                                           PRUnichar** aPassword,
                                           const PRUnichar* aCheckMsg,
                                           PRBool* aCheckValue,
                                           PRBool* aConfirm)
{
    UserpassDialog d(GetQWidgetForDOMWindow(aParent));
    d.icon->setPixmap(QApplication::style().
                      stylePixmap(QStyle::SP_MessageBoxQuestion));
    if (aDialogTitle) {
        d.setCaption(QString::fromUcs2(aDialogTitle));
    }
    d.message->setText(QString::fromUcs2(aDialogText));
    if (aUsername && *aUsername) {
        d.username->setText(QString::fromUcs2(*aUsername));
    }
    if (aPassword && *aPassword) {
        d.password->setText(QString::fromUcs2(*aPassword));
    }
    if (aCheckMsg) {
        d.check->setText(QString::fromUcs2(aCheckMsg));
        d.check->setChecked(*aCheckValue);
    }
    else {
        d.check->hide();
    }
    d.adjustSize();
    int ret = d.exec();

    if (aCheckMsg) {
        *aCheckValue = d.check->isChecked();
    }
    *aConfirm = (ret & QMessageBox::Ok);
    if (*aConfirm) {
        if (*aUsername) nsMemory::Free(*aUsername);
        *aUsername =
            ToNewUnicode(NS_ConvertUTF8toUTF16(d.username->text().utf8()));
        if (*aPassword) nsMemory::Free(*aPassword);
        *aPassword =
            ToNewUnicode(NS_ConvertUTF8toUTF16(d.password->text().utf8()));
    }

    return NS_OK;
}














NS_IMETHODIMP
QtPromptService::PromptPassword(nsIDOMWindow* aParent,
                                const PRUnichar* aDialogTitle,
                                const PRUnichar* aDialogText,
                                PRUnichar** aPassword,
                                const PRUnichar* aCheckMsg,
                                PRBool* aCheckValue,
                                PRBool* aConfirm)
{
    UserpassDialog d(GetQWidgetForDOMWindow(aParent));
    d.icon->setPixmap(QApplication::style().
                      stylePixmap(QStyle::SP_MessageBoxQuestion));
    if (aDialogTitle) {
        d.setCaption(QString::fromUcs2(aDialogTitle));
    }
    d.message->setText(QString::fromUcs2(aDialogText));
    d.lb_username->hide();
    d.username->hide();
    if (aPassword && *aPassword) {
        d.password->setText(QString::fromUcs2(*aPassword));
    }
    if (aCheckMsg) {
        d.check->setText(QString::fromUcs2(aCheckMsg));
        d.check->setChecked(*aCheckValue);
    }
    else {
        d.check->hide();
    }
    d.adjustSize();
    int ret = d.exec();

    if (aCheckMsg) {
        *aCheckValue = d.check->isChecked();
    }
    *aConfirm = (ret & QMessageBox::Ok);
    if (*aConfirm) {
        if (*aPassword) nsMemory::Free(*aPassword);
        *aPassword =
            ToNewUnicode(NS_ConvertUTF8toUTF16(d.password->text().utf8()));
    }

    return NS_OK;
}




NS_IMETHODIMP
QtPromptService::Select(nsIDOMWindow* aParent,
                        const PRUnichar* aDialogTitle,
                        const PRUnichar* aDialogText,
                        PRUint32 aCount,
                        const PRUnichar** aSelectList,
                        PRInt32* outSelection,
                        PRBool* aConfirm)
{
    SelectDialog d(GetQWidgetForDOMWindow(aParent));
    d.icon->setPixmap(QApplication::style().
                      stylePixmap(QStyle::SP_MessageBoxQuestion));
    if (aDialogTitle) {
        d.setCaption(QString::fromUcs2(aDialogTitle));
    }
    d.message->setText(QString::fromUcs2(aDialogText));
    if (aSelectList) {
        QStringList l;
        for (PRUint32 i = 0; i < aCount; ++i) {
            l.append(QString::fromUcs2(aSelectList[i]));
        }
        d.select->clear();
        d.select->insertStringList(l);
    }
    d.adjustSize();
    int ret = d.exec();

    *aConfirm = (ret & QMessageBox::Ok);
    if (*aConfirm) {
        *outSelection = d.select->currentItem();
    }

    return NS_OK;
}

QWidget*
QtPromptService::GetQWidgetForDOMWindow(nsIDOMWindow* aDOMWindow)
{
    nsCOMPtr<nsIWindowWatcher> wwatch = do_GetService("@mozilla.org/embedcomp/window-watcher;1");

    nsCOMPtr<nsIWebBrowserChrome> chrome;
    wwatch->GetChromeForWindow(aDOMWindow, getter_AddRefs(chrome));
    nsCOMPtr<nsIEmbeddingSiteWindow> siteWindow = do_QueryInterface(chrome);
    QWidget* parentWidget;
    siteWindow->GetSiteWindow((void**)&parentWidget);

    return parentWidget;
}

QString
QtPromptService::GetButtonLabel(PRUint32 aFlags,
                                PRUint32 aPos,
                                const PRUnichar* aStringValue)
{
    PRUint32 posFlag = (aFlags & (255 * aPos)) / aPos;
    switch (posFlag) {
    case BUTTON_TITLE_OK:
        return qApp->translate("QtPromptService", "&OK");
    case BUTTON_TITLE_CANCEL:
        return qApp->translate("QtPromptService", "&Cancel");
    case BUTTON_TITLE_YES:
        return qApp->translate("QtPromptService", "&Yes");
    case BUTTON_TITLE_NO:
        return qApp->translate("QtPromptService", "&No");
    case BUTTON_TITLE_SAVE:
        return qApp->translate("QtPromptService", "&Save");
    case BUTTON_TITLE_DONT_SAVE:
        return qApp->translate("QtPromptService", "&Don't Save");
    case BUTTON_TITLE_REVERT:
        return qApp->translate("QtPromptService", "&Revert");
    case BUTTON_TITLE_IS_STRING:
        return qApp->translate("QtPromptService",
                               QString::fromUcs2(aStringValue));
    case 0:
        return QString::null;
    default:
        NS_WARNING("Unexpected button flags");
        return QString::null;
    }
}
