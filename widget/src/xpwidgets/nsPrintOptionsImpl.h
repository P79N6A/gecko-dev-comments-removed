







































#ifndef nsPrintOptionsImpl_h__
#define nsPrintOptionsImpl_h__

#include "nsCOMPtr.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSettingsService.h"
#include "nsString.h"
#include "nsFont.h"




class nsPrintOptions : public nsIPrintOptions,
                       public nsIPrintSettingsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTOPTIONS
  NS_DECL_NSIPRINTSETTINGSSERVICE

  





  virtual nsresult Init();

  nsPrintOptions();
  virtual ~nsPrintOptions();

protected:
  void ReadBitFieldPref(const char * aPrefId, PRInt32 anOption);
  void WriteBitFieldPref(const char * aPrefId, PRInt32 anOption);
  void ReadJustification(const char * aPrefId, PRInt16& aJust,
                         PRInt16 aInitValue);
  void WriteJustification(const char * aPrefId, PRInt16 aJust);
  void ReadInchesToTwipsPref(const char * aPrefId, PRInt32&  aTwips,
                             const char * aMarginPref);
  void WriteInchesFromTwipsPref(const char * aPrefId, PRInt32 aTwips);
  void ReadInchesIntToTwipsPref(const char * aPrefId, PRInt32&  aTwips,
                                const char * aMarginPref);
  void WriteInchesIntFromTwipsPref(const char * aPrefId, PRInt32 aTwips);

  nsresult ReadPrefDouble(const char * aPrefId, double& aVal);
  nsresult WritePrefDouble(const char * aPrefId, double aVal);

  





  virtual nsresult ReadPrefs(nsIPrintSettings* aPS, const nsAString&
                             aPrinterName, PRUint32 aFlags);
  





  virtual nsresult WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrefName,
                              PRUint32 aFlags);
  const char* GetPrefName(const char *     aPrefName,
                          const nsAString&  aPrinterName);

  





  virtual nsresult _CreatePrintSettings(nsIPrintSettings **_retval);

  
  nsCOMPtr<nsIPrintSettings> mGlobalPrintSettings;

  nsCString mPrefName;

private:
  
  nsPrintOptions(const nsPrintOptions& x);
  nsPrintOptions& operator=(const nsPrintOptions& x);
};

#endif 
