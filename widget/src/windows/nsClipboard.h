




































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsBaseClipboard.h"
#include "nsIURI.h"
#include <windows.h>

class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;
class nsILocalFile;
struct IDataObject;





class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, PRBool *_retval); 

  
  static nsresult CreateNativeDataObject(nsITransferable * aTransferable, 
                                         IDataObject ** aDataObj,
                                         nsIURI       * uri);
  static nsresult SetupNativeDataObject(nsITransferable * aTransferable, 
                                        IDataObject * aDataObj);
  static nsresult GetDataFromDataObject(IDataObject     * aDataObject,
                                        UINT              anIndex,
                                        nsIWidget       * aWindow,
                                        nsITransferable * aTransferable);
  static nsresult GetNativeDataOffClipboard(nsIWidget * aWindow, UINT aIndex, const char * aMIMEFormat, void ** aData, PRUint32 * aLen);
  static nsresult GetNativeDataOffClipboard(IDataObject * aDataObject, UINT aIndex, const char * aMIMEFormat, void ** aData, PRUint32 * aLen);
  static nsresult GetGlobalData(HGLOBAL aHGBL, void ** aData, PRUint32 * aLen);
  static UINT     GetFormat(const char* aMimeStr);

  static UINT     CF_HTML;
  
protected:
  NS_IMETHOD SetNativeClipboardData ( PRInt32 aWhichClipboard );
  NS_IMETHOD GetNativeClipboardData ( nsITransferable * aTransferable, PRInt32 aWhichClipboard );
  
  static PRBool IsInternetShortcut ( const nsAString& inFileName ) ;
  static PRBool FindURLFromLocalFile ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen ) ;
  static PRBool FindURLFromNativeURL ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen ) ;
  static PRBool FindUnicodeFromPlainText ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen ) ;
  static PRBool FindPlatformHTML ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen );
  static void ResolveShortcut ( nsILocalFile* inFileName, nsACString& outURL ) ;

  nsIWidget         * mWindow;

};

#define SET_FORMATETC(fe, cf, td, asp, li, med)   \
	 {\
	 (fe).cfFormat=cf;\
	 (fe).ptd=td;\
	 (fe).dwAspect=asp;\
	 (fe).lindex=li;\
	 (fe).tymed=med;\
	 }


#endif 

