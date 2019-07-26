




#ifndef nsNameSpaceManager_h___
#define nsNameSpaceManager_h___

#include "nsDataHashtable.h"
#include "nsTArray.h"

#include "mozilla/StaticPtr.h"

class nsIAtom;
class nsAString;

#define kNameSpaceID_Unknown -1




static const int32_t kNameSpaceID_None = 0;
#define kNameSpaceID_XMLNS    1 // not really a namespace, but it needs to play the game
#define kNameSpaceID_XML      2
#define kNameSpaceID_XHTML    3
#define kNameSpaceID_XLink    4
#define kNameSpaceID_XSLT     5
#define kNameSpaceID_XBL      6
#define kNameSpaceID_MathML   7
#define kNameSpaceID_RDF      8
#define kNameSpaceID_XUL      9
#define kNameSpaceID_SVG      10
#define kNameSpaceID_LastBuiltin          10 // last 'built-in' namespace

class nsNameSpaceKey : public PLDHashEntryHdr
{
public:
  typedef const nsAString* KeyType;
  typedef const nsAString* KeyTypePointer;

  nsNameSpaceKey(KeyTypePointer aKey) : mKey(aKey)
  {
  }
  nsNameSpaceKey(const nsNameSpaceKey& toCopy) : mKey(toCopy.mKey)
  {
  }

  KeyType GetKey() const
  {
    return mKey;
  }
  bool KeyEquals(KeyType aKey) const
  {
    return mKey->Equals(*aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }
  static PLDHashNumber HashKey(KeyTypePointer aKey) {
    return mozilla::HashString(*aKey);
  }

  enum {
    ALLOW_MEMMOVE = true
  };

private:
  const nsAString* mKey;
};
 














class nsNameSpaceManager
{
public:
  virtual ~nsNameSpaceManager() {}

  virtual nsresult RegisterNameSpace(const nsAString& aURI,
                                     int32_t& aNameSpaceID);

  virtual nsresult GetNameSpaceURI(int32_t aNameSpaceID, nsAString& aURI);
  virtual int32_t GetNameSpaceID(const nsAString& aURI);

  virtual bool HasElementCreator(int32_t aNameSpaceID);

  static nsNameSpaceManager* GetInstance();
private:
  bool Init();
  nsresult AddNameSpace(const nsAString& aURI, const int32_t aNameSpaceID);

  nsDataHashtable<nsNameSpaceKey,int32_t> mURIToIDTable;
  nsTArray< nsAutoPtr<nsString> > mURIArray;

  static mozilla::StaticAutoPtr<nsNameSpaceManager> sInstance;
};
 
#endif 
