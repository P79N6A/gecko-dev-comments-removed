




































#include "PromptService.h"
#include "nsCOMPtr.h"
#include "nsIPromptService.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIWindowWatcher.h"
#include "nsIServiceManager.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"


#include "ApplIDs.h"
#include "UMacUnicode.h"
#include "CBrowserChrome.h"


#include <LStaticText.h>
#include <LCheckBox.h>
#include <LEditText.h>
#include <UModalDialogs.h>
#include <LPushButton.h>





NS_IMPL_ISUPPORTS1(CPromptService, nsIPromptService)

CPromptService::CPromptService()
{
}

CPromptService::~CPromptService()
{
}


NS_IMETHODIMP CPromptService::Alert(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                    const PRUnichar *text)
{    
    StDialogHandler	 theHandler(dlog_Alert, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    nsCAutoString    cStr;
    Str255           pStr;

    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
    theDialog->SetDescriptor(pStr);

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');   			
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());

    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
   		break;
	}

    return NS_OK;
}

NS_IMETHODIMP CPromptService::AlertCheck(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                         const PRUnichar *text,
                                         const PRUnichar *checkMsg, PRBool *checkValue)
{
    NS_ENSURE_ARG_POINTER(checkValue);

    StDialogHandler	theHandler(dlog_AlertCheck, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    nsCAutoString    cStr;
    Str255           pStr;

    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
    theDialog->SetDescriptor(pStr);

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');   			
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());
    
    LCheckBox *checkBox = dynamic_cast<LCheckBox*>(theDialog->FindPaneByID('Chck'));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(checkMsg), pStr);
    checkBox->SetDescriptor(pStr);
    checkBox->SetValue(*checkValue ? 1 : 0);

    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
		{
		    *checkValue = checkBox->GetValue();    
   		    break;
   		}
	}

    return NS_OK;
}

NS_IMETHODIMP CPromptService::Confirm(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                      const PRUnichar *text,
                                      PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    
    StDialogHandler	theHandler(dlog_Confirm, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    nsCAutoString    cStr;
    Str255           pStr;
    
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
    theDialog->SetDescriptor(pStr);
   			
    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');   			
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());

    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
		{
		    *_retval = PR_TRUE;    
   		    break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    *_retval = PR_FALSE;
   		    break;
   		}
	}

    return NS_OK;
}

NS_IMETHODIMP CPromptService::ConfirmCheck(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                           const PRUnichar *text,
                                           const PRUnichar *checkMsg, PRBool *checkValue,
                                           PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(checkValue);
    NS_ENSURE_ARG_POINTER(_retval);

    StDialogHandler	theHandler(dlog_ConfirmCheck, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    nsCAutoString    cStr;
    Str255           pStr;

    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
    theDialog->SetDescriptor(pStr);

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');   			
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());
    
    LCheckBox *checkBox = dynamic_cast<LCheckBox*>(theDialog->FindPaneByID('Chck'));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(checkMsg), pStr);
    checkBox->SetDescriptor(pStr);
    checkBox->SetValue(*checkValue ? 1 : 0);

    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
		{
		    *_retval = PR_TRUE;
		    *checkValue = checkBox->GetValue();    
   		    break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    *_retval = PR_FALSE;
   		    break;
   		}
	}

    return NS_OK;
}

NS_IMETHODIMP CPromptService::ConfirmEx(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                        const PRUnichar *text,
                                        PRUint32 buttonFlags,
                                        const PRUnichar *button0Title,
                                        const PRUnichar *button1Title,
                                        const PRUnichar *button2Title,
                                        const PRUnichar *checkMsg, PRBool *checkValue,
                                        PRInt32 *buttonPressed)
{
    NS_ENSURE_ARG_POINTER(buttonPressed);

    StDialogHandler	theHandler(dlog_ConfirmEx, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    nsCAutoString   cStr;
    Str255          pStr;
    SDimension16    oldSize;
    SInt16          bestWidth, bestHeight, baselineOffset;

    if (dialogTitle) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
        theDialog->SetDescriptor(pStr);
    }

    LCheckBox *checkBox = nil;
    if (checkMsg && checkValue) {
        checkBox = dynamic_cast<LCheckBox*>(theDialog->FindPaneByID('Chck'));
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(checkMsg), pStr);
        checkBox->SetDescriptor(pStr);
        checkBox->SetValue(*checkValue ? 1 : 0);
        
        
        
        
        if (checkBox->SupportsCalcBestRect()) {
            checkBox->GetFrameSize(oldSize);
            checkBox->CalcBestControlRect(bestWidth, bestHeight, baselineOffset);
            
            if (bestWidth > oldSize.width)
                theDialog->ResizeWindowBy(bestWidth - oldSize.width, bestHeight - oldSize.height);
        }
    }

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());
    
    
    if (msgText->SupportsCalcBestRect()) {
        msgText->GetFrameSize(oldSize);
        msgText->CalcBestControlRect(bestWidth, bestHeight, baselineOffset);
        
        
        SInt16 hDelta = bestWidth - oldSize.width;
        if (hDelta < 0)
            hDelta = 0;
        theDialog->ResizeWindowBy(hDelta, bestHeight - oldSize.height);
    }
    
    const PRUnichar* buttonTitles[3] = { button0Title, button1Title, button2Title };
    PaneIDT buttonIDs[3] = { 'Btn0', 'Btn1', 'Btn2' };
    SInt16 buttonShift = 0;
    
    for (int i = 0; i < 3; i++)
    {
        LPushButton *button = dynamic_cast<LPushButton*>(theDialog->FindPaneByID(buttonIDs[i]));
        if (!button)
            continue;
            
        SInt16 strIndex = -1;
        Boolean hidden = false;
        
        switch (buttonFlags & 0x00FF)
        {
            case 0:
                button->Hide();
                hidden = true;
                break;
            case BUTTON_TITLE_OK:
                strIndex = str_OK;
                break;
            case BUTTON_TITLE_CANCEL:
                strIndex = str_Cancel;
                break;
            case BUTTON_TITLE_YES:
                strIndex = str_Yes;
                break;
            case BUTTON_TITLE_NO:
                strIndex = str_No;
                break;
            case BUTTON_TITLE_SAVE:
                strIndex = str_Save;
                break;
            case BUTTON_TITLE_DONT_SAVE:
                strIndex = str_DontSave;
                break;
            case BUTTON_TITLE_REVERT:
                strIndex = str_Revert;
                break;
            case BUTTON_TITLE_IS_STRING:
                {
                CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(buttonTitles[i]), pStr);
                button->SetDescriptor(pStr);
                }
                break;
        }
        if (strIndex != -1) {
            LStr255 buttonTitle(STRx_StdButtonTitles, strIndex);
            button->SetDescriptor(buttonTitle);
        }
        if (!hidden) {
            if (button->SupportsCalcBestRect()) {
                button->GetFrameSize(oldSize);
                button->CalcBestControlRect(bestWidth, bestHeight, baselineOffset);
                buttonShift += bestWidth - oldSize.width;
                button->MoveBy(-buttonShift, 0, false);
                button->ResizeFrameTo(bestWidth, bestHeight, false);
            }
        }
        buttonFlags >>= 8;
    }
        
    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == 'Btn0')
		{
		    *buttonPressed = 0;
   		    break;
   		}
   		else if (hitMessage == 'Btn1')
   		{
   		    *buttonPressed = 1;
   		    break;
   		}
   		else if (hitMessage == 'Btn2')
   		{
   		    *buttonPressed = 2;
   		    break;
   		}
	}

    if (checkBox)
        *checkValue = checkBox->GetValue();

    return NS_OK;
}

NS_IMETHODIMP CPromptService::Prompt(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                     const PRUnichar *text, PRUnichar **value,
                                     const PRUnichar *checkMsg, PRBool *checkValue,
                                     PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    nsresult resultErr = NS_OK;

    StDialogHandler	theHandler(dlog_Prompt, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    LCheckBox        *checkbox = dynamic_cast<LCheckBox*>(theDialog->FindPaneByID('Chck'));
    nsCAutoString   cStr;
    Str255          pStr;

    if (dialogTitle) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
        theDialog->SetDescriptor(pStr);
    }

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());
    
    LEditText *responseText = dynamic_cast<LEditText*>(theDialog->FindPaneByID('Rslt'));
    if (value && *value) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(*value), pStr);
        responseText->SetDescriptor(pStr);
    }
    
    if (checkValue) {
        checkbox->SetValue(*checkValue);
        if (checkMsg) {
            CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(checkMsg), pStr);
            checkbox->SetDescriptor(pStr);
        }
    }
    else
        checkbox->Hide();

    theDialog->SetLatentSub(responseText);    
    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
		{
		    nsAutoString ucStr;

		    *_retval = PR_TRUE;
		    if (value && *value) {
		        nsMemory::Free(*value);
		        *value = nsnull;
		    }
		    responseText->GetDescriptor(pStr);
		    CPlatformUCSConversion::GetInstance()->PlatformToUCS(pStr, ucStr);
		    *value = ToNewUnicode(ucStr);    
   		    if (*value == nsnull)
   		        resultErr = NS_ERROR_OUT_OF_MEMORY;
   		        
   		    if (checkValue)
   		        *checkValue = checkbox->GetValue();
   		        
   		    break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    *_retval = PR_FALSE;
   		    break;
   		}
	}

    return resultErr;
}

NS_IMETHODIMP CPromptService::PromptUsernameAndPassword(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                                        const PRUnichar *text,
                                                        PRUnichar **username, PRUnichar **password,
                                                        const PRUnichar *checkMsg, PRBool *checkValue,
                                                        PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    nsresult resultErr = NS_OK;

    StDialogHandler	theHandler(dlog_PromptNameAndPass, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    LCheckBox        *checkbox = dynamic_cast<LCheckBox*>(theDialog->FindPaneByID('Chck'));
    nsCAutoString   cStr;
    Str255          pStr;

    if (dialogTitle) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
        theDialog->SetDescriptor(pStr);
    }

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());
    
    LEditText *userText = dynamic_cast<LEditText*>(theDialog->FindPaneByID('Name'));
    if (username && *username) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(*username), pStr);
        userText->SetDescriptor(pStr);
    }
    LEditText *pwdText = dynamic_cast<LEditText*>(theDialog->FindPaneByID('Pass'));
    if (password && *password) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(*password), pStr);
        pwdText->SetDescriptor(pStr);
    }

    if (checkValue) {
        checkbox->SetValue(*checkValue);
        if (checkMsg) {
            CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(checkMsg), pStr);
            checkbox->SetDescriptor(pStr);
        }
    }
    else
        checkbox->Hide();
 
    theDialog->SetLatentSub(userText);   
    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
		{
		    nsAutoString    ucStr;
		    
		    if (username && *username) {
		        nsMemory::Free(*username);
		        *username = nsnull;
		    }
		    userText->GetDescriptor(pStr);
		    CPlatformUCSConversion::GetInstance()->PlatformToUCS(pStr, ucStr);
		    *username = ToNewUnicode(ucStr);
		    if (*username == nsnull)
		        resultErr = NS_ERROR_OUT_OF_MEMORY;
		    
		    if (password && *password) {
		        nsMemory::Free(*password);
		        *password = nsnull;
		    }
		    pwdText->GetDescriptor(pStr);
		    CPlatformUCSConversion::GetInstance()->PlatformToUCS(pStr, ucStr);
		    *password = ToNewUnicode(ucStr);
		    if (*password == nsnull)
		        resultErr = NS_ERROR_OUT_OF_MEMORY;

   		    if (checkValue)
   		        *checkValue = checkbox->GetValue();
		    
		    *_retval = PR_TRUE;        
   		    break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    *_retval = PR_FALSE;
   		    break;
   		}
	}

    return resultErr;
}

NS_IMETHODIMP CPromptService::PromptPassword(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                             const PRUnichar *text,
                                             PRUnichar **password,
                                             const PRUnichar *checkMsg, PRBool *checkValue,
                                             PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    
    nsresult resultErr = NS_OK;

    StDialogHandler	 theHandler(dlog_PromptPassword, CBrowserChrome::GetLWindowForDOMWindow(parent));
    LWindow			 *theDialog = theHandler.GetDialog();
    LCheckBox        *checkbox = dynamic_cast<LCheckBox*>(theDialog->FindPaneByID('Chck'));
    nsCAutoString    cStr;
    Str255           pStr;

    if (dialogTitle) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(dialogTitle), pStr);
        theDialog->SetDescriptor(pStr);
    }

    LStaticText	*msgText = dynamic_cast<LStaticText*>(theDialog->FindPaneByID('Msg '));
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(text), cStr);
    cStr.ReplaceChar('\n', '\r');
    msgText->SetText(const_cast<char *>(cStr.get()), cStr.Length());
    
    LEditText *pwdText = dynamic_cast<LEditText*>(theDialog->FindPaneByID('Pass'));
    if (password && *password) {
        CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(*password), pStr);
        pwdText->SetDescriptor(pStr);
    }
    
    if (checkValue) {
        checkbox->SetValue(*checkValue);
        if (checkMsg) {
            CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(checkMsg), pStr);
            checkbox->SetDescriptor(pStr);
        }
    }
    else
        checkbox->Hide();
 
    theDialog->SetLatentSub(pwdText);   
    theDialog->Show();
    theDialog->Select();
	
	while (true)  
	{				
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_OK)
		{
		    nsAutoString    ucStr;
		    		    
		    if (password && *password) {
		        nsMemory::Free(*password);
		        *password = nsnull;
		    }
		    pwdText->GetDescriptor(pStr);
		    CPlatformUCSConversion::GetInstance()->PlatformToUCS(pStr, ucStr);
		    *password = ToNewUnicode(ucStr);
		    if (*password == nsnull)
		        resultErr = NS_ERROR_OUT_OF_MEMORY;

   		    if (checkValue)
   		        *checkValue = checkbox->GetValue();

		    *_retval = PR_TRUE;        
   		    break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    *_retval = PR_FALSE;
   		    break;
   		}
	}

    return resultErr;
}

NS_IMETHODIMP CPromptService::Select(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                     const PRUnichar *text, PRUint32 count,
                                     const PRUnichar **selectList, PRInt32 *outSelection,
                                     PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
