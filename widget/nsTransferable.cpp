













 
#include "nsTransferable.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsTArray.h"
#include "nsIFormatConverter.h"
#include "nsIComponentManager.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsMemory.h"
#include "nsPrimitiveHelpers.h"
#include "nsXPIDLString.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryService.h"
#include "nsCRT.h" 
#include "nsNetUtil.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsIFile.h"
#include "nsILoadContext.h"
#include "nsAutoPtr.h"

NS_IMPL_ISUPPORTS(nsTransferable, nsITransferable)

size_t GetDataForFlavor (const nsTArray<DataStruct>& aArray,
                           const char* aDataFlavor)
{
  for (size_t i = 0 ; i < aArray.Length () ; ++i) {
    if (aArray[i].GetFlavor().Equals (aDataFlavor))
      return i;
  }

  return aArray.NoIndex;
}


DataStruct::~DataStruct() 
{ 
  if (mCacheFileName) free(mCacheFileName); 
}


void
DataStruct::SetData ( nsISupports* aData, uint32_t aDataLen )
{
  
  if (aDataLen > kLargeDatasetSize) {
    
    if ( NS_SUCCEEDED(WriteCache(aData, aDataLen)) )
      return;
    else
			NS_WARNING("Oh no, couldn't write data to the cache file");   
  } 

  mData    = aData;
  mDataLen = aDataLen;  
}



void
DataStruct::GetData ( nsISupports** aData, uint32_t *aDataLen )
{
  
  if ( !mData && mCacheFileName ) {
    
    
    if ( NS_SUCCEEDED(ReadCache(aData, aDataLen)) )
      return;
    else {
      
      NS_WARNING("Oh no, couldn't read data in from the cache file");
      *aData = nullptr;
      *aDataLen = 0;
      return;
    }
  }
  
  *aData = mData;
  if ( mData )
    NS_ADDREF(*aData); 
  *aDataLen = mDataLen;
}



already_AddRefed<nsIFile>
DataStruct::GetFileSpec(const char* aFileName)
{
  nsCOMPtr<nsIFile> cacheFile;
  NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(cacheFile));
  
  if (!cacheFile)
    return nullptr;

  
  
  
  if (!aFileName) {
    cacheFile->AppendNative(NS_LITERAL_CSTRING("clipboardcache"));
    cacheFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  } else {
    cacheFile->AppendNative(nsDependentCString(aFileName));
  }
  
  return cacheFile.forget();
}



nsresult
DataStruct::WriteCache(nsISupports* aData, uint32_t aDataLen)
{
  
  nsCOMPtr<nsIFile> cacheFile = GetFileSpec(mCacheFileName);
  if (cacheFile) {
    
    if (!mCacheFileName) {
      nsXPIDLCString fName;
      cacheFile->GetNativeLeafName(fName);
      mCacheFileName = strdup(fName);
    }

    
    
    
    nsCOMPtr<nsIOutputStream> outStr;

    NS_NewLocalFileOutputStream(getter_AddRefs(outStr),
                                cacheFile);

    if (!outStr) return NS_ERROR_FAILURE;

    void* buff = nullptr;
    nsPrimitiveHelpers::CreateDataFromPrimitive ( mFlavor.get(), aData, &buff, aDataLen );
    if ( buff ) {
      uint32_t ignored;
      outStr->Write(reinterpret_cast<char*>(buff), aDataLen, &ignored);
      nsMemory::Free(buff);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}



nsresult
DataStruct::ReadCache(nsISupports** aData, uint32_t* aDataLen)
{
  
  if (!mCacheFileName)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIFile> cacheFile = GetFileSpec(mCacheFileName);
  bool exists;
  if ( cacheFile && NS_SUCCEEDED(cacheFile->Exists(&exists)) && exists ) {
    
    int64_t fileSize;
    int64_t max32 = 0xFFFFFFFF;
    cacheFile->GetFileSize(&fileSize);
    if (fileSize > max32)
      return NS_ERROR_OUT_OF_MEMORY;

    uint32_t size = uint32_t(fileSize);
    
    nsAutoArrayPtr<char> data(new char[size]);
    if ( !data )
      return NS_ERROR_OUT_OF_MEMORY;
      
    
    nsCOMPtr<nsIInputStream> inStr;
    NS_NewLocalFileInputStream( getter_AddRefs(inStr),
                                cacheFile);
    
    if (!cacheFile) return NS_ERROR_FAILURE;

    nsresult rv = inStr->Read(data, fileSize, aDataLen);

    
    if (NS_SUCCEEDED(rv) && *aDataLen == size) {
      nsPrimitiveHelpers::CreatePrimitiveForData ( mFlavor.get(), data, fileSize, aData );
      return *aData ? NS_OK : NS_ERROR_FAILURE;
    }

    
    *aData    = nullptr;
    *aDataLen = 0;
  }

  return NS_ERROR_FAILURE;
}







nsTransferable::nsTransferable()
  : mPrivateData(false)
#ifdef DEBUG
  , mInitialized(false)
#endif
{
}






nsTransferable::~nsTransferable()
{
}


NS_IMETHODIMP
nsTransferable::Init(nsILoadContext* aContext)
{
  MOZ_ASSERT(!mInitialized);

  if (aContext) {
    mPrivateData = aContext->UsePrivateBrowsing();
  }
#ifdef DEBUG
  mInitialized = true;
#endif
  return NS_OK;
}








nsresult
nsTransferable::GetTransferDataFlavors(nsISupportsArray ** aDataFlavorList)
{
  MOZ_ASSERT(mInitialized);

  nsresult rv = NS_NewISupportsArray ( aDataFlavorList );
  if (NS_FAILED(rv)) return rv;

  for (size_t i = 0; i < mDataArray.Length(); ++i) {
    DataStruct& data = mDataArray.ElementAt(i);
    nsCOMPtr<nsISupportsCString> flavorWrapper = do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID);
    if ( flavorWrapper ) {
      flavorWrapper->SetData ( data.GetFlavor() );
      nsCOMPtr<nsISupports> genericWrapper ( do_QueryInterface(flavorWrapper) );
      (*aDataFlavorList)->AppendElement( genericWrapper );
    }
  }

  return NS_OK;
}









NS_IMETHODIMP
nsTransferable::GetTransferData(const char *aFlavor, nsISupports **aData, uint32_t *aDataLen)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(aFlavor && aData && aDataLen);

  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> savedData;
  
  
  for (size_t i = 0; i < mDataArray.Length(); ++i) {
    DataStruct& data = mDataArray.ElementAt(i);
    if ( data.GetFlavor().Equals(aFlavor) ) {
      nsCOMPtr<nsISupports> dataBytes;
      uint32_t len;
      data.GetData(getter_AddRefs(dataBytes), &len);
      if (len == kFlavorHasDataProvider && dataBytes) {
        
        nsCOMPtr<nsIFlavorDataProvider> dataProvider = do_QueryInterface(dataBytes);
        if (dataProvider) {
          rv = dataProvider->GetFlavorData(this, aFlavor,
                                           getter_AddRefs(dataBytes), &len);
          if (NS_FAILED(rv))
            break;    
        }
      }
      if (dataBytes && len > 0) { 
        *aDataLen = len;
        dataBytes.forget(aData);
        return NS_OK;
      }
      savedData = dataBytes;  
      break;
    }
  }

  bool found = false;

  
  if ( mFormatConv ) {
    for (size_t i = 0; i < mDataArray.Length(); ++i) {
      DataStruct& data = mDataArray.ElementAt(i);
      bool canConvert = false;
      mFormatConv->CanConvert(data.GetFlavor().get(), aFlavor, &canConvert);
      if ( canConvert ) {
        nsCOMPtr<nsISupports> dataBytes;
        uint32_t len;
        data.GetData(getter_AddRefs(dataBytes), &len);
        if (len == kFlavorHasDataProvider && dataBytes) {
          
          nsCOMPtr<nsIFlavorDataProvider> dataProvider = do_QueryInterface(dataBytes);
          if (dataProvider) {
            rv = dataProvider->GetFlavorData(this, aFlavor,
                                             getter_AddRefs(dataBytes), &len);
            if (NS_FAILED(rv))
              break;  
          }
        }
        mFormatConv->Convert(data.GetFlavor().get(), dataBytes, len, aFlavor, aData, aDataLen);
        found = true;
        break;
      }
    }
  }

  
  if (!found) {
    savedData.forget(aData);
    *aDataLen = 0;
  }

  return found ? NS_OK : NS_ERROR_FAILURE;
}








NS_IMETHODIMP
nsTransferable::GetAnyTransferData(char **aFlavor, nsISupports **aData, uint32_t *aDataLen)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(aFlavor && aData && aDataLen);

  for (size_t i = 0; i < mDataArray.Length(); ++i) {
    DataStruct& data = mDataArray.ElementAt(i);
    if (data.IsDataAvailable()) {
      *aFlavor = ToNewCString(data.GetFlavor());
      data.GetData(aData, aDataLen);
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}







NS_IMETHODIMP
nsTransferable::SetTransferData(const char *aFlavor, nsISupports *aData, uint32_t aDataLen)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG(aFlavor);

  
  for (size_t i = 0; i < mDataArray.Length(); ++i) {
    DataStruct& data = mDataArray.ElementAt(i);
    if ( data.GetFlavor().Equals(aFlavor) ) {
      data.SetData ( aData, aDataLen );
      return NS_OK;
    }
  }

  
  if ( mFormatConv ) {
    for (size_t i = 0; i < mDataArray.Length(); ++i) {
      DataStruct& data = mDataArray.ElementAt(i);
      bool canConvert = false;
      mFormatConv->CanConvert(aFlavor, data.GetFlavor().get(), &canConvert);

      if ( canConvert ) {
        nsCOMPtr<nsISupports> ConvertedData;
        uint32_t ConvertedLen;
        mFormatConv->Convert(aFlavor, aData, aDataLen, data.GetFlavor().get(), getter_AddRefs(ConvertedData), &ConvertedLen);
        data.SetData(ConvertedData, ConvertedLen);
        return NS_OK;
      }
    }
  }

  
  nsresult result = NS_ERROR_FAILURE;
  if ( NS_SUCCEEDED(AddDataFlavor(aFlavor)) )
    result = SetTransferData (aFlavor, aData, aDataLen);
    
  return result;
}







NS_IMETHODIMP
nsTransferable::AddDataFlavor(const char *aDataFlavor)
{
  MOZ_ASSERT(mInitialized);

  if (GetDataForFlavor (mDataArray, aDataFlavor) != mDataArray.NoIndex)
    return NS_ERROR_FAILURE;

  
  mDataArray.AppendElement(DataStruct ( aDataFlavor ));

  return NS_OK;
}








NS_IMETHODIMP
nsTransferable::RemoveDataFlavor(const char *aDataFlavor)
{
  MOZ_ASSERT(mInitialized);

  size_t idx = GetDataForFlavor(mDataArray, aDataFlavor);
  if (idx != mDataArray.NoIndex) {
    mDataArray.RemoveElementAt (idx);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}






NS_IMETHODIMP
nsTransferable::IsLargeDataSet(bool *_retval)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = false;
  return NS_OK;
}






NS_IMETHODIMP nsTransferable::SetConverter(nsIFormatConverter * aConverter)
{
  MOZ_ASSERT(mInitialized);

  mFormatConv = aConverter;
  return NS_OK;
}






NS_IMETHODIMP nsTransferable::GetConverter(nsIFormatConverter * *aConverter)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(aConverter);
  *aConverter = mFormatConv;
  NS_IF_ADDREF(*aConverter);
  return NS_OK;
}








NS_IMETHODIMP
nsTransferable::FlavorsTransferableCanImport(nsISupportsArray **_retval)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(_retval);
  
  
  
  
  GetTransferDataFlavors(_retval);                        
  nsCOMPtr<nsIFormatConverter> converter;
  GetConverter(getter_AddRefs(converter));
  if ( converter ) {
    nsCOMPtr<nsISupportsArray> convertedList;
    converter->GetInputDataFlavors(getter_AddRefs(convertedList));

    if ( convertedList ) {
      uint32_t importListLen;
      convertedList->Count(&importListLen);

      for (uint32_t i = 0; i < importListLen; ++i ) {
        nsCOMPtr<nsISupports> genericFlavor;
        convertedList->GetElementAt ( i, getter_AddRefs(genericFlavor) );

        nsCOMPtr<nsISupportsCString> flavorWrapper ( do_QueryInterface (genericFlavor) );
        nsAutoCString flavorStr;
        flavorWrapper->GetData( flavorStr );

        if (GetDataForFlavor (mDataArray, flavorStr.get())
            == mDataArray.NoIndex) 
          (*_retval)->AppendElement (genericFlavor);
      } 
    }
  } 

  return NS_OK;  
} 








NS_IMETHODIMP
nsTransferable::FlavorsTransferableCanExport(nsISupportsArray **_retval)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(_retval);
  
  
  
  
  GetTransferDataFlavors(_retval);  
  nsCOMPtr<nsIFormatConverter> converter;
  GetConverter(getter_AddRefs(converter));
  if ( converter ) {
    nsCOMPtr<nsISupportsArray> convertedList;
    converter->GetOutputDataFlavors(getter_AddRefs(convertedList));

    if ( convertedList ) {
      uint32_t importListLen;
      convertedList->Count(&importListLen);

      for ( uint32_t i=0; i < importListLen; ++i ) {
        nsCOMPtr<nsISupports> genericFlavor;
        convertedList->GetElementAt ( i, getter_AddRefs(genericFlavor) );

        nsCOMPtr<nsISupportsCString> flavorWrapper ( do_QueryInterface (genericFlavor) );
        nsAutoCString flavorStr;
        flavorWrapper->GetData( flavorStr );

        if (GetDataForFlavor (mDataArray, flavorStr.get())
            == mDataArray.NoIndex) 
          (*_retval)->AppendElement (genericFlavor);
      } 
    }
  } 

  return NS_OK;
} 

NS_IMETHODIMP
nsTransferable::GetIsPrivateData(bool *aIsPrivateData)
{
  MOZ_ASSERT(mInitialized);

  NS_ENSURE_ARG_POINTER(aIsPrivateData);

  *aIsPrivateData = mPrivateData;

  return NS_OK;
}

NS_IMETHODIMP
nsTransferable::SetIsPrivateData(bool aIsPrivateData)
{
  MOZ_ASSERT(mInitialized);

  mPrivateData = aIsPrivateData;

  return NS_OK;
}

NS_IMETHODIMP
nsTransferable::GetRequestingNode(nsIDOMNode** outRequestingNode)
{
  nsCOMPtr<nsIDOMNode> node = do_QueryReferent(mRequestingNode);
  node.forget(outRequestingNode);
  return NS_OK;
}

NS_IMETHODIMP
nsTransferable::SetRequestingNode(nsIDOMNode* aRequestingNode)
{
  mRequestingNode = do_GetWeakReference(aRequestingNode);
  return NS_OK;
}
