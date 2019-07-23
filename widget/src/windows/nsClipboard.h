




































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsBaseClipboard.h"
#include "nsIObserver.h"
#include "nsIURI.h"
#include <windows.h>

class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;
class nsILocalFile;
struct IDataObject;





class nsClipboard : public nsBaseClipboard,
                    public nsIObserver
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD HasDataMatchingFlavors(nsISupportsArray *aFlavorList, PRInt32 aWhichClipboard, PRBool *_retval); 

  
  static nsresult CreateNativeDataObject(nsITransferable * aTransferable, 
                                         IDataObject ** aDataObj,
                                         nsIURI       * uri);
  static nsresult SetupNativeDataObject(nsITransferable * aTransferable, 
                                        IDataObject * aDataObj);
  static nsresult GetDataFromDataObject(IDataObject     * aDataObject,
                                        UINT              anIndex,
                                        nsIWidget       * aWindow,
                                        nsITransferable * aTransferable);
  static nsresult GetNativeDataOffClipboard(nsIWidget * aWindow, UINT aIndex, UINT aFormat, void ** aData, PRUint32 * aLen);
  static nsresult GetNativeDataOffClipboard(IDataObject * aDataObject, UINT aIndex, UINT aFormat, void ** aData, PRUint32 * aLen);
  static nsresult GetGlobalData(HGLOBAL aHGBL, void ** aData, PRUint32 * aLen);
  static UINT     GetFormat(const char* aMimeStr);

  static UINT     CF_HTML;
  
protected:
  NS_IMETHOD SetNativeClipboardData ( PRInt32 aWhichClipboard );
  NS_IMETHOD GetNativeClipboardData ( nsITransferable * aTransferable, PRInt32 aWhichClipboard );
  
  static PRBool IsInternetShortcut ( const nsAString& inFileName ) ;
  static PRBool FindURLFromLocalFile ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen ) ;
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

