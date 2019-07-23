





































#include "nsMorkReader.h"
#include "prio.h"
#include "nsNetUtil.h"
#include "nsVoidArray.h"


class nsCLineString : public nsFixedCString
{
public:
  nsCLineString() : fixed_string_type(mStorage, sizeof(mStorage), 0) {}
  explicit nsCLineString(const substring_type& str)
    : fixed_string_type(mStorage, sizeof(mStorage), 0)
  {
    Assign(str);
  }

private:
  char_type mStorage[160];
};



inline PRBool
ConvertChar(char *c)
{
  char c1 = *c;
  if ('0' <= c1 && c1 <= '9') {
    *c = c1 - '0';
    return PR_TRUE;
  }
  if ('A' <= c1 && c1 <= 'F') {
    *c = c1 - 'A' + 10;
    return PR_TRUE;
  }
  return PR_FALSE;
}





static void
MorkUnescape(const nsCSubstring &aString, nsCString &aResult)
{
  PRUint32 len = aString.Length();

  
  
  
  
  if (!EnsureStringLength(aResult, len)) {
    aResult.Truncate();
    return; 
  }

  char *result = aResult.BeginWriting();
  const char *source = aString.BeginReading();
  const char *sourceEnd = source + len;

  const char *startPos = nsnull;
  PRUint32 bytes;
  for (; source < sourceEnd; ++source) {
    char c = *source;
    if (c == '\\') {
      if (startPos) {
        bytes = source - startPos;
        memcpy(result, startPos, bytes);
        result += bytes;
        startPos = nsnull;
      }
      if (source < sourceEnd - 1) {
        *(result++) = *(++source);
      }
    } else if (c == '$') {
      if (startPos) {
        bytes = source - startPos;
        memcpy(result, startPos, bytes);
        result += bytes;
        startPos = nsnull;
      }
      if (source < sourceEnd - 2) {
        
        
        char c2 = *(++source);
        char c3 = *(++source);
        if (ConvertChar(&c2) && ConvertChar(&c3)) {
          *(result++) = ((c2 << 4) | c3);
        }
      }
    } else if (!startPos) {
      startPos = source;
    }
  }
  if (startPos) {
    bytes = source - startPos;
    memcpy(result, startPos, bytes);
    result += bytes;
  }
  aResult.SetLength(result - aResult.BeginReading());
}

nsresult
nsMorkReader::Init()
{
  NS_ENSURE_TRUE(mValueMap.Init(), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mTable.Init(), NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
DeleteStringArray(const nsCSubstring& aKey,
                  nsTArray<nsCString> *aData,
                  void *aUserArg)
{
  delete aData;
  return PL_DHASH_NEXT;
}

nsMorkReader::~nsMorkReader()
{
  mTable.EnumerateRead(DeleteStringArray, nsnull);
}

struct AddColumnClosure
{
  AddColumnClosure(nsTArray<nsMorkReader::MorkColumn> *a,
                   nsMorkReader::IndexMap *c)
    : array(a), columnMap(c), result(NS_OK) {}

  nsTArray<nsMorkReader::MorkColumn> *array;
  nsMorkReader::IndexMap *columnMap;
  nsresult result;
};

PR_STATIC_CALLBACK(PLDHashOperator)
AddColumn(const nsCSubstring &id, nsCString name, void *userData)
{
  AddColumnClosure *closure = NS_STATIC_CAST(AddColumnClosure*, userData);
  nsTArray<nsMorkReader::MorkColumn> *array = closure->array;

  if (!array->AppendElement(nsMorkReader::MorkColumn(id, name)) ||
      !closure->columnMap->Put(id, array->Length() - 1)) {
    closure->result = NS_ERROR_OUT_OF_MEMORY;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

nsresult
nsMorkReader::Read(nsIFile *aFile)
{
  nsCOMPtr<nsIFileInputStream> stream =
    do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID);
  NS_ENSURE_TRUE(stream, NS_ERROR_FAILURE);

  nsresult rv = stream->Init(aFile, PR_RDONLY, 0, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  mStream = do_QueryInterface(stream);
  NS_ASSERTION(mStream, "file input stream must impl nsILineInputStream");

  nsCLineString line;
  rv = ReadLine(line);
  if (!line.EqualsLiteral("// <!-- <mdb:mork:z v=\"1.4\"/> -->")) {
    return NS_ERROR_FAILURE; 
  }

  IndexMap columnMap;
  NS_ENSURE_TRUE(columnMap.Init(), NS_ERROR_OUT_OF_MEMORY);

  while (NS_SUCCEEDED(ReadLine(line))) {
    
    PRUint32 idx = 0, len = line.Length();
    while (idx < len && line[idx] == ' ') {
      ++idx;
    }
    if (idx >= len) {
      continue;
    }

    const nsCSubstring &l = Substring(line, idx);

    
    if (StringBeginsWith(l, NS_LITERAL_CSTRING("< <(a=c)>"))) {
      
      StringMap columnNameMap;
      NS_ENSURE_TRUE(columnNameMap.Init(), NS_ERROR_OUT_OF_MEMORY);

      rv = ParseMap(l, &columnNameMap);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      
      mColumns.SetCapacity(columnNameMap.Count());

      AddColumnClosure closure(&mColumns, &columnMap);
      columnNameMap.EnumerateRead(AddColumn, &closure);
      if (NS_FAILED(closure.result)) {
        return closure.result;
      }
    } else if (StringBeginsWith(l, NS_LITERAL_CSTRING("<("))) {
      
      rv = ParseMap(l, &mValueMap);
      NS_ENSURE_SUCCESS(rv, rv);
    } else if (l[0] == '{' || l[0] == '[') {
      
      rv = ParseTable(l, columnMap);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
    }
  }

  return NS_OK;
}

void
nsMorkReader::EnumerateRows(RowEnumerator aCallback, void *aUserData) const
{
  
  typedef const nsDataHashtable<IDKey, const nsTArray<nsCString>* > ConstTable;
  NS_REINTERPRET_CAST(ConstTable*, &mTable)->EnumerateRead(aCallback,
                                                           aUserData);
}




nsresult
nsMorkReader::ParseMap(const nsCSubstring &aLine, StringMap *aMap)
{
  nsCLineString line(aLine);
  nsCAutoString key;
  nsresult rv = NS_OK;

  
  if (StringBeginsWith(line, NS_LITERAL_CSTRING("< <(a=c)>"))) {
    rv = ReadLine(line);
  }

  for (; NS_SUCCEEDED(rv); rv = ReadLine(line)) {
    PRUint32 idx = 0;
    PRUint32 len = line.Length();
    PRUint32 tokenStart;

    while (idx < len) {
      switch (line[idx++]) {
      case '(':
        
        if (!key.IsEmpty()) {
          NS_WARNING("unterminated key/value pair?");
          key.Truncate(0);
        }

        tokenStart = idx;
        while (idx < len && line[idx] != '=') {
          ++idx;
        }
        key = Substring(line, tokenStart, idx - tokenStart);
        break;
      case '=':
        {
          
          if (key.IsEmpty()) {
            NS_WARNING("stray value");
            break;
          }

          tokenStart = idx;
          while (idx < len && line[idx] != ')') {
            if (line[idx] == '\\') {
              ++idx; 
            }
            ++idx;
          }
          PRUint32 tokenEnd = PR_MIN(idx, len);
          ++idx;

          nsCString value;
          MorkUnescape(Substring(line, tokenStart, tokenEnd - tokenStart),
                       value);
          aMap->Put(key, value);
          key.Truncate(0);
          break;
        }
      case '>':
        
        NS_WARN_IF_FALSE(key.IsEmpty(),
                         "map terminates inside of key/value pair");
        return NS_OK;
      }
    }
  }

  
  
  NS_WARNING("didn't find end of key/value map");
  return NS_ERROR_FAILURE;
}






nsresult
nsMorkReader::ParseTable(const nsCSubstring &aLine, const IndexMap &aColumnMap)
{
  nsCLineString line(aLine);
  const PRUint32 columnCount = mColumns.Length(); 

  PRInt32 columnIndex = -1; 
  
  nsTArray<nsCString> *currentRow = nsnull;
  PRBool inMetaRow = PR_FALSE;

  do {
    PRUint32 idx = 0;
    PRUint32 len = line.Length();
    PRUint32 tokenStart, tokenEnd;

    while (idx < len) {
      switch (line[idx++]) {
      case '{':
        
        
        
        while (idx < len && line[idx] != '[') {
          if (line[idx] == '{') {
            inMetaRow = PR_TRUE; 
          } else if (line[idx] == '}') {
            inMetaRow = PR_FALSE;
          }
          ++idx;
        }
        break;
      case '[':
        {
          
          
          
          
          if (currentRow) {
            NS_WARNING("unterminated row?");
            currentRow = nsnull;
          }

          
          
          
          PRBool cutColumns;
          if (idx < len && line[idx] == '-') {
            cutColumns = PR_TRUE;
            ++idx;
          } else {
            cutColumns = PR_FALSE;
          }

          tokenStart = idx;
          while (idx < len &&
                 line[idx] != '(' &&
                 line[idx] != ']' &&
                 line[idx] != ':') {
            ++idx;
          }
          tokenEnd = idx;
          while (idx < len && line[idx] != '(' && line[idx] != ']') {
            ++idx;
          }
          
          if (inMetaRow) {
            mMetaRow = NewVoidStringArray(columnCount);
            NS_ENSURE_TRUE(mMetaRow, NS_ERROR_OUT_OF_MEMORY);
            currentRow = mMetaRow;
          } else {
            const nsCSubstring& row = Substring(line, tokenStart,
                                                tokenEnd - tokenStart);
            if (!mTable.Get(row, &currentRow)) {
              currentRow = NewVoidStringArray(columnCount);
              NS_ENSURE_TRUE(currentRow, NS_ERROR_OUT_OF_MEMORY);

              NS_ENSURE_TRUE(mTable.Put(row, currentRow),
                             NS_ERROR_OUT_OF_MEMORY);
            }
          }
          if (cutColumns) {
            
            
            for (PRUint32 i = 0; i < columnCount; ++i) {
              currentRow->ElementAt(i).SetIsVoid(PR_TRUE);
            }
          }
          break;
        }
      case ']':
        
        currentRow = nsnull;
        inMetaRow = PR_FALSE;
        break;
      case '(':
        {
          if (!currentRow) {
            NS_WARNING("cell value outside of row");
            break;
          }

          NS_WARN_IF_FALSE(columnIndex == -1, "unterminated cell?");

          PRBool columnIsAtom;
          if (line[idx] == '^') {
            columnIsAtom = PR_TRUE;
            ++idx; 
          } else {
            columnIsAtom = PR_FALSE;
          }
          tokenStart = idx;
          while (idx < len && line[idx] != '^' && line[idx] != '=') {
            if (line[idx] == '\\') {
              ++idx; 
            }
            ++idx;
          }

          tokenEnd = PR_MIN(idx, len);

          nsCAutoString column;
          const nsCSubstring &colValue =
            Substring(line, tokenStart, tokenEnd - tokenStart);
          if (columnIsAtom) {
            column.Assign(colValue);
          } else {
            MorkUnescape(colValue, column);
          }

          if (!aColumnMap.Get(colValue, &columnIndex)) {
            NS_WARNING("Column not in column map, discarding it");
            columnIndex = -1;
          }
        }
        break;
      case '=':
      case '^':
        {
          if (columnIndex == -1) {
            NS_WARNING("stray ^ or = marker");
            break;
          }

          PRBool valueIsAtom = (line[idx - 1] == '^');
          tokenStart = idx - 1;  
          while (idx < len && line[idx] != ')') {
            if (line[idx] == '\\') {
              ++idx; 
            }
            ++idx;
          }
          tokenEnd = PR_MIN(idx, len);
          ++idx;

          const nsCSubstring &value =
            Substring(line, tokenStart, tokenEnd - tokenStart);
          if (valueIsAtom) {
            (*currentRow)[columnIndex] = value;
          } else {
            nsCAutoString value2;
            MorkUnescape(value, value2);
            (*currentRow)[columnIndex] = value2;
          }
          columnIndex = -1;
        }
        break;
      }
    }
  } while (currentRow && NS_SUCCEEDED(ReadLine(line)));

  return NS_OK;
}

nsresult
nsMorkReader::ReadLine(nsCString &aLine)
{
  PRBool res;
  nsresult rv = mStream->ReadLine(aLine, &res);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!res) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  while (!aLine.IsEmpty() &&  aLine.Last() == '\\') {
    
    nsCLineString line2;
    rv = mStream->ReadLine(line2, &res);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!res) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    aLine.Truncate(aLine.Length() - 1);
    aLine.Append(line2);
  }

  return NS_OK;
}

void
nsMorkReader::NormalizeValue(nsCString &aValue) const
{
  PRUint32 len = aValue.Length();
  if (len == 0) {
    return;
  }
  const nsCSubstring &str = Substring(aValue, 1, len - 1);
  char c = aValue[0];
  if (c == '^') {
    if (!mValueMap.Get(str, &aValue)) {
      aValue.Truncate(0);
    }
  } else if (c == '=') {
    aValue.Assign(str);
  } else {
    aValue.Truncate(0);
  }
}

 nsTArray<nsCString>*
nsMorkReader::NewVoidStringArray(PRInt32 aCount)
{
  nsAutoPtr< nsTArray<nsCString> > array(new nsTArray<nsCString>(aCount));
  NS_ENSURE_TRUE(array, nsnull);

  for (PRInt32 i = 0; i < aCount; ++i) {
    nsCString *elem = array->AppendElement();
    NS_ENSURE_TRUE(elem, nsnull);
    elem->SetIsVoid(PR_TRUE);
  }

  return array.forget();
}
