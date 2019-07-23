





































#include "nsID.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"
#include "nsIInputStream.h"
#include "nsUnicharInputStream.h"
#include "pratom.h"
#include "nsEnumeratorUtils.h"
#include "nsReadableUtils.h"
#include "nsPrintfCString.h"

#define PL_ARENA_CONST_ALIGN_MASK 3
#include "nsPersistentProperties.h"
#include "nsIProperties.h"
#include "nsISupportsArray.h"
#include "nsProperties.h"

struct PropertyTableEntry : public PLDHashEntryHdr
{
  
  const char *mKey;
  const PRUnichar *mValue;
};

static PRUnichar*
ArenaStrdup(const nsAFlatString& aString, PLArenaPool* aArena)
{
  void *mem;
  
  PRInt32 len = (aString.Length()+1) * sizeof(PRUnichar);
  PL_ARENA_ALLOCATE(mem, aArena, len);
  NS_ASSERTION(mem, "Couldn't allocate space!\n");
  if (mem) {
    memcpy(mem, aString.get(), len);
  }
  return static_cast<PRUnichar*>(mem);
}

static char*
ArenaStrdup(const nsAFlatCString& aString, PLArenaPool* aArena)
{
  void *mem;
  
  PRInt32 len = (aString.Length()+1) * sizeof(char);
  PL_ARENA_ALLOCATE(mem, aArena, len);
  NS_ASSERTION(mem, "Couldn't allocate space!\n");
  if (mem)
    memcpy(mem, aString.get(), len);
  return static_cast<char*>(mem);
}

static const struct PLDHashTableOps property_HashTableOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashStringKey,
  PL_DHashMatchStringKey,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  nsnull,
};

nsPersistentProperties::nsPersistentProperties()
: mIn(nsnull)
{
  mSubclass = static_cast<nsIPersistentProperties*>(this);
  mTable.ops = nsnull;
  PL_INIT_ARENA_POOL(&mArena, "PersistentPropertyArena", 2048);
}

nsPersistentProperties::~nsPersistentProperties()
{
  PL_FinishArenaPool(&mArena);
  if (mTable.ops)
    PL_DHashTableFinish(&mTable);
}

nsresult
nsPersistentProperties::Init()
{
  if (!PL_DHashTableInit(&mTable, &property_HashTableOps, nsnull,
                         sizeof(PropertyTableEntry), 20)) {
    mTable.ops = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

NS_METHOD
nsPersistentProperties::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;
  nsPersistentProperties* props = new nsPersistentProperties();
  if (props == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(props);
  nsresult rv = props->Init();
  if (NS_SUCCEEDED(rv))
    rv = props->QueryInterface(aIID, aResult);

  NS_RELEASE(props);
  return rv;
}

NS_IMETHODIMP
nsPersistentProperties::Load(nsIInputStream *aIn)
{
  PRInt32  c;
  nsresult ret = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(aIn, &mIn);

  if (ret != NS_OK) {
    NS_WARNING("NS_NewUTF8ConverterStream failed");
    return NS_ERROR_FAILURE;
  }
  c = Read();
  while (1) {
    c = SkipWhiteSpace(c);
    if (c < 0) {
      break;
    }
    else if ((c == '#') || (c == '!')) {
      c = SkipLine(c);
      continue;
    }
    else {
      nsAutoString key;
      while ((c >= 0) && (c != '=') && (c != ':')) {
        key.Append(PRUnichar(c));
        c = Read();
      }
      if (c < 0) {
        break;
      }
      static const char trimThese[] = " \t";
      key.Trim(trimThese, PR_FALSE, PR_TRUE);
      c = Read();
      nsAutoString value, tempValue;
      while ((c >= 0) && (c != '\r') && (c != '\n')) {
        if (c == '\\') {
          c = Read();
          switch(c) {
            case '\r':
            case '\n':
              
              
              
              if (c == '\r')
                c = Read();
              if (c == '\n')
                c = Read();
              while (c == ' ' || c == '\t')
                c = Read();
              continue;
            default:
              tempValue.Append((PRUnichar) '\\');
              tempValue.Append((PRUnichar) c);
          } 
        } else {
          tempValue.Append((PRUnichar) c);
        }
        c = Read();
      }
      tempValue.Trim(trimThese, PR_TRUE, PR_TRUE);

      PRUint32 state  = 0;
      PRUnichar uchar = 0;
      for (PRUint32 i = 0; i < tempValue.Length(); ++i) {
        PRUnichar ch = tempValue[i];
        switch(state) {
          case 0:
           if (ch == '\\') {
             ++i;
             if (i == tempValue.Length())
               break;
             ch = tempValue[i];
             switch(ch) {
               case 'u':
               case 'U':
                 state = 1;
                 uchar=0;
                 break;
               case 't':
                 value.Append(PRUnichar('\t'));
                 break;
               case 'n':
                 value.Append(PRUnichar('\n'));
                 break;
               case 'r':
                 value.Append(PRUnichar('\r'));
                 break;
               default:
                 value.Append(ch);
             } 
           } else {
             value.Append(ch);
           }
           continue;
         case 1:
         case 2:
         case 3:
         case 4:
           if (('0' <= ch) && (ch <= '9')) {
               uchar = (uchar << 4) | (ch - '0');
              state++;
              continue;
           }
           if (('a' <= ch) && (ch <= 'f')) {
              uchar = (uchar << 4) | (ch - 'a' + 0x0a);
              state++;
              continue;
           }
           if (('A' <= ch) && (ch <= 'F')) {
              uchar = (uchar << 4) | (ch - 'A' + 0x0a);
              state++;
              continue;
           }
           
         case 5:
           value.Append((PRUnichar) uchar);
           state = 0;
           --i;
           break;
        }
      }
      if (state != 0) {
        value.Append((PRUnichar) uchar);
        state = 0;
      }
      
      nsAutoString oldValue;
      mSubclass->SetStringProperty(NS_ConvertUTF16toUTF8(key), value, oldValue);
    }
  }
  mIn->Close();
  NS_RELEASE(mIn);

  return NS_OK;
}

NS_IMETHODIMP
nsPersistentProperties::SetStringProperty(const nsACString& aKey,
                                          const nsAString& aNewValue,
                                          nsAString& aOldValue)
{
#if 0
  cout << "will add " << aKey.get() << "=" <<
    NS_LossyConvertUCS2ToASCII(aNewValue).get() << endl;
#endif

  const nsAFlatCString&  flatKey = PromiseFlatCString(aKey);
  PropertyTableEntry *entry =
    static_cast<PropertyTableEntry*>
               (PL_DHashTableOperate(&mTable, flatKey.get(), PL_DHASH_ADD));

  if (entry->mKey) {
    aOldValue = entry->mValue;
    NS_WARNING(nsPrintfCString(aKey.Length() + 30,
                               "the property %s already exists\n",
                               flatKey.get()).get());
  }

  entry->mKey = ArenaStrdup(flatKey, &mArena);
  entry->mValue = ArenaStrdup(PromiseFlatString(aNewValue), &mArena);

  return NS_OK;
}

NS_IMETHODIMP
nsPersistentProperties::Save(nsIOutputStream* aOut, const nsACString& aHeader)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPersistentProperties::Subclass(nsIPersistentProperties* aSubclass)
{
  if (aSubclass) {
    mSubclass = aSubclass;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPersistentProperties::GetStringProperty(const nsACString& aKey,
                                          nsAString& aValue)
{
  const nsAFlatCString&  flatKey = PromiseFlatCString(aKey);

  PropertyTableEntry *entry =
    static_cast<PropertyTableEntry*>
               (PL_DHashTableOperate(&mTable, flatKey.get(), PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(entry))
    return NS_ERROR_FAILURE;

  aValue = entry->mValue;
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
AddElemToArray(PLDHashTable* table, PLDHashEntryHdr *hdr,
               PRUint32 i, void *arg)
{
  nsISupportsArray  *propArray = (nsISupportsArray *) arg;
  PropertyTableEntry* entry =
    static_cast<PropertyTableEntry*>(hdr);

  nsPropertyElement *element =
    new nsPropertyElement(nsDependentCString(entry->mKey),
                          nsDependentString(entry->mValue));
  if (!element)
     return PL_DHASH_STOP;

  propArray->InsertElementAt(element, i);

  return PL_DHASH_NEXT;
}


NS_IMETHODIMP
nsPersistentProperties::Enumerate(nsISimpleEnumerator** aResult)
{
  nsCOMPtr<nsISupportsArray> propArray;
  nsresult rv = NS_NewISupportsArray(getter_AddRefs(propArray));
  if (NS_FAILED(rv))
    return rv;

  
  if (!propArray->SizeTo(mTable.entryCount))
   return NS_ERROR_OUT_OF_MEMORY;

  
  PRUint32 n =
    PL_DHashTableEnumerate(&mTable, AddElemToArray, (void *)propArray);
  if (n < mTable.entryCount)
      return NS_ERROR_OUT_OF_MEMORY;

  return NS_NewArrayEnumerator(aResult, propArray);
}


PRInt32
nsPersistentProperties::Read()
{
  PRUnichar  c;
  PRUint32  nRead;
  nsresult  ret;

  ret = mIn->Read(&c, 1, &nRead);
  if (ret == NS_OK && nRead == 1) {
    return c;
  }

  return -1;
}

#define IS_WHITE_SPACE(c) \
  (((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n'))

PRInt32
nsPersistentProperties::SkipWhiteSpace(PRInt32 c)
{
  while (IS_WHITE_SPACE(c)) {
    c = Read();
  }

  return c;
}

PRInt32
nsPersistentProperties::SkipLine(PRInt32 c)
{
  while ((c >= 0) && (c != '\r') && (c != '\n')) {
    c = Read();
  }
  if (c == '\r') {
    c = Read();
  }
  if (c == '\n') {
    c = Read();
  }

  return c;
}





NS_IMETHODIMP
nsPersistentProperties::Get(const char* prop, const nsIID & uuid, void* *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPersistentProperties::Set(const char* prop, nsISupports* value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsPersistentProperties::Undefine(const char* prop)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPersistentProperties::Has(const char* prop, PRBool *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPersistentProperties::GetKeys(PRUint32 *count, char ***keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}






NS_METHOD
nsPropertyElement::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;
  nsPropertyElement* propElem = new nsPropertyElement();
  if (propElem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(propElem);
  nsresult rv = propElem->QueryInterface(aIID, aResult);
  NS_RELEASE(propElem);
  return rv;
}

NS_IMETHODIMP
nsPropertyElement::GetKey(nsACString& aReturnKey)
{
  aReturnKey = mKey;
  return NS_OK;
}

NS_IMETHODIMP
nsPropertyElement::GetValue(nsAString& aReturnValue)
{
  aReturnValue = mValue;
  return NS_OK;
}

NS_IMETHODIMP
nsPropertyElement::SetKey(const nsACString& aKey)
{
  mKey = aKey;
  return NS_OK;
}

NS_IMETHODIMP
nsPropertyElement::SetValue(const nsAString& aValue)
{
  mValue = aValue;
  return NS_OK;
}



