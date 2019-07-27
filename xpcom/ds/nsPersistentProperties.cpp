





#include "nsArrayEnumerator.h"
#include "nsID.h"
#include "nsCOMArray.h"
#include "nsUnicharInputStream.h"
#include "nsPrintfCString.h"
#include "nsAutoPtr.h"

#define PL_ARENA_CONST_ALIGN_MASK 3
#include "nsPersistentProperties.h"
#include "nsIProperties.h"

struct PropertyTableEntry : public PLDHashEntryHdr
{
  
  const char* mKey;
  const char16_t* mValue;
};

static char16_t*
ArenaStrdup(const nsAFlatString& aString, PLArenaPool* aArena)
{
  void* mem;
  
  int32_t len = (aString.Length() + 1) * sizeof(char16_t);
  PL_ARENA_ALLOCATE(mem, aArena, len);
  NS_ASSERTION(mem, "Couldn't allocate space!\n");
  if (mem) {
    memcpy(mem, aString.get(), len);
  }
  return static_cast<char16_t*>(mem);
}

static char*
ArenaStrdup(const nsAFlatCString& aString, PLArenaPool* aArena)
{
  void* mem;
  
  int32_t len = (aString.Length() + 1) * sizeof(char);
  PL_ARENA_ALLOCATE(mem, aArena, len);
  NS_ASSERTION(mem, "Couldn't allocate space!\n");
  if (mem) {
    memcpy(mem, aString.get(), len);
  }
  return static_cast<char*>(mem);
}

static const struct PLDHashTableOps property_HashTableOps = {
  PL_DHashStringKey,
  PL_DHashMatchStringKey,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr,
};




enum EParserState
{
  eParserState_AwaitingKey,
  eParserState_Key,
  eParserState_AwaitingValue,
  eParserState_Value,
  eParserState_Comment
};

enum EParserSpecial
{
  eParserSpecial_None,          
  eParserSpecial_Escaped,       
  eParserSpecial_Unicode        
};

class MOZ_STACK_CLASS nsPropertiesParser
{
public:
  explicit nsPropertiesParser(nsIPersistentProperties* aProps)
    : mHaveMultiLine(false)
    , mState(eParserState_AwaitingKey)
    , mProps(aProps)
  {
  }

  void FinishValueState(nsAString& aOldValue)
  {
    static const char trimThese[] = " \t";
    mKey.Trim(trimThese, false, true);

    
    char16_t backup_char;
    uint32_t minLength = mMinLength;
    if (minLength) {
      backup_char = mValue[minLength - 1];
      mValue.SetCharAt('x', minLength - 1);
    }
    mValue.Trim(trimThese, false, true);
    if (minLength) {
      mValue.SetCharAt(backup_char, minLength - 1);
    }

    mProps->SetStringProperty(NS_ConvertUTF16toUTF8(mKey), mValue, aOldValue);
    mSpecialState = eParserSpecial_None;
    WaitForKey();
  }

  EParserState GetState() { return mState; }

  static NS_METHOD SegmentWriter(nsIUnicharInputStream* aStream,
                                 void* aClosure,
                                 const char16_t* aFromSegment,
                                 uint32_t aToOffset,
                                 uint32_t aCount,
                                 uint32_t* aWriteCount);

  nsresult ParseBuffer(const char16_t* aBuffer, uint32_t aBufferLength);

private:
  bool ParseValueCharacter(
    char16_t aChar,               
    const char16_t* aCur,         
    const char16_t*& aTokenStart, 
                                  
                                  
    nsAString& aOldValue);        
                                  
                                  

  void WaitForKey()
  {
    mState = eParserState_AwaitingKey;
  }

  void EnterKeyState()
  {
    mKey.Truncate();
    mState = eParserState_Key;
  }

  void WaitForValue()
  {
    mState = eParserState_AwaitingValue;
  }

  void EnterValueState()
  {
    mValue.Truncate();
    mMinLength = 0;
    mState = eParserState_Value;
    mSpecialState = eParserSpecial_None;
  }

  void EnterCommentState()
  {
    mState = eParserState_Comment;
  }

  nsAutoString mKey;
  nsAutoString mValue;

  uint32_t  mUnicodeValuesRead; 
  char16_t mUnicodeValue;      
  bool      mHaveMultiLine;     
                                
                                
                                
                                
                                
                                
  bool      mMultiLineCanSkipN; 
  uint32_t  mMinLength;         
                                
  EParserState mState;
  
  EParserSpecial mSpecialState;
  nsCOMPtr<nsIPersistentProperties> mProps;
};

inline bool
IsWhiteSpace(char16_t aChar)
{
  return (aChar == ' ') || (aChar == '\t') ||
         (aChar == '\r') || (aChar == '\n');
}

inline bool
IsEOL(char16_t aChar)
{
  return (aChar == '\r') || (aChar == '\n');
}


bool
nsPropertiesParser::ParseValueCharacter(char16_t aChar, const char16_t* aCur,
                                        const char16_t*& aTokenStart,
                                        nsAString& aOldValue)
{
  switch (mSpecialState) {
    
    case eParserSpecial_None:
      switch (aChar) {
        case '\\':
          if (mHaveMultiLine) {
            
            mHaveMultiLine = false;
          } else {
            mValue += Substring(aTokenStart, aCur);
          }

          mSpecialState = eParserSpecial_Escaped;
          break;

        case '\n':
          
          if (mHaveMultiLine && mMultiLineCanSkipN) {
            
            mMultiLineCanSkipN = false;
            
            
            
            
            aTokenStart = aCur + 1;
            break;
          }
        

        case '\r':
          
          mValue += Substring(aTokenStart, aCur);
          FinishValueState(aOldValue);
          mHaveMultiLine = false;
          break;

        default:
          
          
          if (mHaveMultiLine) {
            if (aChar == ' ' || aChar == '\t') {
              
              mMultiLineCanSkipN = false;
              
              
              
              
              aTokenStart = aCur + 1;
              break;
            }
            mHaveMultiLine = false;
            aTokenStart = aCur;
          }
          break; 
      }
      break; 

    
    case eParserSpecial_Escaped:
      
      
      aTokenStart = aCur + 1;
      mSpecialState = eParserSpecial_None;

      switch (aChar) {
        
        case 't':
          mValue += char16_t('\t');
          mMinLength = mValue.Length();
          break;
        case 'n':
          mValue += char16_t('\n');
          mMinLength = mValue.Length();
          break;
        case 'r':
          mValue += char16_t('\r');
          mMinLength = mValue.Length();
          break;
        case '\\':
          mValue += char16_t('\\');
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
          mMultiLineCanSkipN = (aChar == '\r');
          mSpecialState = eParserSpecial_None;
          break;

        default:
          
          mValue += aChar;
          break;
      }
      break;

    
    
    case eParserSpecial_Unicode:
      if ('0' <= aChar && aChar <= '9') {
        mUnicodeValue =
          (mUnicodeValue << 4) | (aChar - '0');
      } else if ('a' <= aChar && aChar <= 'f') {
        mUnicodeValue =
          (mUnicodeValue << 4) | (aChar - 'a' + 0x0a);
      } else if ('A' <= aChar && aChar <= 'F') {
        mUnicodeValue =
          (mUnicodeValue << 4) | (aChar - 'A' + 0x0a);
      } else {
        
        mValue += mUnicodeValue;
        mMinLength = mValue.Length();
        mSpecialState = eParserSpecial_None;

        
        aTokenStart = aCur;

        
        return false;
      }

      if (++mUnicodeValuesRead >= 4) {
        aTokenStart = aCur + 1;
        mSpecialState = eParserSpecial_None;
        mValue += mUnicodeValue;
        mMinLength = mValue.Length();
      }

      break;
  }

  return true;
}

NS_METHOD
nsPropertiesParser::SegmentWriter(nsIUnicharInputStream* aStream,
                                  void* aClosure,
                                  const char16_t* aFromSegment,
                                  uint32_t aToOffset,
                                  uint32_t aCount,
                                  uint32_t* aWriteCount)
{
  nsPropertiesParser* parser = static_cast<nsPropertiesParser*>(aClosure);
  parser->ParseBuffer(aFromSegment, aCount);

  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
nsPropertiesParser::ParseBuffer(const char16_t* aBuffer,
                                uint32_t aBufferLength)
{
  const char16_t* cur = aBuffer;
  const char16_t* end = aBuffer + aBufferLength;

  
  const char16_t* tokenStart = nullptr;

  
  
  if (mState == eParserState_Key ||
      mState == eParserState_Value) {
    tokenStart = aBuffer;
  }

  nsAutoString oldValue;

  while (cur != end) {

    char16_t c = *cur;

    switch (mState) {
      case eParserState_AwaitingKey:
        if (c == '#' || c == '!') {
          EnterCommentState();
        }

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

          
          if (ParseValueCharacter(c, cur, tokenStart, oldValue)) {
            cur++;
          }
          
          
          
          
          continue;
        }
        break;

      case eParserState_Value:
        if (ParseValueCharacter(c, cur, tokenStart, oldValue)) {
          cur++;
        }
        
        continue;

      case eParserState_Comment:
        
        if (c == '\r' || c == '\n') {
          WaitForKey();
        }
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
  : mIn(nullptr)
{
  PL_DHashTableInit(&mTable, &property_HashTableOps,
                    sizeof(PropertyTableEntry), 16);

  PL_INIT_ARENA_POOL(&mArena, "PersistentPropertyArena", 2048);
}

nsPersistentProperties::~nsPersistentProperties()
{
  PL_FinishArenaPool(&mArena);
  if (mTable.IsInitialized()) {
    PL_DHashTableFinish(&mTable);
  }
}

nsresult
nsPersistentProperties::Create(nsISupports* aOuter, REFNSIID aIID,
                               void** aResult)
{
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }
  nsRefPtr<nsPersistentProperties> props = new nsPersistentProperties();
  return props->QueryInterface(aIID, aResult);
}

NS_IMPL_ISUPPORTS(nsPersistentProperties, nsIPersistentProperties, nsIProperties)

NS_IMETHODIMP
nsPersistentProperties::Load(nsIInputStream* aIn)
{
  nsresult rv = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(aIn, getter_AddRefs(mIn));

  if (rv != NS_OK) {
    NS_WARNING("Error creating UnicharInputStream");
    return NS_ERROR_FAILURE;
  }

  nsPropertiesParser parser(this);

  uint32_t nProcessed;
  
  
  while (NS_SUCCEEDED(rv = mIn->ReadSegments(nsPropertiesParser::SegmentWriter,
                                             &parser, 4096, &nProcessed)) &&
         nProcessed != 0);
  mIn = nullptr;
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
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
  PropertyTableEntry* entry = static_cast<PropertyTableEntry*>(
    PL_DHashTableAdd(&mTable, flatKey.get(), mozilla::fallible));

  if (entry->mKey) {
    aOldValue = entry->mValue;
    NS_WARNING(nsPrintfCString("the property %s already exists\n",
                               flatKey.get()).get());
  } else {
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
nsPersistentProperties::GetStringProperty(const nsACString& aKey,
                                          nsAString& aValue)
{
  const nsAFlatCString&  flatKey = PromiseFlatCString(aKey);

  PropertyTableEntry* entry = static_cast<PropertyTableEntry*>(
    PL_DHashTableSearch(&mTable, flatKey.get()));

  if (!entry) {
    return NS_ERROR_FAILURE;
  }

  aValue = entry->mValue;
  return NS_OK;
}

static PLDHashOperator
AddElemToArray(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
               uint32_t aIndex, void* aArg)
{
  nsCOMArray<nsIPropertyElement>* props =
    static_cast<nsCOMArray<nsIPropertyElement>*>(aArg);
  PropertyTableEntry* entry =
    static_cast<PropertyTableEntry*>(aHdr);

  nsPropertyElement* element =
    new nsPropertyElement(nsDependentCString(entry->mKey),
                          nsDependentString(entry->mValue));

  props->AppendObject(element);

  return PL_DHASH_NEXT;
}


NS_IMETHODIMP
nsPersistentProperties::Enumerate(nsISimpleEnumerator** aResult)
{
  nsCOMArray<nsIPropertyElement> props;

  
  props.SetCapacity(mTable.EntryCount());

  
  uint32_t n = PL_DHashTableEnumerate(&mTable, AddElemToArray, (void*)&props);
  if (n < mTable.EntryCount()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_NewArrayEnumerator(aResult, props);
}





NS_IMETHODIMP
nsPersistentProperties::Get(const char* aProp, const nsIID& aUUID,
                            void** aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPersistentProperties::Set(const char* aProp, nsISupports* value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsPersistentProperties::Undefine(const char* aProp)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPersistentProperties::Has(const char* aProp, bool* aResult)
{
  *aResult = !!PL_DHashTableSearch(&mTable, aProp);
  return NS_OK;
}

NS_IMETHODIMP
nsPersistentProperties::GetKeys(uint32_t* aCount, char*** aKeys)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_METHOD
nsPropertyElement::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }
  nsRefPtr<nsPropertyElement> propElem = new nsPropertyElement();
  return propElem->QueryInterface(aIID, aResult);
}

NS_IMPL_ISUPPORTS(nsPropertyElement, nsIPropertyElement)

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


