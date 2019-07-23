











































#ifdef _WINDOWS
  #include "stdafx.h"
#endif
#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"









NS_IMETHODIMP
CBrowserImpl::Alert(const PRUnichar *dialogTitle, const PRUnichar *text)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;
	
	m_pBrowserFrameGlue->Alert(dialogTitle, text);

	return NS_OK;
}


NS_IMETHODIMP
CBrowserImpl::Confirm(const PRUnichar *dialogTitle, 
			   const PRUnichar *text, PRBool *retval)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->Confirm(dialogTitle, text, retval);

	return NS_OK;
}

NS_IMETHODIMP
CBrowserImpl::Prompt(const PRUnichar *dialogTitle, const PRUnichar *text,
                     PRUnichar **promptText,
                     const PRUnichar *checkMsg, PRBool *checkValue,
                     PRBool *_retval)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->Prompt(dialogTitle, text, promptText, checkMsg, checkValue, _retval);

	return NS_OK;
}

NS_IMETHODIMP
CBrowserImpl::PromptPassword(const PRUnichar *dialogTitle, const PRUnichar *text,
                             PRUnichar **password,
                             const PRUnichar *checkMsg, PRBool *checkValue,
                             PRBool *_retval)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->PromptPassword(dialogTitle, text, password,
	                                    checkMsg, checkValue, _retval);

    return NS_OK;
}

NS_IMETHODIMP
CBrowserImpl::PromptUsernameAndPassword(const PRUnichar *dialogTitle, const PRUnichar *text,
                                        PRUnichar **username, PRUnichar **password,
                                        const PRUnichar *checkMsg, PRBool *checkValue,
                                        PRBool *_retval)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->PromptUserNamePassword(dialogTitle, text, 
							                    username, password,
							                    checkMsg, checkValue, 
							                    _retval);

    return NS_OK;
}

NS_IMETHODIMP
CBrowserImpl::AlertCheck(const PRUnichar *dialogTitle, 
			      const PRUnichar *text, 
			      const PRUnichar *checkMsg,
			      PRBool *checkValue)
{
	    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CBrowserImpl::ConfirmCheck(const PRUnichar *dialogTitle,
				const PRUnichar *text,
				const PRUnichar *checkMsg, 
				PRBool *checkValue, PRBool *retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CBrowserImpl::ConfirmEx(const PRUnichar *dialogTitle, const PRUnichar *text,
                        PRUint32 buttonFlags, const PRUnichar *button0Title,
                        const PRUnichar *button1Title, const PRUnichar *button2Title,
                        const PRUnichar *checkMsg, PRBool *checkValue,
                        PRInt32 *buttonPressed)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CBrowserImpl::Select(const PRUnichar *dialogTitle,
			  const PRUnichar *text, PRUint32 count,
			  const PRUnichar **selectList,
			  PRInt32 *outSelection, PRBool *retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

