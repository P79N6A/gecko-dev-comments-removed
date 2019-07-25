







































#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsMIMEInfoImpl.h"
#include "nsCOMPtr.h"

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(mLog, PR_LOG_DEBUG)

class nsHashtable;
class nsILineInputStream;
class nsMIMEInfoOS2;

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  
  NS_IMETHOD GetFromTypeAndExtension(const nsACString& aMIMEType,
                                     const nsACString& aFileExt,
                                     nsIMIMEInfo **_retval);
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMimeType,
                                                  const nsACString& aFileExt,
                                                  bool       *aFound);
  NS_IMETHOD GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                          bool *found,
                                          nsIHandlerInfo **_retval);

  
  NS_IMETHODIMP GetApplicationDescription(const nsACString& aScheme, nsAString& _retval);

  nsresult OSProtocolHandlerExists(const char * aProtocolScheme, bool * aHandlerExists);

protected:
  already_AddRefed<nsMIMEInfoOS2> GetFromType(const nsCString& aMimeType);
  already_AddRefed<nsMIMEInfoOS2> GetFromExtension(const nsCString& aFileExt);

private:
  
  static nsresult UnescapeCommand(const nsAString& aEscapedCommand,
                                  const nsAString& aMajorType,
                                  const nsAString& aMinorType,
                                  nsHashtable& aTypeOptions,
                                  nsACString& aUnEscapedCommand);
  static nsresult GetFileLocation(const char* aPrefName,
                                  const char* aEnvVarName,
                                  nsAString& aFileLocation);
  static nsresult LookUpTypeAndDescription(const nsAString& aFileExtension,
                                           nsAString& aMajorType,
                                           nsAString& aMinorType,
                                           nsAString& aDescription);
  static nsresult CreateInputStream(const nsAString& aFilename,
                                    nsIFileInputStream ** aFileInputStream,
                                    nsILineInputStream ** aLineInputStream,
                                    nsACString& aBuffer,
                                    bool * aNetscapeFormat,
                                    bool * aMore);

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
  static nsresult GetHandlerAndDescriptionFromMailcapFile(const nsAString& aFilename,
                                                          const nsAString& aMajorType,
                                                          const nsAString& aMinorType,
                                                          nsHashtable& aTypeOptions,
                                                          nsAString& aHandler,
                                                          nsAString& aDescription,
                                                          nsAString& aMozillaFlags);
};

#define MAXINIPARAMLENGTH 1024 // max length of OS/2 INI key for application parameters


nsresult GetApplicationAndParametersFromINI(const nsACString& aProtocol,
                                            char* app, unsigned long appLength,
                                            char* param, unsigned long paramLength);

#endif 
