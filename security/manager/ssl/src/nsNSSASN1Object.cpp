



































#include "nsNSSASN1Object.h"
#include "nsIComponentManager.h"
#include "secasn1.h"
#include "nsReadableUtils.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsXPCOMCID.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(nsNSSASN1Sequence, nsIASN1Sequence, 
                                                 nsIASN1Object)
NS_IMPL_THREADSAFE_ISUPPORTS2(nsNSSASN1PrintableItem, nsIASN1PrintableItem,
                                                      nsIASN1Object)







static int
getInteger256(unsigned char *data, unsigned int nb)
{
    int val;

    switch (nb) {
      case 1:
        val = data[0];
        break;
      case 2:
        val = (data[0] << 8) | data[1];
        break;
      case 3:
        val = (data[0] << 16) | (data[1] << 8) | data[2];
        break;
      case 4:
        val = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        break;
      default:
        return -1;
    }

    return val;
}









static PRInt32
getDERItemLength(unsigned char *data, unsigned char *end,
                 unsigned long *bytesUsed, PRBool *indefinite)
{
  unsigned char lbyte = *data++;
  PRInt32 length = -1;
  
  *indefinite = PR_FALSE;
  if (lbyte >= 0x80) {
    
    unsigned nb = (unsigned) (lbyte & 0x7f);
    if (nb > 4) {
      return -1;
    }
    if (nb > 0) {
    
      if ((data+nb) > end) {
        return -1;
      }
      length = getInteger256(data, nb);
      if (length < 0)
        return -1;
    } else {
      *indefinite = PR_TRUE;
      length = 0;
    }
    *bytesUsed = nb+1;
  } else {
    length = lbyte;
    *bytesUsed = 1; 
  }
  return length;
}

static nsresult
buildASN1ObjectFromDER(unsigned char *data,
                       unsigned char *end,
                       nsIASN1Sequence *parent)
{
  nsresult rv;
  nsCOMPtr<nsIASN1Sequence> sequence;
  nsCOMPtr<nsIASN1PrintableItem> printableItem;
  nsCOMPtr<nsIASN1Object> asn1Obj;
  nsCOMPtr<nsIMutableArray> parentObjects;

  NS_ENSURE_ARG_POINTER(parent);
  if (data >= end)
    return NS_OK;

  unsigned char code, tagnum;

  
  
  
  
  
  
  unsigned long bytesUsed;
  PRBool indefinite;
  PRInt32 len;
  PRUint32 type;

  rv = parent->GetASN1Objects(getter_AddRefs(parentObjects));
  if (NS_FAILED(rv) || parentObjects == nsnull)
    return NS_ERROR_FAILURE;
  while (data < end) {
    code = *data;
    tagnum = code & SEC_ASN1_TAGNUM_MASK;

    


    if (tagnum == SEC_ASN1_HIGH_TAG_NUMBER) {
      return NS_ERROR_FAILURE;
    }
    data++;
    len = getDERItemLength(data, end, &bytesUsed, &indefinite);
    data += bytesUsed;
    if ((len < 0) || ((data+len) > end))
      return NS_ERROR_FAILURE;

    if (code & SEC_ASN1_CONSTRUCTED) {
      if (len > 0 || indefinite) {
        sequence = new nsNSSASN1Sequence();
        switch (code & SEC_ASN1_CLASS_MASK) {
        case SEC_ASN1_UNIVERSAL:
          type = tagnum;
          break;
        case SEC_ASN1_APPLICATION:
          type = nsIASN1Object::ASN1_APPLICATION;
          break;
        case SEC_ASN1_CONTEXT_SPECIFIC:
          type = nsIASN1Object::ASN1_CONTEXT_SPECIFIC;
          break;
        case SEC_ASN1_PRIVATE:
          type = nsIASN1Object::ASN1_PRIVATE;
          break;
        default:
          NS_ERROR("Bad DER");
          return NS_ERROR_FAILURE;
        }
        sequence->SetTag(tagnum);
        sequence->SetType(type);
        rv = buildASN1ObjectFromDER(data, (len == 0) ? end : data + len, 
                                    sequence);
        asn1Obj = sequence;
      }
    } else {
      printableItem = new nsNSSASN1PrintableItem();

      asn1Obj = printableItem;
      asn1Obj->SetType(tagnum);
      asn1Obj->SetTag(tagnum); 
      printableItem->SetData((char*)data, len);
    }
    data += len;
    parentObjects->AppendElement(asn1Obj, PR_FALSE);
  }

  return NS_OK;
}

nsresult
CreateFromDER(unsigned char *data,
              unsigned int   len,
              nsIASN1Object **retval)
{
  nsCOMPtr<nsIASN1Sequence> sequence = new nsNSSASN1Sequence;
  *retval = nsnull;
  
  nsresult rv =  buildASN1ObjectFromDER(data, data+len, sequence);

  if (NS_SUCCEEDED(rv)) {
    
    
    nsCOMPtr<nsIMutableArray> elements;

    sequence->GetASN1Objects(getter_AddRefs(elements));
    nsCOMPtr<nsIASN1Object> asn1Obj = do_QueryElementAt(elements, 0);
    *retval = asn1Obj;
    if (*retval == nsnull)
      return NS_ERROR_FAILURE;

    NS_ADDREF(*retval);
      
  }
  return rv; 
}

nsNSSASN1Sequence::nsNSSASN1Sequence() : mType(0),
                                         mTag(0),
                                         mIsValidContainer(PR_TRUE),
                                         mIsExpanded(PR_TRUE)
{
  
}

nsNSSASN1Sequence::~nsNSSASN1Sequence()
{
  
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetASN1Objects(nsIMutableArray * *aASN1Objects)
{
  if (mASN1Objects == nsnull) {
    mASN1Objects = do_CreateInstance(NS_ARRAY_CONTRACTID);
  }
  *aASN1Objects = mASN1Objects;
  NS_IF_ADDREF(*aASN1Objects);
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::SetASN1Objects(nsIMutableArray * aASN1Objects)
{
  mASN1Objects = aASN1Objects;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetTag(PRUint32 *aTag)
{
  *aTag = mTag;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::SetTag(PRUint32 aTag)
{
  mTag = aTag;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetType(PRUint32 *aType)
{
  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::SetType(PRUint32 aType)
{
  mType = aType;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetDisplayName(nsAString &aDisplayName)
{
  aDisplayName = mDisplayName;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::SetDisplayName(const nsAString &aDisplayName)
{
  mDisplayName = aDisplayName;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetDisplayValue(nsAString &aDisplayValue)
{
  aDisplayValue = mDisplayValue;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::SetDisplayValue(const nsAString &aDisplayValue)
{
  mDisplayValue = aDisplayValue;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetIsValidContainer(PRBool *aIsValidContainer)
{
  NS_ENSURE_ARG_POINTER(aIsValidContainer);
  *aIsValidContainer = mIsValidContainer;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSASN1Sequence::SetIsValidContainer(PRBool aIsValidContainer)
{
  mIsValidContainer = aIsValidContainer;
  SetIsExpanded(mIsValidContainer);
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::GetIsExpanded(PRBool *aIsExpanded)
{
  NS_ENSURE_ARG_POINTER(aIsExpanded);
  *aIsExpanded = mIsExpanded;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Sequence::SetIsExpanded(PRBool aIsExpanded)
{
  mIsExpanded = aIsExpanded;
  return NS_OK;
}


nsNSSASN1PrintableItem::nsNSSASN1PrintableItem() : mType(0),
                                                   mTag(0),
                                                   mData(nsnull),
                                                   mLen(0)
{
  
}

nsNSSASN1PrintableItem::~nsNSSASN1PrintableItem()
{
  
  if (mData)
    nsMemory::Free(mData);
}


NS_IMETHODIMP 
nsNSSASN1PrintableItem::GetDisplayValue(nsAString &aValue)
{
  aValue = mValue;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::SetDisplayValue(const nsAString &aValue)
{
  mValue = aValue;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::GetTag(PRUint32 *aTag)
{
  *aTag = mTag;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::SetTag(PRUint32 aTag)
{
  mTag = aTag;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::GetType(PRUint32 *aType)
{
  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::SetType(PRUint32 aType)
{
  mType = aType;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::SetData(char *data, PRUint32 len)
{
  if (len > 0) {
    if (mLen < len) {
      unsigned char* newData = (unsigned char*)nsMemory::Realloc(mData, len);
      if (!newData)
        return NS_ERROR_OUT_OF_MEMORY;

      mData = newData;
    }

    memcpy(mData, data, len);
  } else if (len == 0) {
    if (mData) {
      nsMemory::Free(mData);
      mData = nsnull;
    }
  }
  mLen = len;
  return NS_OK;  
}

NS_IMETHODIMP
nsNSSASN1PrintableItem::GetData(char **outData, PRUint32 *outLen)
{
  NS_ENSURE_ARG_POINTER(outData);
  NS_ENSURE_ARG_POINTER(outLen);

  *outData = (char*)mData;
  *outLen  = mLen;
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1PrintableItem::GetDisplayName(nsAString &aDisplayName)
{
  aDisplayName = mDisplayName;
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1PrintableItem::SetDisplayName(const nsAString &aDisplayName)
{
  mDisplayName = aDisplayName;
  return NS_OK;
}

