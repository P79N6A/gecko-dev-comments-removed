
















































 
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
#include "nsAutoPtr.h"

NS_IMPL_ISUPPORTS1(nsTransferable, nsITransferable)

PRUint32 GetDataForFlavor (const nsTArray<DataStruct>& aArray,
                           const char* aDataFlavor)
{
  for (PRUint32 i = 0 ; i < aArray.Length () ; ++i) {
    if (aArray[i].GetFlavor().Equals (aDataFlavor))
      return i;
  }

  return aArray.NoIndex;
}


DataStruct::~DataStruct() 
{ 
  if (mCacheFileName) nsCRT::free(mCacheFileName); 

    
    
}


void
DataStruct::SetData ( nsISupports* aData, PRUint32 aDataLen )
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
DataStruct::GetData ( nsISupports** aData, PRUint32 *aDataLen )
{
  
  if ( !mData && mCacheFileName ) {
    
    
    if ( NS_SUCCEEDED(ReadCache(aData, aDataLen)) )
      return;
    else {
      
      NS_WARNING("Oh no, couldn't read data in from the cache file");
      *aData = nsnull;
      *aDataLen = 0;
      return;
    }
  }
  
  *aData = mData;
  if ( mData )
    NS_ADDREF(*aData); 
  *aDataLen = mDataLen;
}



nsIFile*
DataStruct::GetFileSpec(const char * aFileName)
{
  nsIFile* cacheFile;
  NS_GetSpecialDirectory(NS_OS_TEMP_DIR, &cacheFile);
  
  if (cacheFile == nsnull)
    return nsnull;

  
  
  
  if (!aFileName) {
    cacheFile->AppendNative(NS_LITERAL_CSTRING("clipboardcache"));
    cacheFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  } else {
    cacheFile->AppendNative(nsDependentCString(aFileName));
  }
  
  return cacheFile;
}



nsresult
DataStruct::WriteCache(nsISupports* aData, PRUint32 aDataLen)
{
  
  nsCOMPtr<nsIFile> cacheFile ( getter_AddRefs(GetFileSpec(mCacheFileName)) );
  if (cacheFile) {
    
    if (!mCacheFileName) {
      nsXPIDLCString fName;
      cacheFile->GetNativeLeafName(fName);
      mCacheFileName = nsCRT::strdup(fName);
    }

    
    
    
    nsCOMPtr<nsIOutputStream> outStr;

    NS_NewLocalFileOutputStream(getter_AddRefs(outStr),
                                cacheFile);

    if (!outStr) return NS_ERROR_FAILURE;

    void* buff = nsnull;
    nsPrimitiveHelpers::CreateDataFromPrimitive ( mFlavor.get(), aData, &buff, aDataLen );
    if ( buff ) {
      PRUint32 ignored;
      outStr->Write(reinterpret_cast<char*>(buff), aDataLen, &ignored);
      nsMemory::Free(buff);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}



nsresult
DataStruct::ReadCache(nsISupports** aData, PRUint32* aDataLen)
{
  
  if (!mCacheFileName)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIFile> cacheFile ( getter_AddRefs(GetFileSpec(mCacheFileName)) );
  PRBool exists;
  if ( cacheFile && NS_SUCCEEDED(cacheFile->Exists(&exists)) && exists ) {
    
    PRInt64 fileSize;
    PRInt64 max32(LL_INIT(0, 0xFFFFFFFF));
    cacheFile->GetFileSize(&fileSize);
    if (LL_CMP(fileSize, >, max32))
      return NS_ERROR_OUT_OF_MEMORY;
    PRUint32 size;
    LL_L2UI(size, fileSize);

    
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

    
    *aData    = nsnull;
    *aDataLen = 0;
  }

  return NS_ERROR_FAILURE;
}







nsTransferable::nsTransferable()
{
}






nsTransferable::~nsTransferable()
{
}









nsresult
nsTransferable::GetTransferDataFlavors(nsISupportsArray ** aDataFlavorList)
{
  nsresult rv = NS_NewISupportsArray ( aDataFlavorList );
  if (NS_FAILED(rv)) return rv;

  for ( PRUint32 i=0; i<mDataArray.Length(); ++i ) {
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
nsTransferable::GetTransferData(const char *aFlavor, nsISupports **aData, PRUint32 *aDataLen)
{
  NS_ENSURE_ARG_POINTER(aFlavor && aData && aDataLen);

  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> savedData;
  
  
  PRUint32 i;
  for (i = 0; i < mDataArray.Length(); ++i ) {
    DataStruct& data = mDataArray.ElementAt(i);
    if ( data.GetFlavor().Equals(aFlavor) ) {
      nsCOMPtr<nsISupports> dataBytes;
      PRUint32 len;
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

  PRBool found = PR_FALSE;

  
  if ( mFormatConv ) {
    for (i = 0; i < mDataArray.Length(); ++i) {
      DataStruct& data = mDataArray.ElementAt(i);
      PRBool canConvert = PR_FALSE;
      mFormatConv->CanConvert(data.GetFlavor().get(), aFlavor, &canConvert);
      if ( canConvert ) {
        nsCOMPtr<nsISupports> dataBytes;
        PRUint32 len;
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
        found = PR_TRUE;
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
nsTransferable::GetAnyTransferData(char **aFlavor, nsISupports **aData, PRUint32 *aDataLen)
{
  NS_ENSURE_ARG_POINTER(aFlavor && aData && aDataLen);

  for ( PRUint32 i=0; i < mDataArray.Length(); ++i ) {
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
nsTransferable::SetTransferData(const char *aFlavor, nsISupports *aData, PRUint32 aDataLen)
{
  NS_ENSURE_ARG(aFlavor);

  
  PRUint32 i = 0;
  for (i = 0; i < mDataArray.Length(); ++i) {
    DataStruct& data = mDataArray.ElementAt(i);
    if ( data.GetFlavor().Equals(aFlavor) ) {
      data.SetData ( aData, aDataLen );
      return NS_OK;
    }
  }

  
  if ( mFormatConv ) {
    for (i = 0; i < mDataArray.Length(); ++i) {
      DataStruct& data = mDataArray.ElementAt(i);
      PRBool canConvert = PR_FALSE;
      mFormatConv->CanConvert(aFlavor, data.GetFlavor().get(), &canConvert);

      if ( canConvert ) {
        nsCOMPtr<nsISupports> ConvertedData;
        PRUint32 ConvertedLen;
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
  if (GetDataForFlavor (mDataArray, aDataFlavor) != mDataArray.NoIndex)
    return NS_ERROR_FAILURE;

  
  mDataArray.AppendElement(DataStruct ( aDataFlavor ));

  return NS_OK;
}








NS_IMETHODIMP
nsTransferable::RemoveDataFlavor(const char *aDataFlavor)
{
  PRUint32 idx;
  if ((idx = GetDataForFlavor(mDataArray, aDataFlavor)) != mDataArray.NoIndex) {
    mDataArray.RemoveElementAt (idx);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}






NS_IMETHODIMP
nsTransferable::IsLargeDataSet(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}






NS_IMETHODIMP nsTransferable::SetConverter(nsIFormatConverter * aConverter)
{
  mFormatConv = aConverter;
  return NS_OK;
}






NS_IMETHODIMP nsTransferable::GetConverter(nsIFormatConverter * *aConverter)
{
  NS_ENSURE_ARG_POINTER(aConverter);
  *aConverter = mFormatConv;
  NS_IF_ADDREF(*aConverter);
  return NS_OK;
}








NS_IMETHODIMP
nsTransferable::FlavorsTransferableCanImport(nsISupportsArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  
  
  
  GetTransferDataFlavors(_retval);                        
  nsCOMPtr<nsIFormatConverter> converter;
  GetConverter(getter_AddRefs(converter));
  if ( converter ) {
    nsCOMPtr<nsISupportsArray> convertedList;
    converter->GetInputDataFlavors(getter_AddRefs(convertedList));

    if ( convertedList ) {
      PRUint32 importListLen;
      convertedList->Count(&importListLen);

      for ( PRUint32 i=0; i < importListLen; ++i ) {
        nsCOMPtr<nsISupports> genericFlavor;
        convertedList->GetElementAt ( i, getter_AddRefs(genericFlavor) );

        nsCOMPtr<nsISupportsCString> flavorWrapper ( do_QueryInterface (genericFlavor) );
        nsCAutoString flavorStr;
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
  NS_ENSURE_ARG_POINTER(_retval);
  
  
  
  
  GetTransferDataFlavors(_retval);  
  nsCOMPtr<nsIFormatConverter> converter;
  GetConverter(getter_AddRefs(converter));
  if ( converter ) {
    nsCOMPtr<nsISupportsArray> convertedList;
    converter->GetOutputDataFlavors(getter_AddRefs(convertedList));

    if ( convertedList ) {
      PRUint32 importListLen;
      convertedList->Count(&importListLen);

      for ( PRUint32 i=0; i < importListLen; ++i ) {
        nsCOMPtr<nsISupports> genericFlavor;
        convertedList->GetElementAt ( i, getter_AddRefs(genericFlavor) );

        nsCOMPtr<nsISupportsCString> flavorWrapper ( do_QueryInterface (genericFlavor) );
        nsCAutoString flavorStr;
        flavorWrapper->GetData( flavorStr );

        if (GetDataForFlavor (mDataArray, flavorStr.get())
            == mDataArray.NoIndex) 
          (*_retval)->AppendElement (genericFlavor);
      } 
    }
  } 

  return NS_OK;
} 
