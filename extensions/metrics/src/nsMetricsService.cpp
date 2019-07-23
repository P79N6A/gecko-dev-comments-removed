





































#include "nsMetricsService.h"
#include "nsMetricsEventItem.h"
#include "nsIMetricsCollector.h"
#include "nsStringUtils.h"
#include "nsXPCOM.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIFile.h"
#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsNetCID.h"
#include "nsIObserverService.h"
#include "nsIUploadChannel.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIObserver.h"
#include "nsILocalFile.h"
#include "nsIPropertyBag.h"
#include "nsIProperty.h"
#include "nsIVariant.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMSerializer.h"
#include "nsIVariant.h"
#include "blapi.h"
#include "plbase64.h"
#include "nsISimpleEnumerator.h"
#include "nsIInputStreamChannel.h"
#include "nsIFileStreams.h"
#include "nsIBufferedStreams.h"
#include "nsIHttpChannel.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIIOService.h"
#include "nsMultiplexInputStream.h"
#include "prtime.h"
#include "prmem.h"
#include "prprf.h"
#include "prrng.h"
#include "bzlib.h"
#ifndef MOZILLA_1_8_BRANCH
#include "nsIClassInfoImpl.h"
#endif
#include "nsIDocShellTreeItem.h"
#include "nsDocShellCID.h"
#include "nsMemory.h"
#include "nsIBadCertListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIX509Cert.h"
#include "nsAutoPtr.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"


#define nsString_h___
#include "nsIStringStream.h"
#undef nsString_h___


#ifdef NS_METRICS_SEND_UNCOMPRESSED_DATA
#define NS_METRICS_MIME_TYPE "application/vnd.mozilla.metrics"
#else
#define NS_METRICS_MIME_TYPE "application/vnd.mozilla.metrics.bz2"
#endif


#define NS_EVENTLOG_FLUSH_POINT 64

#define NS_SECONDS_PER_DAY (60 * 60 * 24)

nsMetricsService* nsMetricsService::sMetricsService = nsnull;
#ifdef PR_LOGGING
PRLogModuleInfo *gMetricsLog;
#endif

static const char kQuitApplicationTopic[] = "quit-application";
static const char kUploadTimePref[] = "metrics.upload.next-time";
static const char kPingTimePref[] = "metrics.upload.next-ping";
static const char kEventCountPref[] = "metrics.event-count";
static const char kEnablePref[] = "metrics.upload.enable";

const PRUint32 nsMetricsService::kMaxRetries = 3;
const PRUint32 nsMetricsService::kMetricsVersion = 2;



#ifndef NS_METRICS_SEND_UNCOMPRESSED_DATA


static nsresult
CompressBZ2(nsIInputStream *src, PRFileDesc *outFd)
{
  

  char inbuf[4096], outbuf[4096];
  bz_stream strm;
  int ret = BZ_OK;

  memset(&strm, 0, sizeof(strm));

  if (BZ2_bzCompressInit(&strm, 9 , 0, 0) != BZ_OK)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = NS_OK;
  int action = BZ_RUN;
  for (;;) {
    PRUint32 bytesRead = 0;
    if (action == BZ_RUN && strm.avail_in == 0) {
      
      rv = src->Read(inbuf, sizeof(inbuf), &bytesRead);
      if (NS_FAILED(rv))
        break;
      strm.next_in = inbuf;
      strm.avail_in = (int) bytesRead;
    }

    strm.next_out = outbuf;
    strm.avail_out = sizeof(outbuf);

    ret = BZ2_bzCompress(&strm, action);
    if (action == BZ_RUN) {
      if (ret != BZ_RUN_OK) {
        MS_LOG(("BZ2_bzCompress/RUN failed: %d", ret));
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      if (bytesRead < sizeof(inbuf)) {
        
        action = BZ_FINISH;
      }
    } else if (ret != BZ_FINISH_OK && ret != BZ_STREAM_END) {
      MS_LOG(("BZ2_bzCompress/FINISH failed: %d", ret));
      rv = NS_ERROR_UNEXPECTED;
      break;
    }

    if (strm.avail_out < sizeof(outbuf)) {
      PRInt32 n = sizeof(outbuf) - strm.avail_out;
      if (PR_Write(outFd, outbuf, n) != n) {
        MS_LOG(("Failed to write compressed file"));
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
    }

    if (ret == BZ_STREAM_END)
      break;
  }

  BZ2_bzCompressEnd(&strm);
  return rv;
}

#endif 



class nsMetricsService::BadCertListener : public nsIBadCertListener,
                                          public nsIInterfaceRequestor
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBADCERTLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

  BadCertListener() { }

 private:
  ~BadCertListener() { }
};




NS_IMPL_THREADSAFE_ISUPPORTS2(nsMetricsService::BadCertListener,
                              nsIBadCertListener, nsIInterfaceRequestor)

NS_IMETHODIMP
nsMetricsService::BadCertListener::ConfirmUnknownIssuer(
    nsIInterfaceRequestor *socketInfo, nsIX509Cert *cert,
    PRInt16 *certAddType, PRBool *result)
{
  *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::BadCertListener::ConfirmMismatchDomain(
    nsIInterfaceRequestor *socketInfo, const nsACString &targetURL,
    nsIX509Cert *cert, PRBool *result)
{
  *result = PR_FALSE;

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);

  nsCString certHostOverride;
  prefs->GetCharPref("metrics.upload.cert-host-override",
                     getter_Copies(certHostOverride));

  if (!certHostOverride.IsEmpty()) {
    
    nsString certHost;
    cert->GetCommonName(certHost);
    if (certHostOverride.Equals(NS_ConvertUTF16toUTF8(certHost))) {
      *result = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::BadCertListener::ConfirmCertExpired(
    nsIInterfaceRequestor *socketInfo, nsIX509Cert *cert, PRBool *result)
{
  *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::BadCertListener::NotifyCrlNextupdate(
    nsIInterfaceRequestor *socketInfo,
    const nsACString &targetURL, nsIX509Cert *cert)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::BadCertListener::GetInterface(const nsIID &uuid,
                                                void **result)
{
  NS_ENSURE_ARG_POINTER(result);

  if (uuid.Equals(NS_GET_IID(nsIBadCertListener))) {
    *result = NS_STATIC_CAST(nsIBadCertListener *, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  *result = nsnull;
  return NS_ERROR_NO_INTERFACE;
}



nsMetricsService::nsMetricsService()  
    : mMD5Context(nsnull),
      mEventCount(0),
      mSuspendCount(0),
      mUploading(PR_FALSE),
      mNextWindowID(0),
      mRetryCount(0)
{
  NS_ASSERTION(!sMetricsService, ">1 MetricsService object created");
  sMetricsService = this;
}

 PLDHashOperator PR_CALLBACK
nsMetricsService::DetachCollector(const nsAString &key,
                                  nsIMetricsCollector *value, void *userData)
{
  value->OnDetach();
  return PL_DHASH_NEXT;
}

nsMetricsService::~nsMetricsService()
{
  NS_ASSERTION(sMetricsService == this, ">1 MetricsService object created");

  mCollectorMap.EnumerateRead(DetachCollector, nsnull);
  MD5_DestroyContext(mMD5Context, PR_TRUE);

  sMetricsService = nsnull;
}

NS_IMPL_ISUPPORTS6_CI(nsMetricsService, nsIMetricsService, nsIAboutModule,
                      nsIStreamListener, nsIRequestObserver, nsIObserver,
                      nsITimerCallback)

NS_IMETHODIMP
nsMetricsService::CreateEventItem(const nsAString &itemNamespace,
                                  const nsAString &itemName,
                                  nsIMetricsEventItem **result)
{
  *result = nsnull;

  nsMetricsEventItem *item = new nsMetricsEventItem(itemNamespace, itemName);
  NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*result = item);
  return NS_OK;
}

nsresult
nsMetricsService::BuildEventItem(nsIMetricsEventItem *item,
                                 nsIDOMElement **itemElement)
{
  *itemElement = nsnull;

  nsString itemNS, itemName;
  item->GetItemNamespace(itemNS);
  item->GetItemName(itemName);

  nsCOMPtr<nsIDOMElement> element;
  nsresult rv = mDocument->CreateElementNS(itemNS, itemName,
                                           getter_AddRefs(element));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPropertyBag> properties;
  item->GetProperties(getter_AddRefs(properties));
  if (properties) {
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = properties->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> propertySupports;
    while (NS_SUCCEEDED(
               enumerator->GetNext(getter_AddRefs(propertySupports)))) {
      nsCOMPtr<nsIProperty> property = do_QueryInterface(propertySupports);
      if (!property) {
        NS_WARNING("PropertyBag enumerator has non-nsIProperty elements");
        continue;
      }

      nsString name;
      rv = property->GetName(name);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to get property name");
        continue;
      }

      nsCOMPtr<nsIVariant> value;
      rv = property->GetValue(getter_AddRefs(value));
      if (NS_FAILED(rv) || !value) {
        NS_WARNING("Failed to get property value");
        continue;
      }

      
      
      PRUint16 dataType;
      value->GetDataType(&dataType);

      nsString valueString;
      if (dataType == nsIDataType::VTYPE_BOOL) {
        PRBool valueBool;
        rv = value->GetAsBool(&valueBool);
        if (NS_FAILED(rv)) {
          NS_WARNING("Variant has bool type but couldn't get bool value");
          continue;
        }
        valueString = valueBool ? NS_LITERAL_STRING("true")
                      : NS_LITERAL_STRING("false");
      } else {
        rv = value->GetAsDOMString(valueString);
        if (NS_FAILED(rv)) {
          NS_WARNING("Failed to convert property value to string");
          continue;
        }
      }

      rv = element->SetAttribute(name, valueString);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to set attribute value");
      }
      continue;
    }
  }

  
  PRInt32 childCount = 0;
  item->GetChildCount(&childCount);
  for (PRInt32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIMetricsEventItem> childItem;
    item->ChildAt(i, getter_AddRefs(childItem));
    NS_ASSERTION(childItem, "The child list cannot contain null items");

    nsCOMPtr<nsIDOMElement> childElement;
    rv = BuildEventItem(childItem, getter_AddRefs(childElement));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> nodeReturn;
    rv = element->AppendChild(childElement, getter_AddRefs(nodeReturn));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  element.swap(*itemElement);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::LogEvent(nsIMetricsEventItem *item)
{
  NS_ENSURE_ARG_POINTER(item);

  if (mSuspendCount != 0)  
    return NS_OK;

  
  if (mEventCount >= mConfig.EventLimit())
    return NS_OK;

  
  nsString eventNS, eventName;
  item->GetItemNamespace(eventNS);
  item->GetItemName(eventName);

  if (!mConfig.IsEventEnabled(eventNS, eventName))
    return NS_OK;

  
  nsCOMPtr<nsIDOMElement> eventElement;
  nsresult rv = BuildEventItem(item, getter_AddRefs(eventElement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsString timeString;
  AppendInt(timeString, PR_Now() / PR_USEC_PER_SEC);
  rv = eventElement->SetAttribute(NS_LITERAL_STRING("time"), timeString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = eventElement->SetAttribute(NS_LITERAL_STRING("session"), mSessionID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMNode> outChild;
  rv = mRoot->AppendChild(eventElement, getter_AddRefs(outChild));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if ((++mEventCount % NS_EVENTLOG_FLUSH_POINT) == 0)
    Flush();
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::LogSimpleEvent(const nsAString &eventNS,
                                 const nsAString &eventName,
                                 nsIPropertyBag *eventProperties)
{
  NS_ENSURE_ARG_POINTER(eventProperties);

  nsCOMPtr<nsIMetricsEventItem> item;
  nsresult rv = CreateEventItem(eventNS, eventName, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  item->SetProperties(eventProperties);
  return LogEvent(item);
}

NS_IMETHODIMP
nsMetricsService::Flush()
{
  nsresult rv;

  PRFileDesc *fd;
  rv = OpenDataFile(PR_WRONLY | PR_APPEND | PR_CREATE_FILE, &fd);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  nsCOMPtr<nsIDOMSerializer> ds =
    do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID);
  NS_ENSURE_TRUE(ds, NS_ERROR_UNEXPECTED);

  nsString docText;
  rv = ds->SerializeToString(mRoot, docText);
  NS_ENSURE_SUCCESS(rv, rv);

  
  docText.Cut(0, FindChar(docText, '>') + 1);

  
  PRInt32 start = RFindChar(docText, '<');
  docText.Cut(start, docText.Length() - start);

  NS_ConvertUTF16toUTF8 utf8Doc(docText);
  PRInt32 num = utf8Doc.Length();
  PRBool succeeded = ( PR_Write(fd, utf8Doc.get(), num) == num );

  PR_Close(fd);
  NS_ENSURE_STATE(succeeded);

  
  NS_ENSURE_STATE(PersistEventCount());

  
  rv = CreateRoot();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::Upload()
{
  if (mUploading) {
    
    MS_LOG(("Upload already in progress, aborting"));
    return NS_OK;
  }

  
  
  if (mEventCount == 0) {
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    NS_ENSURE_STATE(prefs);

    PRInt32 pingTime_sec;
    if (NS_SUCCEEDED(prefs->GetIntPref(kPingTimePref, &pingTime_sec))) {
      PRInt32 now_sec = PRInt32(PR_Now() / PR_USEC_PER_SEC);
      if (now_sec < pingTime_sec) {
        
        MS_LOG(("Suppressing upload while idle"));
        InitUploadTimer(PR_FALSE);
        return NS_OK;
      }
    }
  }

  

  nsresult rv = Flush();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UploadData();
  if (NS_SUCCEEDED(rv))
    mUploading = PR_TRUE;

  
  

  
  mEventCount = 0;
  NS_ENSURE_STATE(PersistEventCount());

  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::Suspend()
{
  mSuspendCount++;
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::Resume()
{
  if (mSuspendCount > 0)
    mSuspendCount--;
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::NewChannel(nsIURI *uri, nsIChannel **result)
{
  nsresult rv = Flush();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> dataFile;
  GetDataFile(&dataFile);
  NS_ENSURE_STATE(dataFile);

  nsCOMPtr<nsIInputStreamChannel> streamChannel =
      do_CreateInstance(NS_INPUTSTREAMCHANNEL_CONTRACTID);
  NS_ENSURE_STATE(streamChannel);

  rv = streamChannel->SetURI(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(streamChannel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool val;
  if (NS_SUCCEEDED(dataFile->Exists(&val)) && val) {
    nsCOMPtr<nsIInputStream> stream;
    OpenCompleteXMLStream(dataFile, getter_AddRefs(stream));
    NS_ENSURE_STATE(stream);

    rv  = streamChannel->SetContentStream(stream);
    rv |= channel->SetContentType(NS_LITERAL_CSTRING("text/xml"));
  } else {
    nsCOMPtr<nsIStringInputStream> errorStream =
        do_CreateInstance("@mozilla.org/io/string-input-stream;1");
    NS_ENSURE_STATE(errorStream);

    rv = errorStream->SetData("no metrics data", -1);
    NS_ENSURE_SUCCESS(rv, rv);

    rv  = streamChannel->SetContentStream(errorStream);
    rv |= channel->SetContentType(NS_LITERAL_CSTRING("text/plain"));
  }

  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  NS_ADDREF(*result = channel);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
    *result = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::OnStartRequest(nsIRequest *request, nsISupports *context)
{
  NS_ENSURE_STATE(!mConfigOutputStream);

  nsCOMPtr<nsIFile> file;
  GetConfigTempFile(getter_AddRefs(file));

  nsCOMPtr<nsIFileOutputStream> out =
      do_CreateInstance(NS_LOCALFILEOUTPUTSTREAM_CONTRACTID);
  NS_ENSURE_STATE(out);

  nsresult rv = out->Init(file, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, -1,
                          0);
  NS_ENSURE_SUCCESS(rv, rv);

  mConfigOutputStream = out;
  return NS_OK;
}

PRBool
nsMetricsService::LoadNewConfig(nsIFile *newConfig, nsIFile *oldConfig)
{
  
  PRBool exists = PR_FALSE;
  newConfig->Exists(&exists);
  if (exists && NS_SUCCEEDED(mConfig.Load(newConfig))) {
    MS_LOG(("Successfully loaded new config"));

    
    oldConfig->Remove(PR_FALSE);

    nsString filename;
    oldConfig->GetLeafName(filename);

    nsCOMPtr<nsIFile> directory;
    oldConfig->GetParent(getter_AddRefs(directory));

    newConfig->MoveTo(directory, filename);
    return PR_TRUE;
  }

  MS_LOG(("Couldn't load new config"));

  
  
  
  
  mConfig.ClearEvents();

  nsCOMPtr<nsILocalFile> lf = do_QueryInterface(oldConfig);
  nsresult rv = mConfig.Save(lf);
  if (NS_FAILED(rv)) {
    MS_LOG(("failed to save config: %d", rv));
  }

  return PR_FALSE;
}

void
nsMetricsService::RemoveDataFile()
{
  nsCOMPtr<nsILocalFile> dataFile;
  GetDataFile(&dataFile);
  if (!dataFile) {
    MS_LOG(("Couldn't get data file to remove"));
    return;
  }

  nsresult rv = dataFile->Remove(PR_FALSE);
  if (NS_SUCCEEDED(rv)) {
    MS_LOG(("Removed data file"));
  } else {
    MS_LOG(("Couldn't remove data file: %d", rv));
  }
}

PRInt32
nsMetricsService::GetRandomUploadInterval()
{
  static const int kSecondsPerHour = 60 * 60;
  mRetryCount = 0;

  PRInt32 interval_sec = kSecondsPerHour * 12;
  PRUint32 random = 0;
  if (nsMetricsUtils::GetRandomNoise(&random, sizeof(random))) {
    interval_sec += (random % (24 * kSecondsPerHour));
  }
  
  

  return interval_sec;
}

NS_IMETHODIMP
nsMetricsService::OnStopRequest(nsIRequest *request, nsISupports *context,
                                nsresult status)
{
  MS_LOG(("OnStopRequest status = %x", status));

  
  if (mConfigOutputStream) {
    mConfigOutputStream->Close();
    mConfigOutputStream = 0;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIFile> configTempFile;  
  GetConfigTempFile(getter_AddRefs(configTempFile));
  NS_ENSURE_STATE(configTempFile);

  nsCOMPtr<nsIFile> configFile;  
  GetConfigFile(getter_AddRefs(configFile));
  NS_ENSURE_STATE(configFile);

  PRBool success = PR_FALSE, replacedConfig = PR_FALSE;
  if (NS_SUCCEEDED(status)) {
    
    PRUint32 responseCode = 500;
    nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(request);
    if (channel) {
      channel->GetResponseStatus(&responseCode);
    }
    MS_LOG(("Server response: %u", responseCode));

    if (responseCode == 200) {
      success = PR_TRUE;
      RemoveDataFile();
    } else if (responseCode < 500) {
      
      RemoveDataFile();
    }

    replacedConfig = LoadNewConfig(configTempFile, configFile);
  } else {
    MS_LOG(("Request failed"));
  }

  
  if (!replacedConfig) {
    configTempFile->Remove(PR_FALSE);
  }

  
  if (success) {
    mRetryCount = 0;

    
    FlushClearPref(kUploadTimePref);
    MS_LOG(("Uploaded successfully and reset retry count"));

    
    
    
    PRInt32 interval_sec = GetRandomUploadInterval();
    MS_LOG(("Next ping no later than %d seconds from now", interval_sec));
    FlushIntPref(kPingTimePref, (PR_Now() / PR_USEC_PER_SEC) + interval_sec);
  } else if (++mRetryCount >= kMaxRetries) {
    RemoveDataFile();

    PRInt32 interval_sec = GetRandomUploadInterval();
    MS_LOG(("Reached max retry count, deferring upload for %d seconds",
            interval_sec));
    FlushIntPref(kUploadTimePref, (PR_Now() / PR_USEC_PER_SEC) + interval_sec);

    
    
  }

  
  InitUploadTimer(PR_FALSE);
  EnableCollectors();

  mUploading = PR_FALSE;
  return NS_OK;
}

struct DisabledCollectorsClosure
{
  DisabledCollectorsClosure(const nsTArray<nsString> &enabled)
      : enabledCollectors(enabled) { }

  
  const nsTArray<nsString> &enabledCollectors;

  
  nsTArray< nsCOMPtr<nsIMetricsCollector> > disabledCollectors;
};

 PLDHashOperator PR_CALLBACK
nsMetricsService::PruneDisabledCollectors(const nsAString &key,
                                          nsCOMPtr<nsIMetricsCollector> &value,
                                          void *userData)
{
  DisabledCollectorsClosure *dc =
    NS_STATIC_CAST(DisabledCollectorsClosure *, userData);

  
  
  for (PRUint32 i = 0; i < dc->enabledCollectors.Length(); ++i) {
    if (dc->enabledCollectors[i].Equals(key)) {
      
      return PL_DHASH_NEXT;
    }
  }

  
  
  MS_LOG(("Disabling collector %s", NS_ConvertUTF16toUTF8(key).get()));
  dc->disabledCollectors.AppendElement(value);
  return PL_DHASH_REMOVE;
}

 PLDHashOperator PR_CALLBACK
nsMetricsService::NotifyNewLog(const nsAString &key,
                               nsIMetricsCollector *value, void *userData)
{
  value->OnNewLog();
  return PL_DHASH_NEXT;
}

void
nsMetricsService::EnableCollectors()
{
  
  nsTArray<nsString> enabledCollectors;
  mConfig.GetEvents(enabledCollectors);

  
  
  
  
  

  DisabledCollectorsClosure dc(enabledCollectors);
  mCollectorMap.Enumerate(PruneDisabledCollectors, &dc);

  
  PRUint32 i;
  for (i = 0; i < dc.disabledCollectors.Length(); ++i) {
    dc.disabledCollectors[i]->OnDetach();
  }
  dc.disabledCollectors.Clear();

  
  for (i = 0; i < enabledCollectors.Length(); ++i) {
    const nsString &name = enabledCollectors[i];
    if (!mCollectorMap.GetWeak(name)) {
      nsCString contractID("@mozilla.org/metrics/collector;1?name=");
      contractID.Append(NS_ConvertUTF16toUTF8(name));

      nsCOMPtr<nsIMetricsCollector> coll = do_GetService(contractID.get());
      if (coll) {
        MS_LOG(("Created collector %s", contractID.get()));
        mCollectorMap.Put(name, coll);
        coll->OnAttach();
      } else {
        MS_LOG(("Couldn't instantiate collector %s", contractID.get()));
      }
    }
  }

  
  mCollectorMap.EnumerateRead(NotifyNewLog, nsnull);
}


static NS_METHOD
CopySegmentToStream(nsIInputStream *inStr,
                    void *closure,
                    const char *buffer,
                    PRUint32 offset,
                    PRUint32 count,
                    PRUint32 *countWritten)
{
  nsIOutputStream *outStr = NS_STATIC_CAST(nsIOutputStream *, closure);
  *countWritten = 0;
  while (count) {
    PRUint32 n;
    nsresult rv = outStr->Write(buffer, count, &n);
    if (NS_FAILED(rv))
      return rv;
    buffer += n;
    count -= n;
    *countWritten += n;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::OnDataAvailable(nsIRequest *request, nsISupports *context,
                                  nsIInputStream *stream, PRUint32 offset,
                                  PRUint32 count)
{
  PRUint32 n;
  return stream->ReadSegments(CopySegmentToStream, mConfigOutputStream,
                              count, &n);
}

NS_IMETHODIMP
nsMetricsService::Observe(nsISupports *subject, const char *topic,
                          const PRUnichar *data)
{
  if (strcmp(topic, kQuitApplicationTopic) == 0) {
    Flush();

    
    
    
  } else if (strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    mUploadTimer->Cancel();
  } else if (strcmp(topic, "profile-after-change") == 0) {
    nsresult rv = ProfileStartup();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (strcmp(topic, NS_WEBNAVIGATION_DESTROY) == 0 ||
             strcmp(topic, NS_CHROME_WEBNAVIGATION_DESTROY) == 0) {

    
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService("@mozilla.org/observer-service;1");
    NS_ENSURE_STATE(obsSvc);

    const char *newTopic;
    if (strcmp(topic, NS_WEBNAVIGATION_DESTROY) == 0) {
      newTopic = NS_METRICS_WEBNAVIGATION_DESTROY;
    } else {
      newTopic = NS_METRICS_CHROME_WEBNAVIGATION_DESTROY;
    }

    obsSvc->NotifyObservers(subject, newTopic, data);

    
    nsCOMPtr<nsIDOMWindow> window = do_GetInterface(subject);
    if (window) {
      MS_LOG(("Removing window from map: %p", window.get()));
      mWindowMap.Remove(window);
    } else {
      MS_LOG(("Couldn't get window to remove from map"));
    }
  } else if (strcmp(topic, NS_HTTP_ON_MODIFY_REQUEST_TOPIC) == 0) {
    
    nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(subject);
    if (props) {
      PRBool isMetrics = PR_FALSE;
      props->GetPropertyAsBool(
          NS_LITERAL_STRING("moz-metrics-request"), &isMetrics);
      if (isMetrics) {
        nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(subject);
        if (channel) {
          channel->SetRequestHeader(NS_LITERAL_CSTRING("Cookie"),
                                    EmptyCString(), PR_FALSE);
        }
      }
    }
  } else if (strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    
    if (NS_ConvertUTF8toUTF16(kEnablePref).Equals(nsDependentString(data))) {
      if (CollectionEnabled()) {
        StartCollection();
      } else {
        StopCollection();
      }
    }
  }
  
  return NS_OK;
}

nsresult
nsMetricsService::ProfileStartup()
{
  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);
  prefs->AddObserver(kEnablePref, this, PR_FALSE);

  return CollectionEnabled() ? StartCollection() : StopCollection();
}

nsresult
nsMetricsService::StartCollection()
{
  
  nsCOMPtr<nsIFile> file;
  GetConfigFile(getter_AddRefs(file));

  PRBool loaded = PR_FALSE;
  if (file) {
    PRBool exists;
    if (NS_SUCCEEDED(file->Exists(&exists)) && exists) {
      loaded = NS_SUCCEEDED(mConfig.Load(file));
    }
  }
  
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);
  prefs->GetIntPref("metrics.event-count", &mEventCount);

  
  static const char kSessionIDPref[] = "metrics.last-session-id";
  PRInt32 sessionID = -1;
  prefs->GetIntPref(kSessionIDPref, &sessionID);
  mSessionID.Cut(0, PR_UINT32_MAX);
  AppendInt(mSessionID, ++sessionID);
  nsresult rv = FlushIntPref(kSessionIDPref, sessionID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  EnableCollectors();

  
  InitUploadTimer(!loaded);

  return NS_OK;
}

nsresult
nsMetricsService::StopCollection()
{
  
  MS_LOG(("Clearing metrics state"));
  FlushClearPref(kUploadTimePref);
  FlushClearPref(kPingTimePref);
  FlushClearPref(kEventCountPref);

  nsCOMPtr<nsIFile> configFile;
  GetConfigFile(getter_AddRefs(configFile));
  if (configFile) {
    configFile->Remove(PR_FALSE);
  }

  nsCOMPtr<nsILocalFile> dataFile;
  GetDataFile(&dataFile);
  if (dataFile) {
    dataFile->Remove(PR_FALSE);
  }

  
  mConfig.Reset();
  EnableCollectors();
  CreateRoot();  

  return NS_OK;
}

NS_IMETHODIMP
nsMetricsService::Notify(nsITimer *timer)
{
  
  MS_LOG(("Timer fired, uploading metrics log"));

  
  FlushClearPref(kUploadTimePref);

  Upload();
  return NS_OK;
}

 nsMetricsService *
nsMetricsService::get()
{
  if (!sMetricsService) {
    nsCOMPtr<nsIMetricsService> ms =
      do_GetService(NS_METRICSSERVICE_CONTRACTID);
    if (!sMetricsService)
      NS_WARNING("failed to initialize metrics service");
  }
  return sMetricsService;
}

 NS_METHOD
nsMetricsService::Create(nsISupports *outer, const nsIID &iid, void **result)
{
  NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

  nsRefPtr<nsMetricsService> ms;
  if (!sMetricsService) {
    ms = new nsMetricsService();
    if (!ms)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ASSERTION(sMetricsService, "should be non-null");

    nsresult rv = ms->Init();
    if (NS_FAILED(rv))
      return rv;
  }
  return sMetricsService->QueryInterface(iid, result);
}

nsresult
nsMetricsService::Init()
{
  
  
  
  
#ifdef PR_LOGGING
  gMetricsLog = PR_NewLogModule("nsMetricsService");
#endif

  MS_LOG(("nsMetricsService::Init"));

  
  NS_ENSURE_TRUE(mWindowMap.Init(32), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mCollectorMap.Init(16), NS_ERROR_OUT_OF_MEMORY);

  mUploadTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ENSURE_TRUE(mUploadTimer, NS_ERROR_OUT_OF_MEMORY);

  mMD5Context = MD5_NewContext();
  NS_ENSURE_TRUE(mMD5Context, NS_ERROR_FAILURE);

  NS_ENSURE_STATE(mConfig.Init());

  
  mDocument = do_CreateInstance("@mozilla.org/xml/xml-document;1");
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  
  nsresult rv = CreateRoot();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = obsSvc->AddObserver(this, "profile-after-change", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = obsSvc->AddObserver(this, kQuitApplicationTopic, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = obsSvc->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = obsSvc->AddObserver(this, NS_WEBNAVIGATION_DESTROY, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this, NS_CHROME_WEBNAVIGATION_DESTROY, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = obsSvc->AddObserver(this, NS_HTTP_ON_MODIFY_REQUEST_TOPIC, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsMetricsService::CreateRoot()
{
  nsresult rv;
  nsCOMPtr<nsIDOMElement> root;
  rv = nsMetricsUtils::CreateElement(mDocument, NS_LITERAL_STRING("log"),
                                     getter_AddRefs(root));
  NS_ENSURE_SUCCESS(rv, rv);

  mRoot = root;
  return NS_OK;
}

nsresult
nsMetricsService::GetDataFile(nsCOMPtr<nsILocalFile> *result)
{
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->AppendNative(NS_LITERAL_CSTRING("metrics.xml"));
  NS_ENSURE_SUCCESS(rv, rv);

  *result = do_QueryInterface(file, &rv);
  return rv;
}

nsresult
nsMetricsService::OpenDataFile(PRUint32 flags, PRFileDesc **fd)
{
  nsCOMPtr<nsILocalFile> dataFile;
  nsresult rv = GetDataFile(&dataFile);
  NS_ENSURE_SUCCESS(rv, rv);

  return dataFile->OpenNSPRFileDesc(flags, 0600, fd);
}

nsresult
nsMetricsService::UploadData()
{
  
  
 
  if (!CollectionEnabled()) {
    MS_LOG(("Upload disabled"));
    return NS_ERROR_ABORT;
  }
 
  nsCString spec;
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->GetCharPref("metrics.upload.uri", getter_Copies(spec));
  }
  if (spec.IsEmpty()) {
    MS_LOG(("Upload URI not set"));
    return NS_ERROR_ABORT;
  }

  nsCOMPtr<nsILocalFile> file;
  nsresult rv = GetDataFileForUpload(&file);
  NS_ENSURE_SUCCESS(rv, rv);

  

  nsCOMPtr<nsIFileInputStream> fileStream =
      do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID);
  NS_ENSURE_STATE(fileStream);

  rv = fileStream->Init(file, -1, -1, nsIFileInputStream::DELETE_ON_CLOSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 streamLen;
  rv = fileStream->Available(&streamLen);
  NS_ENSURE_SUCCESS(rv, rv);

  if (streamLen == 0)
    return NS_ERROR_ABORT;

  nsCOMPtr<nsIBufferedInputStream> uploadStream =
      do_CreateInstance(NS_BUFFEREDINPUTSTREAM_CONTRACTID);
  NS_ENSURE_STATE(uploadStream);

  rv = uploadStream->Init(fileStream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID);
  NS_ENSURE_STATE(ios);

  nsCOMPtr<nsIChannel> channel;
  ios->NewChannel(spec, nsnull, nsnull, getter_AddRefs(channel));
  NS_ENSURE_STATE(channel); 

  
  nsCOMPtr<nsIWritablePropertyBag2> props = do_QueryInterface(channel);
  NS_ENSURE_STATE(props);
  props->SetPropertyAsBool(NS_LITERAL_STRING("moz-metrics-request"), PR_TRUE);

  nsCOMPtr<nsIInterfaceRequestor> certListener = new BadCertListener();
  NS_ENSURE_TRUE(certListener, NS_ERROR_OUT_OF_MEMORY);

  channel->SetNotificationCallbacks(certListener);

  nsCOMPtr<nsIUploadChannel> uploadChannel = do_QueryInterface(channel);
  NS_ENSURE_STATE(uploadChannel); 

  NS_NAMED_LITERAL_CSTRING(binaryType, NS_METRICS_MIME_TYPE);
  rv = uploadChannel->SetUploadStream(uploadStream, binaryType, -1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
  NS_ENSURE_STATE(httpChannel);
  rv = httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("POST"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = channel->AsyncOpen(this, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsMetricsService::GetDataFileForUpload(nsCOMPtr<nsILocalFile> *result)
{
  nsCOMPtr<nsILocalFile> input;
  nsresult rv = GetDataFile(&input);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> src;
  rv = OpenCompleteXMLStream(input, getter_AddRefs(src));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> temp;
  rv = input->Clone(getter_AddRefs(temp));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString leafName;
  rv = temp->GetNativeLeafName(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  leafName.Append(".bz2");
  rv = temp->SetNativeLeafName(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> ltemp = do_QueryInterface(temp, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRFileDesc *destFd = NULL;
  rv = ltemp->OpenNSPRFileDesc(PR_WRONLY | PR_TRUNCATE | PR_CREATE_FILE, 0600,
                               &destFd);

  

  if (NS_SUCCEEDED(rv)) {
#ifdef NS_METRICS_SEND_UNCOMPRESSED_DATA
    char buf[4096];
    PRUint32 n;

    while (NS_SUCCEEDED(rv = src->Read(buf, sizeof(buf), &n)) && n) {
      if (PR_Write(destFd, buf, n) != n) {
        NS_WARNING("failed to write data");
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
    }
#else
    rv = CompressBZ2(src, destFd);
#endif
  }

  if (destFd)
    PR_Close(destFd);

  if (NS_SUCCEEDED(rv)) {
    *result = nsnull;
    ltemp.swap(*result);
  }

  return rv;
}

nsresult
nsMetricsService::OpenCompleteXMLStream(nsILocalFile *dataFile,
                                       nsIInputStream **result)
{
  
  
  static const char kClientIDPref[] = "metrics.client-id";

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);
  
  nsCString clientID;
  nsresult rv = prefs->GetCharPref(kClientIDPref, getter_Copies(clientID));
  if (NS_FAILED(rv) || clientID.IsEmpty()) {
    rv = GenerateClientID(clientID);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = FlushCharPref(kClientIDPref, clientID.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  static const char METRICS_XML_HEAD[] =
      "<?xml version=\"1.0\"?>\n"
      "<log xmlns=\"" NS_METRICS_NAMESPACE "\" "
           "version=\"%d\" clientid=\"%s\">\n";
  static const char METRICS_XML_TAIL[] = "</log>";

  nsCOMPtr<nsIFileInputStream> fileStream =
      do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID);
  NS_ENSURE_STATE(fileStream);

  rv = fileStream->Init(dataFile, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMultiplexInputStream> miStream =
    do_CreateInstance(NS_MULTIPLEXINPUTSTREAM_CONTRACTID);
  NS_ENSURE_STATE(miStream);

  nsCOMPtr<nsIStringInputStream> stringStream =
      do_CreateInstance("@mozilla.org/io/string-input-stream;1");
  NS_ENSURE_STATE(stringStream);

  char *head = PR_smprintf(METRICS_XML_HEAD, kMetricsVersion, clientID.get());
  rv = stringStream->SetData(head, -1);
  PR_smprintf_free(head);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = miStream->AppendStream(stringStream);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = miStream->AppendStream(fileStream);
  NS_ENSURE_SUCCESS(rv, rv);

  stringStream = do_CreateInstance("@mozilla.org/io/string-input-stream;1");
  NS_ENSURE_STATE(stringStream);

  rv = stringStream->SetData(METRICS_XML_TAIL, sizeof(METRICS_XML_TAIL)-1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = miStream->AppendStream(stringStream);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*result = miStream);
  return NS_OK;
}

void
nsMetricsService::InitUploadTimer(PRBool immediate)
{
  
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefs) {
    NS_WARNING("couldn't get prefs service");
    return;
  }

  PRUint32 delay_sec;

  PRInt32 uploadTime_sec;
  if (NS_SUCCEEDED(prefs->GetIntPref(kUploadTimePref, &uploadTime_sec))) {
    
    
    PRInt32 now_sec = PRInt32(PR_Now() / PR_USEC_PER_SEC);
    if (now_sec >= uploadTime_sec) {
      delay_sec = 0;
    } else {
      delay_sec = (uploadTime_sec - now_sec);
    }
  } else if (immediate) {
    delay_sec = 0;
  } else {
    delay_sec = mConfig.UploadInterval();
  }

  nsresult rv = mUploadTimer->InitWithCallback(this,
                                               delay_sec * PR_MSEC_PER_SEC,
                                               nsITimer::TYPE_ONE_SHOT);
  if (NS_SUCCEEDED(rv)) {
    MS_LOG(("Initialized upload timer for %d sec", delay_sec));
  } else {
    MS_LOG(("Failed to initialize upload timer"));
  }
}

void
nsMetricsService::GetConfigFile(nsIFile **result)
{
  nsCOMPtr<nsIFile> file;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(file));
  if (file)
    file->AppendNative(NS_LITERAL_CSTRING("metrics-config.xml"));

  *result = nsnull;
  file.swap(*result);
}

void
nsMetricsService::GetConfigTempFile(nsIFile **result)
{
  nsCOMPtr<nsIFile> file;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(file));
  if (file)
    file->AppendNative(NS_LITERAL_CSTRING("metrics-config.tmp"));

  *result = nsnull;
  file.swap(*result);
}

nsresult
nsMetricsService::GenerateClientID(nsCString &clientID)
{
  

  struct {
    PRTime  a;
    PRUint8 b[32];
  } input;

  input.a = PR_Now();
  nsMetricsUtils::GetRandomNoise(input.b, sizeof(input.b));

  return HashBytes(
      NS_REINTERPRET_CAST(const PRUint8 *, &input), sizeof(input), clientID);
}

nsresult
nsMetricsService::HashBytes(const PRUint8 *bytes, PRUint32 length,
                            nsACString &result)
{
  unsigned char buf[HASH_LENGTH_MAX];
  unsigned int resultLength = 0;

  MD5_Begin(mMD5Context);
  MD5_Update(mMD5Context, bytes, length);
  MD5_End(mMD5Context, buf, &resultLength, sizeof(buf));

  
  
  char *resultBuffer;
  if (NS_CStringGetMutableData(
          result, ((resultLength + 2) / 3) * 4, &resultBuffer) == 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PL_Base64Encode(NS_REINTERPRET_CAST(char*, buf), resultLength, resultBuffer);

  
  result.SetLength(strlen(resultBuffer));
  return NS_OK;
}

PRBool
nsMetricsService::PersistEventCount()
{
  return NS_SUCCEEDED(FlushIntPref(kEventCountPref, mEventCount));
}

 PRUint32
nsMetricsService::GetWindowID(nsIDOMWindow *window)
{
  if (!sMetricsService) {
    NS_NOTREACHED("metrics service not created");
    return PR_UINT32_MAX;
  }

  return sMetricsService->GetWindowIDInternal(window);
}

NS_IMETHODIMP
nsMetricsService::GetWindowID(nsIDOMWindow *window, PRUint32 *id)
{
  *id = GetWindowIDInternal(window);
  return NS_OK;
}

PRUint32
nsMetricsService::GetWindowIDInternal(nsIDOMWindow *window)
{
  PRUint32 id;
  if (!mWindowMap.Get(window, &id)) {
    id = mNextWindowID++;
    MS_LOG(("Adding window %p to map with id %d", window, id));
    mWindowMap.Put(window, id);
  }

  return id;
}

nsresult
nsMetricsService::HashUTF8(const nsCString &str, nsCString &hashed)
{
  if (str.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  return HashBytes(
      NS_REINTERPRET_CAST(const PRUint8 *, str.get()), str.Length(), hashed);
}

 nsresult
nsMetricsService::FlushIntPref(const char *prefName, PRInt32 prefValue)
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);

  nsresult rv = prefs->SetIntPref(prefName, prefValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefService> prefService = do_QueryInterface(prefs);
  NS_ENSURE_STATE(prefService);

  rv = prefService->SavePrefFile(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

 nsresult
nsMetricsService::FlushCharPref(const char *prefName, const char *prefValue)
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);

  nsresult rv = prefs->SetCharPref(prefName, prefValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefService> prefService = do_QueryInterface(prefs);
  NS_ENSURE_STATE(prefService);

  rv = prefService->SavePrefFile(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

 nsresult
nsMetricsService::FlushClearPref(const char *prefName)
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_STATE(prefs);

  nsresult rv = prefs->ClearUserPref(prefName);
  if (NS_FAILED(rv)) {
    
    
    return NS_OK;
  }

  nsCOMPtr<nsIPrefService> prefService = do_QueryInterface(prefs);
  NS_ENSURE_STATE(prefService);

  rv = prefService->SavePrefFile(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

 PRBool
nsMetricsService::CollectionEnabled()
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(prefs, PR_FALSE);

  PRBool enabled = PR_FALSE;
  prefs->GetBoolPref(kEnablePref, &enabled);
  return enabled;
}

 nsresult
nsMetricsUtils::NewPropertyBag(nsIWritablePropertyBag2 **result)
{
  return CallCreateInstance("@mozilla.org/hash-property-bag;1", result);
}

 nsresult
nsMetricsUtils::AddChildItem(nsIMetricsEventItem *parent,
                             const nsAString &childName,
                             nsIPropertyBag *childProperties)
{
  nsCOMPtr<nsIMetricsEventItem> item;
  nsMetricsService::get()->CreateEventItem(childName, getter_AddRefs(item));
  NS_ENSURE_STATE(item);

  item->SetProperties(childProperties);

  nsresult rv = parent->AppendChild(item);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

 PRBool
nsMetricsUtils::GetRandomNoise(void *buf, PRSize size)
{
  PRSize nbytes = 0;
  while (nbytes < size) {
    PRSize n = PR_GetRandomNoise(
        NS_STATIC_CAST(char *, buf) + nbytes, size - nbytes);
    if (n == 0) {
      MS_LOG(("Couldn't get any random bytes"));
      return PR_FALSE;
    }
    nbytes += n;
  }
  return PR_TRUE;
}

 nsresult
nsMetricsUtils::CreateElement(nsIDOMDocument *ownerDoc,
                              const nsAString &tag, nsIDOMElement **element)
{
  return ownerDoc->CreateElementNS(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                                   tag, element);
}


 PRBool
nsMetricsUtils::IsSubframe(nsIDocShellTreeItem* docShell)
{
  
  
  if (!docShell) {
    return PR_FALSE;
  }

  PRInt32 itemType;
  docShell->GetItemType(&itemType);
  if (itemType != nsIDocShellTreeItem::typeContent) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIDocShellTreeItem> parent;
  docShell->GetSameTypeParent(getter_AddRefs(parent));
  return (parent != nsnull);
}


 PRUint32
nsMetricsUtils::FindWindowForNode(nsIDOMNode *node)
{
  nsCOMPtr<nsIDOMDocument> ownerDoc;
  node->GetOwnerDocument(getter_AddRefs(ownerDoc));
  NS_ENSURE_STATE(ownerDoc);

  nsCOMPtr<nsIDOMDocumentView> docView = do_QueryInterface(ownerDoc);
  NS_ENSURE_STATE(docView);

  nsCOMPtr<nsIDOMAbstractView> absView;
  docView->GetDefaultView(getter_AddRefs(absView));
  NS_ENSURE_STATE(absView);

  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(absView);
  NS_ENSURE_STATE(window);

  return nsMetricsService::GetWindowID(window);
}
