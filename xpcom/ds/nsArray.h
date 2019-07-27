





#ifndef nsArray_h__
#define nsArray_h__

#include "nsIMutableArray.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"


#define NS_ARRAY_CID \
{ 0x35c66fd1, 0x95e9, 0x4e0a, \
  { 0x80, 0xc5, 0xc3, 0xbd, 0x2b, 0x37, 0x54, 0x81 } }


class nsArrayBase : public nsIMutableArray
{
public:
  NS_DECL_NSIARRAY
  NS_DECL_NSIMUTABLEARRAY

  


  static already_AddRefed<nsIMutableArray> Create();
  

  static nsresult XPCOMConstructor(nsISupports* aOuter, const nsIID& aIID,
                                   void** aResult);
protected:
  nsArrayBase() {}
  nsArrayBase(const nsArrayBase& aOther);
  explicit nsArrayBase(const nsCOMArray_base& aBaseArray) : mArray(aBaseArray) {}
  virtual ~nsArrayBase();

  nsCOMArray_base mArray;
};

class nsArray final : public nsArrayBase
{
  friend class nsArrayBase;

public:
  NS_DECL_ISUPPORTS

private:
  nsArray() : nsArrayBase() {}
  nsArray(const nsArray& aOther);
  explicit nsArray(const nsCOMArray_base& aBaseArray) : nsArrayBase(aBaseArray) {}
  ~nsArray() {}
};

class nsArrayCC final : public nsArrayBase
{
  friend class nsArrayBase;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsArrayCC)

private:
  nsArrayCC() : nsArrayBase() {}
  nsArrayCC(const nsArrayCC& aOther);
  explicit nsArrayCC(const nsCOMArray_base& aBaseArray) : nsArrayBase(aBaseArray) {}
  ~nsArrayCC() {}
};

#endif
