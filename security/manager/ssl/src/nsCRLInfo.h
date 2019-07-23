



































#ifndef _NSCLRLINFO_H_
#define _NSCRLINFO_H_

#include "nsICRLInfo.h"

#include "certt.h"
#include "nsString.h"
	  
#define CRL_AUTOUPDATE_TIMIINGTYPE_PREF "security.crl.autoupdate.timingType"
#define CRL_AUTOUPDATE_TIME_PREF "security.crl.autoupdate.nextInstant"
#define CRL_AUTOUPDATE_URL_PREF "security.crl.autoupdate.url"
#define CRL_AUTOUPDATE_DAYCNT_PREF "security.crl.autoupdate.dayCnt"
#define CRL_AUTOUPDATE_FREQCNT_PREF "security.crl.autoupdate.freqCnt"
#define CRL_AUTOUPDATE_ERRCNT_PREF "security.crl.autoupdate.errCount"
#define CRL_AUTOUPDATE_ERRDETAIL_PREF "security.crl.autoupdate.errDetail"
#define CRL_AUTOUPDATE_ENABLED_PREF "security.crl.autoupdate.enable."
#define CRL_AUTOUPDATE_DEFAULT_DELAY 30000UL

class nsCRLInfo : public nsICRLInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICRLINFO

  nsCRLInfo();
  nsCRLInfo(CERTSignedCrl *);
  virtual ~nsCRLInfo();
  
private:
  nsString mOrg;
  nsString mOrgUnit;
  nsString mLastUpdateLocale;
  nsString mNextUpdateLocale;
  PRTime mLastUpdate;
  PRTime mNextUpdate;
  nsString mNameInDb;
  nsCString mLastFetchURL;
  nsString mNextAutoUpdateDate;
};

#endif
