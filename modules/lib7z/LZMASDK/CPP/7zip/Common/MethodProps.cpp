

#include "StdAfx.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "MethodProps.h"

static const UInt64 k_LZMA = 0x030101;
static const UInt64 k_LZMA2 = 0x21;

HRESULT SetMethodProperties(const CMethod &method, const UInt64 *inSizeForReduce, IUnknown *coder)
{
  bool tryReduce = false;
  UInt32 reducedDictionarySize = 1 << 10;
  if (inSizeForReduce != 0 && (method.Id == k_LZMA || method.Id == k_LZMA2))
  {
    for (;;)
    {
      const UInt32 step = (reducedDictionarySize >> 1);
      if (reducedDictionarySize >= *inSizeForReduce)
      {
        tryReduce = true;
        break;
      }
      reducedDictionarySize += step;
      if (reducedDictionarySize >= *inSizeForReduce)
      {
        tryReduce = true;
        break;
      }
      if (reducedDictionarySize >= ((UInt32)3 << 30))
        break;
      reducedDictionarySize += step;
    }
  }

  {
    int numProps = method.Props.Size();
    CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
    coder->QueryInterface(IID_ICompressSetCoderProperties, (void **)&setCoderProperties);
    if (setCoderProperties == NULL)
    {
      if (numProps != 0)
        return E_INVALIDARG;
    }
    else
    {
      CRecordVector<PROPID> propIDs;
      NWindows::NCOM::CPropVariant *values = new NWindows::NCOM::CPropVariant[numProps];
      HRESULT res = S_OK;
      try
      {
        for (int i = 0; i < numProps; i++)
        {
          const CProp &prop = method.Props[i];
          propIDs.Add(prop.Id);
          NWindows::NCOM::CPropVariant &value = values[i];
          value = prop.Value;
          
          if (tryReduce)
            if (prop.Id == NCoderPropID::kDictionarySize)
              if (value.vt == VT_UI4)
                if (reducedDictionarySize < value.ulVal)
            value.ulVal = reducedDictionarySize;
        }
        CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
        coder->QueryInterface(IID_ICompressSetCoderProperties, (void **)&setCoderProperties);
        res = setCoderProperties->SetCoderProperties(&propIDs.Front(), values, numProps);
      }
      catch(...)
      {
        delete []values;
        throw;
      }
      delete []values;
      RINOK(res);
    }
  }
 
  













  return S_OK;
}

