





































#ifndef nsMorkReader_h_
#define nsMorkReader_h_

#include "nsDataHashtable.h"
#include "nsILineInputStream.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"










class nsMorkReader
{
 public:
  
  
  class IDString : public nsFixedCString
  {
  public:
    IDString() : fixed_string_type(mStorage, sizeof(mStorage), 0) {}
    IDString(const substring_type &str) :
      fixed_string_type(mStorage, sizeof(mStorage), 0)
    {
      Assign(str);
    }

  private:
    char_type mStorage[9];
  };

  
  class IDKey : public PLDHashEntryHdr
  {
  public:
    typedef const nsCSubstring& KeyType;
    typedef const nsCSubstring* KeyTypePointer;

    IDKey(KeyTypePointer aStr) : mStr(*aStr) { }
    IDKey(const IDKey& toCopy) : mStr(toCopy.mStr) { }
    ~IDKey() { }

    KeyType GetKey() const { return mStr; }
    PRBool KeyEquals(const KeyTypePointer aKey) const
    {
      return mStr.Equals(*aKey);
    }

    static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
    static PLDHashNumber HashKey(const KeyTypePointer aKey)
    {
      return HashString(*aKey);
    }
    enum { ALLOW_MEMMOVE = PR_FALSE };

  private:
    const IDString mStr;
  };

  
  typedef nsDataHashtable<IDKey,nsCString> StringMap;

  
  
  typedef nsDataHashtable<IDKey,PRInt32> IndexMap;

  
  struct MorkColumn
  {
    MorkColumn(const nsCSubstring &i, const nsCSubstring &n)
      : id(i), name(n) {}

    IDString id;
    nsCString name;
  };

  
  
  
  
  
  
  
  
  
  
  typedef PLDHashOperator
  (*PR_CALLBACK RowEnumerator)(const nsCSubstring &rowID, 
                               const nsTArray<nsCString> *values,
                               void *userData);

  
  nsresult Init();

  
  
  nsresult Read(nsIFile *aFile);

  
  const nsTArray<MorkColumn>& GetColumns() const { return mColumns; }

  
  void EnumerateRows(RowEnumerator aCallback, void *aUserData) const;

  
  
  
  
  const nsTArray<nsCString>* GetMetaRow() const { return mMetaRow; }

  
  
  void NormalizeValue(nsCString &aValue) const;

  nsMorkReader() {}
  ~nsMorkReader();

private:
  
  
  
  nsresult ParseMap(const nsCSubstring &aLine, StringMap *aMap);

  
  
  
  
  
  nsresult ParseTable(const nsCSubstring &aLine, const IndexMap &aColumnMap);

  
  
  nsresult ReadLine(nsCString &aLine);

  
  
  static nsTArray<nsCString>* NewVoidStringArray(PRInt32 aSize);

  nsTArray<MorkColumn> mColumns;
  StringMap mValueMap;
  nsAutoPtr< nsTArray<nsCString> > mMetaRow;
  nsDataHashtable< IDKey,nsTArray<nsCString>* > mTable;
  nsCOMPtr<nsILineInputStream> mStream;
  nsCString mEmptyString; 
};

#endif 
