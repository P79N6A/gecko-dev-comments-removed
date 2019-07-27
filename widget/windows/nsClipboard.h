




#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsBaseClipboard.h"
#include "nsIObserver.h"
#include "nsIURI.h"
#include <windows.h>

class nsITransferable;
class nsIWidget;
class nsIFile;
struct IDataObject;





class nsClipboard : public nsBaseClipboard,
                    public nsIObserver
{
  virtual ~nsClipboard();

public:
  nsClipboard();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, uint32_t aLength,
                                    int32_t aWhichClipboard, bool *_retval); 
  NS_IMETHOD EmptyClipboard(int32_t aWhichClipboard);

  
  static nsresult CreateNativeDataObject(nsITransferable * aTransferable, 
                                         IDataObject ** aDataObj,
                                         nsIURI       * uri);
  static nsresult SetupNativeDataObject(nsITransferable * aTransferable, 
                                        IDataObject * aDataObj);
  static nsresult GetDataFromDataObject(IDataObject     * aDataObject,
                                        UINT              anIndex,
                                        nsIWidget       * aWindow,
                                        nsITransferable * aTransferable);
  static nsresult GetNativeDataOffClipboard(nsIWidget * aWindow, UINT aIndex, UINT aFormat, void ** aData, uint32_t * aLen);
  static nsresult GetNativeDataOffClipboard(IDataObject * aDataObject, UINT aIndex, UINT aFormat, const char * aMIMEImageFormat, void ** aData, uint32_t * aLen);
  static nsresult GetGlobalData(HGLOBAL aHGBL, void ** aData, uint32_t * aLen);
  static UINT     GetFormat(const char* aMimeStr);

  static UINT     CF_HTML;
  
protected:
  NS_IMETHOD SetNativeClipboardData ( int32_t aWhichClipboard );
  NS_IMETHOD GetNativeClipboardData ( nsITransferable * aTransferable, int32_t aWhichClipboard );
  
  static bool IsInternetShortcut ( const nsAString& inFileName ) ;
  static bool FindURLFromLocalFile ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen ) ;
  static bool FindURLFromNativeURL ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen ) ;
  static bool FindUnicodeFromPlainText ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen ) ;
  static bool FindPlatformHTML ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen );
  static void ResolveShortcut ( nsIFile* inFileName, nsACString& outURL ) ;

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

