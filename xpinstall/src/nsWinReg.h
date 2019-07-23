




































#ifndef __NS_WINREG_H__
#define __NS_WINREG_H__

#include "nsWinRegEnums.h" 
#include "nsWinRegValue.h"

#include "nscore.h"
#include "nsISupports.h"

#include "jsapi.h"

#include "nsString.h"
#include "nsHashtable.h"

#include "nsSoftwareUpdate.h"

#include "nsInstallObject.h"
#include "nsInstallVersion.h"
#include "nsInstall.h"

#define _MAXKEYVALUE_ 8196

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

class nsWinReg
{
  public:

    enum
    {
      NS_HKEY_CLASSES_ROOT          = 0x80000000,
      NS_HKEY_CURRENT_USER          = 0x80000001,
      NS_HKEY_LOCAL_MACHINE         = 0x80000002,
      NS_HKEY_USERS                 = 0x80000003
    };

    

    

                      nsWinReg(nsInstall* suObj);
                      ~nsWinReg();

    PRInt32           SetRootKey(PRInt32 key);
    PRInt32           CreateKey(const nsAString& subkey, const nsAString& classname, PRInt32* aReturn);
    PRInt32           DeleteKey(const nsAString& subkey, PRInt32* aReturn);
    PRInt32           DeleteValue(const nsAString& subkey, const nsAString& valname, PRInt32* aReturn);
    PRInt32           SetValueString(const nsAString& subkey, const nsAString& valname, const nsAString& value, PRInt32* aReturn);
    PRInt32           GetValueString(const nsString& subkey, const nsString& valname, nsString* aReturn);
    PRInt32           SetValueNumber(const nsString& subkey, const nsString& valname, PRInt32 value, PRInt32* aReturn);
    PRInt32           GetValueNumber(const nsString& subkey, const nsString& valname, PRInt32* aReturn);
    PRInt32           SetValue(const nsString& subkey, const nsString& valname, nsWinRegValue* value, PRInt32* aReturn);
    PRInt32           GetValue(const nsString& subkey, const nsString& valname, nsWinRegValue** aReturn);
    PRInt32           EnumValueNames(const nsString& keyname, PRInt32 index, nsString &aReturn);
    PRInt32           EnumKeys(const nsString& keyname, PRInt32 index, nsString &aReturn);

    nsInstall*        InstallObject(void);

    PRInt32           KeyExists(const nsString& subkey, PRInt32* aReturn);
    PRInt32           ValueExists(const nsString& subkey, const nsString& valname, PRInt32* aReturn);
    PRInt32           IsKeyWritable(const nsString& subkey, PRInt32* aReturn);
    PRInt32           PrepareCreateKey(PRInt32 root, const nsString& subkey, PRInt32* aReturn);
    PRInt32           PrepareDeleteKey(PRInt32 root, const nsString& subkey, PRInt32* aReturn);
    PRInt32           PrepareDeleteValue(PRInt32 root, const nsString& subkey, const nsString& valname, PRInt32* aReturn);
    PRInt32           PrepareSetValueString(PRInt32 root, const nsString& subkey, PRInt32* aReturn);
    PRInt32           PrepareSetValueNumber(PRInt32 root, const nsString& subkey, PRInt32* aReturn);
    PRInt32           PrepareSetValue(PRInt32 root, const nsString& subkey, PRInt32* aReturn);

    PRInt32           FinalCreateKey(PRInt32 root, const nsString& subkey, const nsString& classname, PRInt32* aReturn);
    PRInt32           FinalDeleteKey(PRInt32 root, const nsString& subkey, PRInt32* aReturn);
    PRInt32           FinalDeleteValue(PRInt32 root, const nsString& subkey, const nsString& valname, PRInt32* aReturn);
    PRInt32           FinalSetValueString(PRInt32 root, const nsString& subkey, const nsString& valname, const nsString& value, PRInt32* aReturn);
    PRInt32           FinalSetValueNumber(PRInt32 root, const nsString& subkey, const nsString& valname, PRInt32 value, PRInt32* aReturn);
    PRInt32           FinalSetValue(PRInt32 root, const nsString& subkey, const nsString& valname, nsWinRegValue* value, PRInt32* aReturn);

    
  private:
    
    
    PRInt32    mRootKey;
    nsInstall* mInstallObject;

    
    PRBool            NativeKeyExists(const nsString& subkey);
    PRBool            NativeValueExists(const nsString& subkey, const nsString& valname);
    PRBool            NativeIsKeyWritable(const nsString& subkey);
    PRInt32           NativeCreateKey(const nsString& subkey, const nsString& classname);
    PRInt32           NativeDeleteKey(const nsString& subkey);
    PRInt32           NativeDeleteValue(const nsString& subkey, const nsString& valname);

    PRInt32           NativeSetValueString(const nsString& subkey, const nsString& valname, const nsString& value);
    PRInt32           NativeGetValueString(const nsString& subkey, const nsString& valname, nsString* aReturn);
    PRInt32           NativeSetValueNumber(const nsString& subkey, const nsString& valname, PRInt32 value);
    PRInt32           NativeGetValueNumber(const nsString& subkey, const nsString& valname, PRInt32* aReturn);

    PRInt32           NativeSetValue(const nsString& subkey, const nsString& valname, nsWinRegValue* value);
    nsWinRegValue*    NativeGetValue(const nsString& subkey, const nsString& valname);
};

#endif

