





#ifndef __NS_PK11TOKENDB_H__
#define __NS_PK11TOKENDB_H__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsISupports.h"
#include "nsIPK11TokenDB.h"
#include "nsIPK11Token.h"
#include "nsNSSHelper.h"
#include "pk11func.h"
#include "nsNSSShutDown.h"

class nsPK11Token : public nsIPK11Token,
                    public nsNSSShutDownObject
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPK11TOKEN

  explicit nsPK11Token(PK11SlotInfo *slot);
  

protected:
  virtual ~nsPK11Token();

private:
  friend class nsPK11TokenDB;
  void refreshTokenInfo();

  nsString mTokenName;
  nsString mTokenLabel, mTokenManID, mTokenHWVersion, mTokenFWVersion;
  nsString mTokenSerialNum;
  PK11SlotInfo *mSlot;
  int mSeries;
  nsCOMPtr<nsIInterfaceRequestor> mUIContext;
  virtual void virtualDestroyNSSReference();
  void destructorSafeDestroyNSSReference();
};

class nsPK11TokenDB : public nsIPK11TokenDB
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPK11TOKENDB

  nsPK11TokenDB();

protected:
  virtual ~nsPK11TokenDB();
  
};

#define NS_PK11TOKENDB_CID \
{ 0xb084a2ce, 0x1dd1, 0x11b2, \
  { 0xbf, 0x10, 0x83, 0x24, 0xf8, 0xe0, 0x65, 0xcc }}

#endif
