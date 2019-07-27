





#ifndef nsPrintOptionsImpl_h__
#define nsPrintOptionsImpl_h__

#include "mozilla/embedding/PPrinting.h"
#include "nsCOMPtr.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSettingsService.h"
#include "nsString.h"
#include "nsFont.h"

class nsIPrintSettings;




class nsPrintOptions : public nsIPrintOptions,
                       public nsIPrintSettingsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTOPTIONS
  NS_DECL_NSIPRINTSETTINGSSERVICE

  





  virtual nsresult Init();

  nsPrintOptions();

protected:
  virtual ~nsPrintOptions();

  void ReadBitFieldPref(const char * aPrefId, int32_t anOption);
  void WriteBitFieldPref(const char * aPrefId, int32_t anOption);
  void ReadJustification(const char * aPrefId, int16_t& aJust,
                         int16_t aInitValue);
  void WriteJustification(const char * aPrefId, int16_t aJust);
  void ReadInchesToTwipsPref(const char * aPrefId, int32_t&  aTwips,
                             const char * aMarginPref);
  void WriteInchesFromTwipsPref(const char * aPrefId, int32_t aTwips);
  void ReadInchesIntToTwipsPref(const char * aPrefId, int32_t&  aTwips,
                                const char * aMarginPref);
  void WriteInchesIntFromTwipsPref(const char * aPrefId, int32_t aTwips);

  nsresult ReadPrefDouble(const char * aPrefId, double& aVal);
  nsresult WritePrefDouble(const char * aPrefId, double aVal);

  





  virtual nsresult ReadPrefs(nsIPrintSettings* aPS, const nsAString&
                             aPrinterName, uint32_t aFlags);
  





  virtual nsresult WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrefName,
                              uint32_t aFlags);
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
