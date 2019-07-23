





































#include "nsMorkReader.h"
#include "nsNavHistory.h"
#include "mozStorageHelper.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsTArray.h"


enum {
  kURLColumn,
  kNameColumn,
  kVisitCountColumn,
  kHiddenColumn,
  kTypedColumn,
  kLastVisitColumn,
  kColumnCount 
};

static const char * const gColumnNames[] = {
  "URL", "Name", "VisitCount", "Hidden", "Typed", "LastVisitDate"
};

struct TableReadClosure
{
  TableReadClosure(nsMorkReader *aReader, nsNavHistory *aHistory)
    : reader(aReader), history(aHistory), swapBytes(PR_FALSE),
      byteOrderColumn(-1)
  {
    for (PRUint32 i = 0; i < kColumnCount; ++i) {
      columnIndexes[i] = -1;
    }
  }

  
  const nsMorkReader *reader;
  nsNavHistory *history;

  
  PRBool swapBytes;

  
  PRInt32 columnIndexes[kColumnCount];
  PRInt32 byteOrderColumn;
};




static void
SwapBytes(PRUnichar *buffer)
{
  for (PRUnichar *b = buffer; *b; ++b) {
    PRUnichar c = *b;
    *b = (c << 8) | ((c >> 8) & 0xff);
  }
}


static PLDHashOperator PR_CALLBACK
AddToHistoryCB(const nsCSubstring &aRowID,
               const nsTArray<nsCString> *aValues,
               void *aData)
{
  TableReadClosure *data = static_cast<TableReadClosure*>(aData);
  const nsMorkReader *reader = data->reader;
  nsCString values[kColumnCount];
  const PRInt32 *columnIndexes = data->columnIndexes;

  for (PRInt32 i = 0; i < kColumnCount; ++i) {
    if (columnIndexes[i] != -1) {
      values[i] = (*aValues)[columnIndexes[i]];
      reader->NormalizeValue(values[i]);
    }
  }

  
  nsCString &titleC = values[kNameColumn];

  PRUint32 titleLength;
  const char *titleBytes;
  if (titleC.IsEmpty()) {
    titleBytes = "\0";
    titleLength = 0;
  } else {
    titleLength = titleC.Length() / 2;

    
    
    titleC.Append('\0');

    
    if (data->swapBytes) {
      SwapBytes(reinterpret_cast<PRUnichar*>(titleC.BeginWriting()));
    }
    titleBytes = titleC.get();
  }

  const PRUnichar *title = reinterpret_cast<const PRUnichar*>(titleBytes);

  PRInt32 err;
  PRInt32 count = values[kVisitCountColumn].ToInteger(&err);
  if (err != 0 || count == 0) {
    count = 1;
  }

  PRTime date;
  if (PR_sscanf(values[kLastVisitColumn].get(), "%lld", &date) != 1) {
    date = -1;
  }

  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), values[kURLColumn]);

  if (uri) {
    PRBool isTyped = values[kTypedColumn].EqualsLiteral("1");
    PRInt32 transition = isTyped ?
        (PRInt32) nsINavHistoryService::TRANSITION_TYPED
      : (PRInt32) nsINavHistoryService::TRANSITION_LINK;
    nsNavHistory *history = data->history;

    history->AddPageWithVisit(uri,
                              nsDependentString(title, titleLength),
                              values[kHiddenColumn].EqualsLiteral("1"),
                              isTyped, count, transition, date);
  }
  return PL_DHASH_NEXT;
}







NS_IMETHODIMP
nsNavHistory::ImportHistory(nsIFile* aFile)
{
  NS_ENSURE_TRUE(aFile, NS_ERROR_NULL_POINTER);

  
  PRBool exists;
  aFile->Exists(&exists);
  if (!exists) {
    return NS_OK;
  }
  
  
  nsMorkReader reader;
  nsresult rv = reader.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = reader.Read(aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  TableReadClosure data(&reader, this);
  const nsTArray<nsMorkReader::MorkColumn> &columns = reader.GetColumns();
  for (PRUint32 i = 0; i < columns.Length(); ++i) {
    const nsCSubstring &name = columns[i].name;
    for (PRUint32 j = 0; j < kColumnCount; ++j) {
      if (name.Equals(gColumnNames[j])) {
        data.columnIndexes[j] = i;
        break;
      }
    }
    if (name.EqualsLiteral("ByteOrder")) {
      data.byteOrderColumn = i;
    }
  }

  
  const nsTArray<nsCString> *metaRow = reader.GetMetaRow();
  if (metaRow && data.byteOrderColumn != -1) {
    const nsCString &byteOrder = (*metaRow)[data.byteOrderColumn];
    if (!byteOrder.IsVoid()) {
      
      
      
      
      
      nsCAutoString byteOrderValue(byteOrder);
      reader.NormalizeValue(byteOrderValue);
#ifdef IS_LITTLE_ENDIAN
      data.swapBytes = byteOrderValue.EqualsLiteral("BE");
#else
      data.swapBytes = byteOrderValue.EqualsLiteral("LE");
#endif
    }
  }

  
  mozIStorageConnection *conn = GetStorageConnection();
  NS_ENSURE_TRUE(conn, NS_ERROR_NOT_INITIALIZED);
  mozStorageTransaction transaction(conn, PR_FALSE);
#ifdef IN_MEMORY_LINKS
  mozIStorageConnection *memoryConn = GetMemoryStorageConnection();
  mozStorageTransaction memTransaction(memoryConn, PR_FALSE);
#endif

  reader.EnumerateRows(AddToHistoryCB, &data);

  
  rv = RemoveDuplicateURIs();
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef IN_MEMORY_LINKS
  memTransaction.Commit();
#endif
  return transaction.Commit();
}
