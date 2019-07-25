





































#include "nsID.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"
#include "nsIInputStream.h"
#include "nsUnicharInputStream.h"
#include "pratom.h"
#include "nsEnumeratorUtils.h"
#include "nsReadableUtils.h"
#include "nsPrintfCString.h"
#include "nsDependentString.h"

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




enum EParserState {
  eParserState_AwaitingKey,
  eParserState_Key,
  eParserState_AwaitingValue,
  eParserState_Value,
  eParserState_Comment
};

enum EParserSpecial {
  eParserSpecial_None,          
  eParserSpecial_Escaped,       
  eParserSpecial_Unicode        
};

class nsPropertiesParser
{
public:
  nsPropertiesParser(nsIPersistentProperties* aProps) :
    mHaveMultiLine(false), mState(eParserState_AwaitingKey),
    mProps(aProps) {}

  void FinishValueState(nsAString& aOldValue) {
    static const char trimThese[] = " \t";
    mKey.Trim(trimThese, false, true);

    
    PRUnichar backup_char;
    PRUint32 minLength = mMinLength;
    if (minLength)
    {
      backup_char = mValue[minLength-1];
      mValue.SetCharAt('x', minLength-1);
    }
    mValue.Trim(trimThese, false, true);
    if (minLength)
      mValue.SetCharAt(backup_char, minLength-1);

    mProps->SetStringProperty(NS_ConvertUTF16toUTF8(mKey), mValue, aOldValue);
    mSpecialState = eParserSpecial_None;
    WaitForKey();
  }

  EParserState GetState() { return mState; }

  static NS_METHOD SegmentWriter(nsIUnicharInputStream* aStream,
                                 void* aClosure,
                                 const PRUnichar *aFromSegment,
                                 PRUint32 aToOffset,
                                 PRUint32 aCount,
                                 PRUint32 *aWriteCount);

  nsresult ParseBuffer(const PRUnichar* aBuffer, PRUint32 aBufferLength);

private:
  bool ParseValueCharacter(
    PRUnichar c,                  
    const PRUnichar* cur,         
    const PRUnichar* &tokenStart, 
                                  
                                  
    nsAString& oldValue);         
                                  
                                  

  void WaitForKey() {
    mState = eParserState_AwaitingKey;
  }

  void EnterKeyState() {
    mKey.Truncate();
    mState = eParserState_Key;
  }

  void WaitForValue() {
    mState = eParserState_AwaitingValue;
  }

  void EnterValueState() {
    mValue.Truncate();
    mMinLength = 0;
    mState = eParserState_Value;
    mSpecialState = eParserSpecial_None;
  }

  void EnterCommentState() {
    mState = eParserState_Comment;
  }

  nsAutoString mKey;
  nsAutoString mValue;

  PRUint32  mUnicodeValuesRead; 
  PRUnichar mUnicodeValue;      
  bool      mHaveMultiLine;     
                                
                                
                                
                                
                                
                                
  bool      mMultiLineCanSkipN; 
  PRUint32  mMinLength;         
                                
  EParserState mState;
  
  EParserSpecial mSpecialState;
  nsIPersistentProperties* mProps;
};

inline bool IsWhiteSpace(PRUnichar aChar)
{
  return (aChar == ' ') || (aChar == '\t') ||
         (aChar == '\r') || (aChar == '\n');
}

inline bool IsEOL(PRUnichar aChar)
{
  return (aChar == '\r') || (aChar == '\n');
}


bool nsPropertiesParser::ParseValueCharacter(
    PRUnichar c, const PRUnichar* cur, const PRUnichar* &tokenStart,
    nsAString& oldValue)
{
  switch (mSpecialState) {

    
  case eParserSpecial_None:
    switch (c) {
    case '\\':
      if (mHaveMultiLine)
        
        mHaveMultiLine = false;
      else
        mValue += Substring(tokenStart, cur);

      mSpecialState = eParserSpecial_Escaped;
      break;

    case '\n':
      
      if (mHaveMultiLine && mMultiLineCanSkipN) {
        
        mMultiLineCanSkipN = false;
        
        
        
        
        tokenStart = cur+1;
        break;
      }
      

    case '\r':
      
      mValue += Substring(tokenStart, cur);
      FinishValueState(oldValue);
      mHaveMultiLine = false;
      break;

    default:
      
      
      if (mHaveMultiLine) {
        if (c == ' ' || c == '\t') {
          
          mMultiLineCanSkipN = false;
          
          
          
          
          tokenStart = cur+1;
          break;
        }
        mHaveMultiLine = false;
        tokenStart = cur;
      }
      break; 
    }
    break; 

    
  case eParserSpecial_Escaped:
    
    
    tokenStart = cur+1;
    mSpecialState = eParserSpecial_None;

    switch (c) {

      
    case 't':
      mValue += PRUnichar('\t');
      mMinLength = mValue.Length();
      break;
    case 'n':
      mValue += PRUnichar('\n');
      mMinLength = mValue.Length();
      break;
    case 'r':
      mValue += PRUnichar('\r');
      mMinLength = mValue.Length();
      break;
    case '\\':
      mValue += PRUnichar('\\');
      break;

      
    case 'u':
    case 'U':
      mSpecialState = eParserSpecial_Unicode;
      mUnicodeValuesRead = 0;
      mUnicodeValue = 0;
      break;

      
    case '\r':
    case '\n':
      mHaveMultiLine = true;
      mMultiLineCanSkipN = (c == '\r');
      mSpecialState = eParserSpecial_None;
      break;

    default:
      
      mValue += c;
      break;
    }
    break;

    
    
  case eParserSpecial_Unicode:

    if(('0' <= c) && (c <= '9'))
      mUnicodeValue =
        (mUnicodeValue << 4) | (c - '0');
    else if(('a' <= c) && (c <= 'f'))
      mUnicodeValue =
        (mUnicodeValue << 4) | (c - 'a' + 0x0a);
    else if(('A' <= c) && (c <= 'F'))
      mUnicodeValue =
        (mUnicodeValue << 4) | (c - 'A' + 0x0a);
    else {
      
      mValue += mUnicodeValue;
      mMinLength = mValue.Length();
      mSpecialState = eParserSpecial_None;

      
      tokenStart = cur;

      
      return false;
    }

    if (++mUnicodeValuesRead >= 4) {
      tokenStart = cur+1;
      mSpecialState = eParserSpecial_None;
      mValue += mUnicodeValue;
      mMinLength = mValue.Length();
    }

    break;
  }

  return true;
}

NS_METHOD nsPropertiesParser::SegmentWriter(nsIUnicharInputStream* aStream,
                                            void* aClosure,
                                            const PRUnichar *aFromSegment,
                                            PRUint32 aToOffset,
                                            PRUint32 aCount,
                                            PRUint32 *aWriteCount)
{
  nsPropertiesParser *parser = 
    static_cast<nsPropertiesParser *>(aClosure);
  
  parser->ParseBuffer(aFromSegment, aCount);

  *aWriteCount = aCount;
  return NS_OK;
}

nsresult nsPropertiesParser::ParseBuffer(const PRUnichar* aBuffer,
                                         PRUint32 aBufferLength)
{
  const PRUnichar* cur = aBuffer;
  const PRUnichar* end = aBuffer + aBufferLength;

  
  const PRUnichar* tokenStart = nsnull;

  
  
  if (mState == eParserState_Key ||
      mState == eParserState_Value) {
    tokenStart = aBuffer;
  }

  nsAutoString oldValue;

  while (cur != end) {

    PRUnichar c = *cur;

    switch (mState) {
    case eParserState_AwaitingKey:
      if (c == '#' || c == '!')
        EnterCommentState();

      else if (!IsWhiteSpace(c)) {
        
        EnterKeyState();
        tokenStart = cur;
      }
      break;

    case eParserState_Key:
      if (c == '=' || c == ':') {
        mKey += Substring(tokenStart, cur);
        WaitForValue();
      }
      break;

    case eParserState_AwaitingValue:
      if (IsEOL(c)) {
        
        EnterValueState();
        FinishValueState(oldValue);
      }

      
      else if (!IsWhiteSpace(c)) {
        tokenStart = cur;
        EnterValueState();

        
        if (ParseValueCharacter(c, cur, tokenStart, oldValue))
          cur++;
        
        
        
        
        continue;
      }
      break;

    case eParserState_Value:
      if (ParseValueCharacter(c, cur, tokenStart, oldValue))
        cur++;
      
      continue;

    case eParserState_Comment:
      
      if (c == '\r' || c== '\n')
        WaitForKey();
      break;
    }

    
    cur++;
  }

  
  
  if (mState == eParserState_Value && tokenStart &&
      mSpecialState == eParserSpecial_None) {
    mValue += Substring(tokenStart, cur);
  }
  
  else if (mState == eParserState_Key && tokenStart) {
    mKey += Substring(tokenStart, cur);
  }

  return NS_OK;
}

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

nsresult
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

NS_IMPL_THREADSAFE_ISUPPORTS2(nsPersistentProperties, nsIPersistentProperties, nsIProperties)

NS_IMETHODIMP
nsPersistentProperties::Load(nsIInputStream *aIn)
{
  nsresult rv = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(aIn, getter_AddRefs(mIn));

  if (rv != NS_OK) {
    NS_WARNING("Error creating UnicharInputStream");
    return NS_ERROR_FAILURE;
  }

  nsPropertiesParser parser(mSubclass);

  PRUint32 nProcessed;
  
  
  while (NS_SUCCEEDED(rv = mIn->ReadSegments(nsPropertiesParser::SegmentWriter, &parser, 4096, &nProcessed)) &&
         nProcessed != 0);
  mIn = nsnull;
  if (NS_FAILED(rv))
    return rv;

  
  
  if (parser.GetState() == eParserState_Value) {
    nsAutoString oldValue;  
    parser.FinishValueState(oldValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPersistentProperties::SetStringProperty(const nsACString& aKey,
                                          const nsAString& aNewValue,
                                          nsAString& aOldValue)
{
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
  else {
    aOldValue.Truncate();
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

static PLDHashOperator
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
nsPersistentProperties::Has(const char* prop, bool *result)
{
  PropertyTableEntry *entry =
    static_cast<PropertyTableEntry*>
               (PL_DHashTableOperate(&mTable, prop, PL_DHASH_LOOKUP));

  *result = (entry && PL_DHASH_ENTRY_IS_BUSY(entry));

  return NS_OK;
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

NS_IMPL_ISUPPORTS1(nsPropertyElement, nsIPropertyElement)

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



