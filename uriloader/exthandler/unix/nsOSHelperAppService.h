







































#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"

class nsHashtable;
class nsILineInputStream;

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMimeType,
                                                  const nsACString& aFileExt,
                                                  PRBool     *aFound);
  NS_IMETHOD GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                          PRBool *found,
                                          nsIHandlerInfo **_retval);

  
  nsresult OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists);
  NS_IMETHOD GetApplicationDescription(const nsACString& aScheme, nsAString& _retval);

  
  
  
  
  virtual nsresult GetFileTokenForPath(const PRUnichar * platformAppPath, nsIFile ** aFile);
  
protected:
  already_AddRefed<nsMIMEInfoBase> GetFromType(const nsCString& aMimeType);
  already_AddRefed<nsMIMEInfoBase> GetFromExtension(const nsCString& aFileExt);

  virtual void FixFilePermissions(nsILocalFile* aFile);
private:
  PRUint32 mPermissions;

  
  static nsresult UnescapeCommand(const nsAString& aEscapedCommand,
                                  const nsAString& aMajorType,
                                  const nsAString& aMinorType,
                                  nsHashtable& aTypeOptions,
                                  nsACString& aUnEscapedCommand);
  static nsresult GetFileLocation(const char* aPrefName,
                                  const char* aEnvVarName,
                                  PRUnichar** aFileLocation);
  static nsresult LookUpTypeAndDescription(const nsAString& aFileExtension,
                                           nsAString& aMajorType,
                                           nsAString& aMinorType,
                                           nsAString& aDescription,
                                           PRBool aUserData);
  static nsresult CreateInputStream(const nsAString& aFilename,
                                    nsIFileInputStream ** aFileInputStream,
                                    nsILineInputStream ** aLineInputStream,
                                    nsACString& aBuffer,
                                    PRBool * aNetscapeFormat,
                                    PRBool * aMore);

  static nsresult GetTypeAndDescriptionFromMimetypesFile(const nsAString& aFilename,
                                                         const nsAString& aFileExtension,
                                                         nsAString& aMajorType,
                                                         nsAString& aMinorType,
                                                         nsAString& aDescription);

  static nsresult LookUpExtensionsAndDescription(const nsAString& aMajorType,
                                                 const nsAString& aMinorType,
                                                 nsAString& aFileExtensions,
                                                 nsAString& aDescription);

  static nsresult GetExtensionsAndDescriptionFromMimetypesFile(const nsAString& aFilename,
                                                               const nsAString& aMajorType,
                                                               const nsAString& aMinorType,
                                                               nsAString& aFileExtensions,
                                                               nsAString& aDescription);

  static nsresult ParseNetscapeMIMETypesEntry(const nsAString& aEntry,
                                              nsAString::const_iterator& aMajorTypeStart,
                                              nsAString::const_iterator& aMajorTypeEnd,
                                              nsAString::const_iterator& aMinorTypeStart,
                                              nsAString::const_iterator& aMinorTypeEnd,
                                              nsAString& aExtensions,
                                              nsAString::const_iterator& aDescriptionStart,
                                              nsAString::const_iterator& aDescriptionEnd);

  static nsresult ParseNormalMIMETypesEntry(const nsAString& aEntry,
                                            nsAString::const_iterator& aMajorTypeStart,
                                            nsAString::const_iterator& aMajorTypeEnd,
                                            nsAString::const_iterator& aMinorTypeStart,
                                            nsAString::const_iterator& aMinorTypeEnd,
                                            nsAString& aExtensions,
                                            nsAString::const_iterator& aDescriptionStart,
                                            nsAString::const_iterator& aDescriptionEnd);

  static nsresult LookUpHandlerAndDescription(const nsAString& aMajorType,
                                              const nsAString& aMinorType,
                                              nsHashtable& aTypeOptions,
                                              nsAString& aHandler,
                                              nsAString& aDescription,
                                              nsAString& aMozillaFlags);
  
  static nsresult DoLookUpHandlerAndDescription(const nsAString& aMajorType,
                                                const nsAString& aMinorType,
                                                nsHashtable& aTypeOptions,
                                                nsAString& aHandler,
                                                nsAString& aDescription,
                                                nsAString& aMozillaFlags,
                                                PRBool aUserData);
  
  static nsresult GetHandlerAndDescriptionFromMailcapFile(const nsAString& aFilename,
                                                          const nsAString& aMajorType,
                                                          const nsAString& aMinorType,
                                                          nsHashtable& aTypeOptions,
                                                          nsAString& aHandler,
                                                          nsAString& aDescription,
                                                          nsAString& aMozillaFlags);
};

#endif 
