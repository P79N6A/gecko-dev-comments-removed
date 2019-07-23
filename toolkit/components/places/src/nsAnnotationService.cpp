





































#include "nsAnnotationService.h"
#include "mozStorageCID.h"
#include "nsNavHistory.h"
#include "nsNetUtil.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageHelper.h"
#include "nsIServiceManager.h"
#include "nsIVariant.h"
#include "nsString.h"
#include "nsVariant.h"

const PRInt32 nsAnnotationService::kAnnoIndex_ID = 0;
const PRInt32 nsAnnotationService::kAnnoIndex_Page = 1;
const PRInt32 nsAnnotationService::kAnnoIndex_Name = 2;
const PRInt32 nsAnnotationService::kAnnoIndex_MimeType = 3;
const PRInt32 nsAnnotationService::kAnnoIndex_Content = 4;
const PRInt32 nsAnnotationService::kAnnoIndex_Flags = 5;
const PRInt32 nsAnnotationService::kAnnoIndex_Expiration = 6;

nsAnnotationService* nsAnnotationService::gAnnotationService;

NS_IMPL_ISUPPORTS1(nsAnnotationService,
                   nsIAnnotationService)



nsAnnotationService::nsAnnotationService()
{
  NS_ASSERTION(!gAnnotationService,
               "ATTEMPTING TO CREATE TWO INSTANCES OF THE ANNOTATION SERVICE!");
  gAnnotationService = this;
}




nsAnnotationService::~nsAnnotationService()
{
  NS_ASSERTION(gAnnotationService == this,
               "Deleting a non-singleton annotation service");
  if (gAnnotationService == this)
    gAnnotationService = nsnull;
}




nsresult
nsAnnotationService::Init()
{
  nsresult rv;

  
  
  
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  if (! history)
    return NS_ERROR_FAILURE;
  mDBConn = history->GetStorageConnection();

  

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_annos "
      "SET mime_type = ?4, content = ?5, flags = ?6, expiration = ?7 "
      "WHERE id = ?1"),
    getter_AddRefs(mDBSetAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT * "
      "FROM moz_annos "
      "WHERE place_id = ?1 AND anno_attribute_id = "
      "(SELECT id FROM moz_anno_attributes WHERE name = ?2)"),
    getter_AddRefs(mDBGetAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT n.name "
      "FROM moz_annos a LEFT JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
      "WHERE a.place_id = ?1"),
    getter_AddRefs(mDBGetAnnotationNames));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT a.id, a.place_id, ?2, a.mime_type, a.content, a.flags, a.expiration "
      "FROM moz_places h JOIN moz_annos a ON h.id = a.place_id "
      "WHERE h.url = ?1 AND a.anno_attribute_id = "
      "(SELECT id FROM moz_anno_attributes WHERE name = ?2)"),
    getter_AddRefs(mDBGetAnnotationFromURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_anno_attributes WHERE name = ?1"),
    getter_AddRefs(mDBGetAnnotationNameID));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_anno_attributes (name) VALUES (?1)"),
    getter_AddRefs(mDBAddAnnotationName));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_annos "
      "(place_id, anno_attribute_id, mime_type, content, flags, expiration) "
      "VALUES (?2, ?3, ?4, ?5, ?6, ?7)"),
    getter_AddRefs(mDBAddAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_annos WHERE place_id = ?1 AND anno_attribute_id = "
      "(SELECT id FROM moz_anno_attributes WHERE name = ?2)"),
    getter_AddRefs(mDBRemoveAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}











nsresult 
nsAnnotationService::InitTables(mozIStorageConnection* aDBConn)
{
  nsresult rv;
  PRBool exists;
  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_annos"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_annos ("
        "id INTEGER PRIMARY KEY,"
        "place_id INTEGER NOT NULL,"
        "anno_attribute_id INTEGER,"
        "mime_type VARCHAR(32) DEFAULT NULL,"
        "content LONGVARCHAR, flags INTEGER DEFAULT 0,"
        "expiration INTEGER DEFAULT 0)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_annos_place_idindex ON moz_annos (place_id)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_annos_attributesindex ON moz_annos (anno_attribute_id)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_anno_attributes"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE TABLE moz_anno_attributes ("
        "id INTEGER PRIMARY KEY,"
        "name VARCHAR(32) UNIQUE NOT NULL)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_anno_attributes_nameindex ON moz_anno_attributes (name)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::SetAnnotationString(nsIURI* aURI,
                                         const nsACString& aName,
                                         const nsAString& aValue,
                                         PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; 
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindStringParameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  
  
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::SetAnnotationInt32(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt32 aValue,
                                        PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; 
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindInt32Parameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  
  
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::SetAnnotationInt64(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt64 aValue,
                                        PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; 
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindInt64Parameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  
  
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::SetAnnotationDouble(nsIURI* aURI,
                                         const nsACString& aName,
                                         double aValue,
                                         PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; 
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindDoubleParameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  
  
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::SetAnnotationBinary(nsIURI* aURI,
                                         const nsACString& aName,
                                         const PRUint8 *aData,
                                         PRUint32 aDataLen,
                                         const nsACString& aMimeType,
                                         PRInt32 aFlags, PRInt32 aExpiration)
{
  if (aMimeType.Length() == 0)
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; 
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindBlobParameter(kAnnoIndex_Content, aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(kAnnoIndex_MimeType, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  
  
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationString(nsIURI* aURI,
                                         const nsACString& aName,
                                         nsAString& _retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  rv = mDBGetAnnotationFromURI->GetString(kAnnoIndex_Content, _retval);
  mDBGetAnnotationFromURI->Reset();
  return rv;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationInt32(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt32 *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  *_retval = mDBGetAnnotationFromURI->AsInt32(kAnnoIndex_Content);
  mDBGetAnnotationFromURI->Reset();
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationInt64(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt64 *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  *_retval = mDBGetAnnotationFromURI->AsInt64(kAnnoIndex_Content);
  mDBGetAnnotationFromURI->Reset();
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationDouble(nsIURI* aURI,
                                         const nsACString& aName,
                                         double *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  *_retval = mDBGetAnnotationFromURI->AsDouble(kAnnoIndex_Content);
  mDBGetAnnotationFromURI->Reset();
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationBinary(nsIURI* aURI,
                                         const nsACString& aName,
                                         PRUint8** aData, PRUint32* aDataLen,
                                         nsACString& aMimeType)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  rv = mDBGetAnnotationFromURI->GetBlob(kAnnoIndex_Content, aDataLen, aData);
  if (NS_FAILED(rv)) {
    mDBGetAnnotationFromURI->Reset();
    return rv;
  }
  rv = mDBGetAnnotationFromURI->GetUTF8String(kAnnoIndex_MimeType, aMimeType);
  mDBGetAnnotationFromURI->Reset();
  return rv;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationInfo(nsIURI* aURI,
                                       const nsACString& aName,
                                       PRInt32 *aFlags, PRInt32 *aExpiration,
                                       nsACString& aMimeType,
                                       PRInt32 *aStorageType)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  mozStorageStatementScoper resetter(mDBGetAnnotationFromURI);

  *aFlags = mDBGetAnnotationFromURI->AsInt32(kAnnoIndex_Flags);
  *aExpiration = mDBGetAnnotationFromURI->AsInt32(kAnnoIndex_Expiration);
  rv = mDBGetAnnotationFromURI->GetUTF8String(kAnnoIndex_MimeType, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBGetAnnotationFromURI->GetTypeOfIndex(kAnnoIndex_Content, aStorageType);
  return rv;
}




NS_IMETHODIMP
nsAnnotationService::GetPagesWithAnnotation(const nsACString& aName,
                                            PRUint32* aResultCount,
                                            nsIURI*** aResults)
{
  if (aName.IsEmpty() || ! aResultCount || ! aResults)
    return NS_ERROR_INVALID_ARG;
  *aResultCount = 0;
  *aResults = nsnull;
  nsCOMArray<nsIURI> results;

  nsresult rv = GetPagesWithAnnotationCOMArray(aName, &results);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (results.Count() == 0)
    return NS_OK;
  *aResults = NS_STATIC_CAST(nsIURI**,
                             nsMemory::Alloc(results.Count() * sizeof(nsIURI*)));
  if (! aResults)
    return NS_ERROR_OUT_OF_MEMORY;
  *aResultCount = results.Count();
  for (PRUint32 i = 0; i < *aResultCount; i ++) {
    (*aResults)[i] = results[i];
    NS_ADDREF((*aResults)[i]);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsAnnotationService::GetPagesWithAnnotationCOMArray(
    const nsACString& aName, nsCOMArray<nsIURI>* aResults){
  
  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT h.url FROM moz_anno_attributes n "
    "LEFT JOIN moz_annos a ON n.id = a.anno_attribute_id "
    "LEFT JOIN moz_places h ON a.place_id = h.id "
    "WHERE n.name = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindUTF8StringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(rv = statement->ExecuteStep(&hasMore)) && hasMore) {
    nsCAutoString uristring;
    rv = statement->GetUTF8String(0, uristring);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), uristring);
    if (NS_FAILED(rv))
      continue;
    PRBool added = aResults->AppendObject(uri);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsAnnotationService::GetPageAnnotationNames(nsIURI* aURI, PRUint32* aCount,
                                            nsIVariant*** _result)
{
  *aCount = 0;
  *_result = nsnull;

  nsTArray<nsCString> names;
  nsresult rv = GetPageAnnotationNamesTArray(aURI, &names);
  NS_ENSURE_SUCCESS(rv, rv);
  if (names.Length() == 0)
    return NS_OK;

  *_result = NS_STATIC_CAST(nsIVariant**,
      nsMemory::Alloc(sizeof(nsIVariant*) * names.Length()));
  NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < names.Length(); i ++) {
    nsCOMPtr<nsIWritableVariant> var = new nsVariant;
    if (! var) {
      
      for (PRUint32 j = 0; j < i; j ++)
        NS_RELEASE((*_result)[j]);
      nsMemory::Free(*_result);
      *_result = nsnull;
      return rv;
    }
    var->SetAsAUTF8String(names[i]);
    NS_ADDREF((*_result)[i] = var);
  }
  *aCount = names.Length();

  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::GetPageAnnotationNamesTArray(nsIURI* aURI,
                                                  nsTArray<nsCString>* aResult)
{
  aResult->Clear();

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRInt64 uriID;
  nsresult rv = history->GetUrlIdFor(aURI, &uriID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (uriID == 0) 
    return NS_OK;

  mozStorageStatementScoper scoper(mDBGetAnnotationNames);
  rv = mDBGetAnnotationNames->BindInt64Parameter(0, uriID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  nsCAutoString name;
  while (NS_SUCCEEDED(mDBGetAnnotationNames->ExecuteStep(&hasResult)) &&
         hasResult) {
    rv = mDBGetAnnotationNames->GetUTF8String(0, name);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! aResult->AppendElement(name))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::HasAnnotation(nsIURI* aURI,
                                   const nsACString& aName,
                                   PRBool *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    *_retval = PR_FALSE;
    rv = NS_OK;
  } else if (NS_SUCCEEDED(rv)) {
    *_retval = PR_TRUE;
  }
  mDBGetAnnotationFromURI->Reset();
  return rv;
}








NS_IMETHODIMP
nsAnnotationService::RemoveAnnotation(nsIURI* aURI,
                                      const nsACString& aName)
{
  nsresult rv;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRInt64 uriID;
  rv = history->GetUrlIdFor(aURI, &uriID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (uriID == 0) 
    return NS_OK;

  mozStorageStatementScoper resetter(mDBRemoveAnnotation);

  rv = mDBRemoveAnnotation->BindInt64Parameter(0, uriID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBRemoveAnnotation->BindUTF8StringParameter(1, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBRemoveAnnotation->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  resetter.Abandon();

  
  for (PRInt32 i = 0; i < mObservers.Count(); i ++)
    mObservers[i]->OnAnnotationRemoved(aURI, aName);

  return NS_OK;
}








NS_IMETHODIMP
nsAnnotationService::RemovePageAnnotations(nsIURI* aURI)
{
  nsresult rv;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_UNEXPECTED);

  PRInt64 uriID;
  rv = history->GetUrlIdFor(aURI, &uriID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (uriID == 0)
    return NS_OK; 

  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_annos WHERE place_id = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(0, uriID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (PRInt32 i = 0; i < mObservers.Count(); i ++)
    mObservers[i]->OnAnnotationRemoved(aURI, EmptyCString());
  return NS_OK;
}

















NS_IMETHODIMP
nsAnnotationService::CopyAnnotations(nsIURI* aSourceURI, nsIURI* aDestURI,
                                     PRBool aOverwriteDest)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsTArray<nsCString> sourceNames;
  nsresult rv = GetPageAnnotationNamesTArray(aSourceURI, &sourceNames);
  NS_ENSURE_SUCCESS(rv, rv);
  if (sourceNames.Length() == 0)
    return NS_OK; 

  
  nsTArray<nsCString> destNames;
  rv = GetPageAnnotationNamesTArray(aDestURI, &destNames);
  NS_ENSURE_SUCCESS(rv, rv);

  
#ifdef DEBUG
  if (sourceNames.Length() > 10 || destNames.Length() > 10) {
    NS_WARNING("There are a lot of annotations, copying them may be inefficient.");
  }
#endif

  if (aOverwriteDest) {
    
    for (PRUint32 i = 0; i < sourceNames.Length(); i ++) {
      PRUint32 destIndex = destNames.IndexOf(sourceNames[i]);
      if (destIndex != destNames.NoIndex) {
        destNames.RemoveElementAt(destIndex);
        RemoveAnnotation(aDestURI, sourceNames[i]);
      }
    }
  } else {
    
    for (PRUint32 i = 0; i < destNames.Length(); i ++) {
      PRUint32 sourceIndex = sourceNames.IndexOf(destNames[i]);
      if (sourceIndex != sourceNames.NoIndex)
        sourceNames.RemoveElementAt(sourceIndex);
    }
  }

  
  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_annos (place_id, anno_attribute_id, mime_type, content, flags, expiration) "
      "SELECT ?1, anno_attribute_id, mime_type, content, flags, expiration "
      "FROM moz_annos WHERE place_id = ?2 AND anno_attribute_id = "
      "(SELECT id FROM moz_anno_attributes WHERE name = ?3)"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  PRInt64 sourceID, destID;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  rv = history->GetUrlIdFor(aSourceURI, &sourceID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(sourceID, NS_ERROR_UNEXPECTED); 

  rv = history->GetUrlIdFor(aSourceURI, &destID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(destID, NS_ERROR_UNEXPECTED); 

  
  
  for (PRUint32 i = 0; i < sourceNames.Length(); i ++) {
    statement->BindInt64Parameter(0, sourceID);
    statement->BindInt64Parameter(1, destID);
    statement->BindUTF8StringParameter(2, sourceNames[i]);
    rv = statement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  transaction.Commit();
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::AddObserver(nsIAnnotationObserver* aObserver)
{
  if (mObservers.IndexOfObject(aObserver) >= 0)
    return NS_ERROR_INVALID_ARG; 
  if (!mObservers.AppendObject(aObserver))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::RemoveObserver(nsIAnnotationObserver* aObserver)
{
  if (!mObservers.RemoveObject(aObserver))
    return NS_ERROR_INVALID_ARG;
  return NS_OK;
}




NS_IMETHODIMP
nsAnnotationService::GetAnnotationURI(nsIURI* aURI, const nsACString& aName,
                                      nsIURI** _result)
{
  if (aName.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  nsCAutoString annoSpec;
  nsresult rv = aURI->GetSpec(annoSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  spec.AssignLiteral("moz-anno:");
  spec += aName;
  spec += NS_LITERAL_CSTRING(":");
  spec += annoSpec;

  return NS_NewURI(_result, spec);
}









nsresult
nsAnnotationService::HasAnnotationInternal(PRInt64 aURLID,
                                           const nsACString& aName,
                                           PRBool* hasAnnotation,
                                           PRInt64* annotationID)
{
  mozStorageStatementScoper resetter(mDBGetAnnotation);
  nsresult rv;

  rv = mDBGetAnnotation->BindInt64Parameter(0, aURLID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBGetAnnotation->BindUTF8StringParameter(1, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBGetAnnotation->ExecuteStep(hasAnnotation);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! annotationID || ! *hasAnnotation)
    return NS_OK;

  return mDBGetAnnotation->GetInt64(0, annotationID);
}









nsresult
nsAnnotationService::StartGetAnnotationFromURI(nsIURI* aURI,
                                               const nsACString& aName)
{
  mozStorageStatementScoper statementResetter(mDBGetAnnotationFromURI);
  nsresult rv;

  rv = BindStatementURI(mDBGetAnnotationFromURI, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBGetAnnotationFromURI->BindUTF8StringParameter(1, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult = PR_FALSE;
  rv = mDBGetAnnotationFromURI->ExecuteStep(&hasResult);
  if (NS_FAILED(rv) || ! hasResult)
    return NS_ERROR_NOT_AVAILABLE;

  
  
  statementResetter.Abandon();
  return NS_OK;
}















nsresult
nsAnnotationService::StartSetAnnotation(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt32 aFlags, PRInt32 aExpiration,
                                        mozIStorageStatement** aStatement)
{
  nsresult rv;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRInt64 uriID;
  rv = history->GetUrlIdFor(aURI, &uriID, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasAnnotation;
  PRInt64 annotationID;
  rv = HasAnnotationInternal(uriID, aName, &hasAnnotation, &annotationID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (hasAnnotation) {
    *aStatement = mDBSetAnnotation;
    rv = (*aStatement)->BindInt64Parameter(kAnnoIndex_ID, annotationID);
  } else {
    *aStatement = mDBAddAnnotation;

    
    {
      mozStorageStatementScoper scoper(mDBGetAnnotationNameID);
      rv = mDBGetAnnotationNameID->BindUTF8StringParameter(0, aName);
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool hasName;
      PRInt64 nameID;
      if (NS_FAILED(mDBGetAnnotationNameID->ExecuteStep(&hasName)) || ! hasName) {
        
        mDBGetAnnotationNameID->Reset();
        mozStorageStatementScoper addNameScoper(mDBAddAnnotationName);
        rv = mDBAddAnnotationName->BindUTF8StringParameter(0, aName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mDBAddAnnotationName->Execute();
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDBConn->GetLastInsertRowID(&nameID);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        nameID = mDBGetAnnotationNameID->AsInt64(0);
      }
      rv = (*aStatement)->BindInt64Parameter(kAnnoIndex_Page, uriID);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = (*aStatement)->BindInt64Parameter(kAnnoIndex_Name, nameID);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  mozStorageStatementScoper statementResetter(*aStatement);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = (*aStatement)->BindInt32Parameter(kAnnoIndex_Flags, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = (*aStatement)->BindInt32Parameter(kAnnoIndex_Expiration, aExpiration);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  statementResetter.Abandon();
  return NS_OK;
}




void
nsAnnotationService::CallSetObservers(nsIURI* aURI, const nsACString& aName)
{
  for (PRInt32 i = 0; i < mObservers.Count(); i ++)
    mObservers[i]->OnAnnotationSet(aURI, aName);
}
