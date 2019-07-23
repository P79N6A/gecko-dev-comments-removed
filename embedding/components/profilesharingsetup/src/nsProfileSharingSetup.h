





































#ifndef nsProfileSharingSetup_h__
#define nsProfileSharingSetup_h__

#include "nsIProfileSharingSetup.h"
#include "nsString.h"


 
#define NS_PROFILESHARINGSETUP_CID \
{ 0x2f977d58, 0x5485, 0x11d4, { 0x87, 0xe2, 0x00, 0x10, 0xa4, 0xe7, 0x5e, 0xf2 } }

#define NS_PROFILESHARINGSETUP_CONTRACTID \
  "@mozilla.org/embedcomp/profile-sharing-setup;1"





class nsProfileSharingSetup: public nsIProfileSharingSetup
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROFILESHARINGSETUP

public:
                          nsProfileSharingSetup();

protected:
  virtual                 ~nsProfileSharingSetup();

protected:
  PRPackedBool            mSharingGloballyEnabled;
  nsString                mClientName;
};

#endif
