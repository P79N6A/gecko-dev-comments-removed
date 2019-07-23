




































#include "nsCollationMac.h"
#include <Resources.h>
#include <TextUtils.h>
#include <Script.h>
#include "prmem.h"
#include "prmon.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsILocaleService.h"
#include "nsLocaleCID.h"
#include "nsIPlatformCharset.h"
#include "nsIMacLocale.h"
#include "nsCOMPtr.h"




static short mac_get_script_sort_id(const short scriptcode)
{
	short itl2num;
	ItlbRecord **ItlbRecordHandle;

	
	ItlbRecordHandle = (ItlbRecord **) GetResource('itlb', scriptcode);
	
	



	if(ItlbRecordHandle != NULL)
	{
		if(*ItlbRecordHandle == NULL)
			LoadResource((Handle)ItlbRecordHandle);
			
		if(*ItlbRecordHandle != NULL)
			itl2num = (*ItlbRecordHandle)->itlbSort;
		else
			itl2num = GetScriptVariable(scriptcode, smScriptSort);
	} else {	
		itl2num = GetScriptVariable(scriptcode, smScriptSort);
	}
	
	return itl2num;
}

static Handle itl2Handle;

static int mac_sort_tbl_compare(const void* s1, const void* s2)
{
	return CompareText((Ptr) s1, (Ptr) s2, 1, 1, itl2Handle);
}

static int mac_sort_tbl_init(const short scriptcode, unsigned char *mac_sort_tbl)
{
	int i;
	unsigned char sort_tbl[256];
	
	for (i = 0; i < 256; i++)
		sort_tbl[i] = (unsigned char) i;

	
	itl2Handle = GetResource('itl2', mac_get_script_sort_id(scriptcode));
	if (itl2Handle == NULL)
		return -1;
	
	
	PRMonitor* mon = PR_NewMonitor();
	PR_EnterMonitor(mon);
	qsort((void *) sort_tbl, 256, 1, mac_sort_tbl_compare);
	(void) PR_ExitMonitor(mon);
	PR_DestroyMonitor(mon);
	
	
	for (i = 0; i < 256; i++)
		mac_sort_tbl[sort_tbl[i]] = (unsigned char) i;
		
	return 0;
}

inline unsigned char mac_sort_tbl_search(const unsigned char ch, const unsigned char* mac_sort_tbl)
{
	
	return mac_sort_tbl[ch];
}



NS_IMPL_ISUPPORTS1(nsCollationMac, nsICollation)


nsCollationMac::nsCollationMac() 
{
  mCollation = NULL;
}

nsCollationMac::~nsCollationMac() 
{
  if (mCollation != NULL)
    delete mCollation;
}

nsresult nsCollationMac::Initialize(nsILocale* locale) 
{
  NS_ASSERTION(mCollation == NULL, "Should only be initialized once.");
  nsresult res;
  mCollation = new nsCollation;
  if (mCollation == NULL) {
    NS_ASSERTION(0, "mCollation creation failed");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  m_scriptcode = smRoman;

  nsAutoString localeStr;

  
  if (locale == nsnull) {
    nsCOMPtr<nsILocaleService> localeService = 
             do_GetService(NS_LOCALESERVICE_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      nsCOMPtr<nsILocale> appLocale;
      res = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(NS_LITERAL_STRING("NSILOCALE_COLLATE"), 
                                     localeStr);
      }
    }
  }
  else {
    res = locale->GetCategory(NS_LITERAL_STRING("NSILOCALE_COLLATE"), 
                              localeStr);
  }

  if (NS_SUCCEEDED(res)) {
    short scriptcode, langcode, regioncode;
    nsCOMPtr <nsIMacLocale> macLocale = do_GetService(NS_MACLOCALE_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      if (NS_SUCCEEDED(res = macLocale->GetPlatformLocale(localeStr, &scriptcode, &langcode, &regioncode))) {
        m_scriptcode = scriptcode;
      }
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      nsCAutoString mappedCharset;
      res = platformCharset->GetDefaultCharsetForLocale(localeStr, mappedCharset);
      if (NS_SUCCEEDED(res)) {
        res = mCollation->SetCharset(mappedCharset.get());
      }
    }
  }
  NS_ASSERTION(NS_SUCCEEDED(res), "initialization failed, use default values");

  
  if (mac_sort_tbl_init(m_scriptcode, m_mac_sort_tbl) == -1) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
};


nsresult nsCollationMac::CompareString(PRInt32 strength, 
                                       const nsAString& string1, const nsAString& string2, PRInt32* result)
{
  PRUint32 aLength1, aLength2;
  PRUint8 *aKey1 = nsnull, *aKey2 = nsnull;
  nsresult res;

  res = AllocateRawSortKey(strength, string1, &aKey1, &aLength1);
  if (NS_SUCCEEDED(res)) {
    res = AllocateRawSortKey(strength, string2, &aKey2, &aLength2);
    if (NS_SUCCEEDED(res))
      *result = strcmp((const char *)aKey1, (const char *)aKey2); 
  }

  
  PR_FREEIF(aKey1);
  PR_FREEIF(aKey2);

  return res;
}
 

nsresult nsCollationMac::AllocateRawSortKey(PRInt32 strength, 
                                            const nsAString& stringIn, PRUint8** key, PRUint32* outLen)
{
  nsresult res = NS_OK;

  nsAutoString stringNormalized;
  if (strength != kCollationCaseSensitive) {
    res = mCollation->NormalizeString(stringIn, stringNormalized);
  } else {
    stringNormalized = stringIn;
  }

  
  char *str;
  int str_len;

  res = mCollation->UnicodeToChar(stringNormalized, &str);
  if (NS_SUCCEEDED(res) && str != NULL) {
    str_len = strlen(str);
    *key = (PRUint8 *)str;
    *outLen = str_len + 1;
    
    
    if (smJapanese != m_scriptcode && smKorean != m_scriptcode && 
        smTradChinese != m_scriptcode && smSimpChinese != m_scriptcode) {
      while (*str) {
        *str = (PRUint8) mac_sort_tbl_search((const unsigned char) *str, m_mac_sort_tbl);
        ++str;
      }
    }
    
    
    else if (smJapanese == m_scriptcode) {
      while (*str) {
        if ((unsigned char) *str >= 0xA0 && (unsigned char) *str < 0xE0) {
          *str -= (0xA0 - 0x81);
        }
        else if ((unsigned char) *str >= 0x81 && (unsigned char) *str < 0xA0) {
          *str += (0xE0 - 0xA0);
        } 
        
        if (CharacterByteType((Ptr) str, 0, m_scriptcode) == smFirstByte) {
          ++str;
          if (!*str)
            break;
        }
      ++str;
      }
    }
  }

  return NS_OK;
}

nsresult nsCollationMac::CompareRawSortKey(const PRUint8* key1, PRUint32 len1, 
                                           const PRUint8* key2, PRUint32 len2, 
                                           PRInt32* result)
{
  *result = PL_strcmp((const char *)key1, (const char *)key2);
  return NS_OK;
}
