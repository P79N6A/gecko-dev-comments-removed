




































#ifndef nsWinProfile_h__
#define nsWinProfile_h__

#include "prtypes.h"

#include "nsInstall.h"

class nsWinProfile
{
  public:

    

    

    nsWinProfile( nsInstall* suObj, const nsString& folder, const nsString& file );
    ~nsWinProfile(); 

    







    PRInt32 WriteString( nsString section, nsString key, nsString value, PRInt32* aReturn );
    
    





    PRInt32 GetString( nsString section, nsString key, nsString* aReturn );
    
    nsString& GetFilename();
    nsInstall* InstallObject();
    
    PRInt32 FinalWriteString( nsString section, nsString key, nsString value );

    
  private:
    
    
    nsString   mFilename;
    nsInstall* mInstallObject;
    
    
    PRInt32 NativeWriteString( nsString section, nsString key, nsString value );
    PRInt32 NativeGetString( nsString section, nsString key, nsString* aReturn );
};

#endif 
