



















































#include "mozSanitizingSerializer.h"
#include "nsIServiceManager.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsTextFragment.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "plstr.h"
#include "nsIProperties.h"
#include "nsUnicharUtils.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsEscape.h"



static inline PRUnichar* escape(const nsString& source)
{
  return nsEscapeHTML2(source.get(), source.Length()); 
}







#define TEXT_REMOVED "&lt;Text removed&gt;"
#define TEXT_BREAKER "|"

nsresult NS_NewSanitizingHTMLSerializer(nsIContentSerializer** aSerializer)
{
  mozSanitizingHTMLSerializer* it = new mozSanitizingHTMLSerializer();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(it);
  *aSerializer = it;
  return NS_OK;
}

mozSanitizingHTMLSerializer::mozSanitizingHTMLSerializer()
  : mSkipLevel(0),
    mAllowedTags(30) 
{
  mOutputString = nsnull;
}

mozSanitizingHTMLSerializer::~mozSanitizingHTMLSerializer()
{
#ifdef DEBUG_BenB
  printf("Output:\n%s\n", NS_LossyConvertUTF16toASCII(*mOutputString).get());
#endif
  mAllowedTags.Enumerate(ReleaseProperties);
}


PRBool
mozSanitizingHTMLSerializer::ReleaseProperties(nsHashKey* key, void* data,
                                               void* closure)
{
  nsIProperties* prop = (nsIProperties*)data;
  NS_IF_RELEASE(prop);
  return PR_TRUE;
}


NS_IMPL_ISUPPORTS4(mozSanitizingHTMLSerializer,
                   nsIContentSerializer,
                   nsIContentSink,
                   nsIHTMLContentSink,
                   mozISanitizingHTMLSerializer)


NS_IMETHODIMP 
mozSanitizingHTMLSerializer::Init(PRUint32 aFlags, PRUint32 dummy,
                                  const char* aCharSet, PRBool aIsCopying,
                                  PRBool aIsWholeDocument)
{
  NS_ENSURE_TRUE(nsContentUtils::GetParserService(), NS_ERROR_UNEXPECTED);

  return NS_OK;
}

NS_IMETHODIMP
mozSanitizingHTMLSerializer::Initialize(nsAString* aOutString,
                                        PRUint32 aFlags,
                                        const nsAString& allowedTags)
{
  nsresult rv = Init(aFlags, 0, nsnull, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  mOutputString = aOutString;

  ParsePrefs(allowedTags);

  return NS_OK;
}


NS_IMETHODIMP
mozSanitizingHTMLSerializer::Flush(nsAString& aStr)
{
#ifdef DEBUG_BenB
  printf("Flush: -%s-", NS_LossyConvertUTF16toASCII(aStr).get());
#endif
  Write(aStr);
  return NS_OK;
}

NS_IMETHODIMP
mozSanitizingHTMLSerializer::AppendDocumentStart(nsIDOMDocument *aDocument,
                                                 nsAString& aStr)
{
  return NS_OK;
}

void
mozSanitizingHTMLSerializer::Write(const nsAString& aString)
{
  mOutputString->Append(aString);
}


NS_IMETHODIMP
mozSanitizingHTMLSerializer::IsEnabled(PRInt32 aTag, PRBool* aReturn)
{
  *aReturn = PR_FALSE;
  return NS_OK;
}





PRBool
mozSanitizingHTMLSerializer::IsContainer(PRInt32 aId)
{
  PRBool isContainer = PR_FALSE;

  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (parserService) {
    parserService->IsContainer(aId, isContainer);
  }

  return isContainer;
}










PRInt32
mozSanitizingHTMLSerializer::GetIdForContent(nsIContent* aContent)
{
  if (!aContent->IsNodeOfType(nsINode::eHTML)) {
    return eHTMLTag_unknown;
  }

  nsIParserService* parserService = nsContentUtils::GetParserService();

  return parserService ? parserService->HTMLAtomTagToId(aContent->Tag()) :
                         eHTMLTag_unknown;
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::AppendText(nsIDOMText* aText, 
                                        PRInt32 aStartOffset,
                                        PRInt32 aEndOffset, 
                                        nsAString& aStr)
{
  nsresult rv = NS_OK;

  mOutputString = &aStr;

  nsAutoString linebuffer;
  rv = DoAddLeaf(eHTMLTag_text, linebuffer);

  return rv;
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::AppendElementStart(nsIDOMElement *aElement,
                                                nsIDOMElement *aOriginalElement,
                                                nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  mContent = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(mContent, NS_ERROR_FAILURE);

  mOutputString = &aStr;

  PRInt32 id = GetIdForContent(mContent);

  PRBool isContainer = IsContainer(id);

  nsresult rv;
  if (isContainer) {
    rv = DoOpenContainer(id);
  }
  else {
    rv = DoAddLeaf(id, EmptyString());
  }

  mContent = 0;
  mOutputString = nsnull;

  return rv;
} 
 
NS_IMETHODIMP 
mozSanitizingHTMLSerializer::AppendElementEnd(nsIDOMElement *aElement,
                                              nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  mContent = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(mContent, NS_ERROR_FAILURE);

  mOutputString = &aStr;

  nsresult rv = NS_OK;
  PRInt32 id = GetIdForContent(mContent);

  PRBool isContainer = IsContainer(id);

  if (isContainer) {
    rv = DoCloseContainer(id);
  }

  mContent = 0;
  mOutputString = nsnull;

  return rv;
}

NS_IMETHODIMP
mozSanitizingHTMLSerializer::OpenContainer(const nsIParserNode& aNode)
{
  PRInt32 type = aNode.GetNodeType();

  mParserNode = const_cast<nsIParserNode *>(&aNode);
  return DoOpenContainer(type);
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::CloseContainer(const nsHTMLTag aTag)
{
  return DoCloseContainer(aTag);
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::AddLeaf(const nsIParserNode& aNode)
{
  eHTMLTags type = (eHTMLTags)aNode.GetNodeType();
  const nsAString& text = aNode.GetText();

  mParserNode = const_cast<nsIParserNode*>(&aNode);
  return DoAddLeaf(type, text);
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::AddDocTypeDecl(const nsIParserNode& aNode)
{
  return NS_OK;
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::SetDocumentCharset(nsACString& aCharset)
{
  
  Write(NS_LITERAL_STRING("\n<meta http-equiv=\"Context-Type\" content=\"text/html; charset=")
        

        + nsAdoptingString(escape(NS_ConvertASCIItoUTF16(aCharset)))
        + NS_LITERAL_STRING("\">\n"));
  return NS_OK;
}

NS_IMETHODIMP 
mozSanitizingHTMLSerializer::OpenHead()
{
  
  
  return NS_OK;
}



nsresult
mozSanitizingHTMLSerializer::DoOpenContainer(PRInt32 aTag)
{
  eHTMLTags type = (eHTMLTags)aTag;

  if (mSkipLevel == 0 && IsAllowedTag(type))
  {
    nsIParserService* parserService = nsContentUtils::GetParserService();
    if (!parserService)
      return NS_ERROR_OUT_OF_MEMORY;
    const PRUnichar* tag_name = parserService->HTMLIdToStringTag(aTag);
    NS_ENSURE_TRUE(tag_name, NS_ERROR_INVALID_POINTER);

    Write(NS_LITERAL_STRING("<") + nsDependentString(tag_name));

    
    if (mParserNode)
    {
      PRInt32 count = mParserNode->GetAttributeCount();
      for (PRInt32 i = 0; i < count; i++)
      {
        const nsAString& key = mParserNode->GetKeyAt(i);
        if(IsAllowedAttribute(type, key))
        {
          
          nsAutoString value(mParserNode->GetValueAt(i));
                    
          if (NS_SUCCEEDED(SanitizeAttrValue(type, key, value)))
          {
            
            Write(NS_LITERAL_STRING(" "));
            Write(key); 
            Write(NS_LITERAL_STRING("=\"") + value + NS_LITERAL_STRING("\""));
          }
        }
      }
    }

    Write(NS_LITERAL_STRING(">"));
  }
  else if (mSkipLevel != 0 || type == eHTMLTag_script || type == eHTMLTag_style)
    ++mSkipLevel;
  else
    Write(NS_LITERAL_STRING(" "));

  return NS_OK;

}

nsresult
mozSanitizingHTMLSerializer::DoCloseContainer(PRInt32 aTag)
{
  eHTMLTags type = (eHTMLTags)aTag;

  if (mSkipLevel == 0 && IsAllowedTag(type)) {
    nsIParserService* parserService = nsContentUtils::GetParserService();
    if (!parserService)
      return NS_ERROR_OUT_OF_MEMORY;
    const PRUnichar* tag_name = parserService->HTMLIdToStringTag(aTag);
    NS_ENSURE_TRUE(tag_name, NS_ERROR_INVALID_POINTER);

    Write(NS_LITERAL_STRING("</") + nsDependentString(tag_name)
          + NS_LITERAL_STRING(">"));
  }
  else if (mSkipLevel == 0)
    Write(NS_LITERAL_STRING(" "));
  else
    --mSkipLevel;

  return NS_OK;
}

nsresult
mozSanitizingHTMLSerializer::DoAddLeaf(PRInt32 aTag,
                                       const nsAString& aText)
{
  if (mSkipLevel != 0)
    return NS_OK;

  eHTMLTags type = (eHTMLTags)aTag;

  nsresult rv = NS_OK;

  if (type == eHTMLTag_whitespace ||
      type == eHTMLTag_newline)
  {
    Write(aText); 
  }
  else if (type == eHTMLTag_text)
  {
    nsAutoString text(aText);
    if(NS_SUCCEEDED(SanitizeTextNode(text)))
      Write(text);
    else
      Write(NS_LITERAL_STRING(TEXT_REMOVED)); 
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (type == eHTMLTag_entity)
  {
    Write(NS_LITERAL_STRING("&"));
    Write(aText); 
    
    
  }
  else
  {
    DoOpenContainer(type);
  }

  return rv;
}





nsresult
mozSanitizingHTMLSerializer::SanitizeTextNode(nsString& aText )
{
  aText.Adopt(escape(aText));
  return NS_OK;
}












nsresult
mozSanitizingHTMLSerializer::SanitizeAttrValue(nsHTMLTag aTag,
                                               const nsAString& anAttrName,
                                               nsString& aValue )
{
  




  aValue = Substring(aValue, 0, 1000);
  

  aValue.Adopt(escape(aValue));

  



  if (aValue.Find("javascript:") != kNotFound ||
      aValue.Find("data:") != kNotFound ||
      aValue.Find("base64") != kNotFound)
    return NS_ERROR_ILLEGAL_VALUE;

  
  if (aTag == eHTMLTag_img && 
      anAttrName.LowerCaseEqualsLiteral("src"))
  {
    nsresult rv;
    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString scheme;
    rv = ioService->ExtractScheme(NS_LossyConvertUTF16toASCII(aValue), scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!scheme.Equals("cid", nsCaseInsensitiveCStringComparator()))
      return NS_ERROR_ILLEGAL_VALUE;
  }

#ifdef DEBUG_BenB
  printf("attribute value for %s: -%s-\n",
         NS_LossyConvertUTF16toASCII(anAttrName).get(),
         NS_LossyConvertUTF16toASCII(aValue).get());
#endif

  return NS_OK;
}



PRBool
mozSanitizingHTMLSerializer::IsAllowedTag(nsHTMLTag aTag)
{

  nsPRUint32Key tag_key(aTag);
#ifdef DEBUG_BenB
  printf("IsAllowedTag %d: %s\n",
         aTag,
         mAllowedTags.Exists(&tag_key)?"yes":"no");
#endif
  return mAllowedTags.Exists(&tag_key);
}




PRBool
mozSanitizingHTMLSerializer::IsAllowedAttribute(nsHTMLTag aTag,
                                             const nsAString& anAttributeName)
{
#ifdef DEBUG_BenB
  printf("IsAllowedAttribute %d, -%s-\n",
         aTag,
         NS_LossyConvertUTF16toASCII(anAttributeName).get());
#endif
  nsresult rv;

  nsPRUint32Key tag_key(aTag);
  nsIProperties* attr_bag = (nsIProperties*)mAllowedTags.Get(&tag_key);
  NS_ENSURE_TRUE(attr_bag, PR_FALSE);

  PRBool allowed;
  nsAutoString attr(anAttributeName);
  ToLowerCase(attr);
  rv = attr_bag->Has(NS_LossyConvertUTF16toASCII(attr).get(),
                     &allowed);
  if (NS_FAILED(rv))
    return PR_FALSE;

#ifdef DEBUG_BenB
  printf(" Allowed: %s\n", allowed?"yes":"no");
#endif
  return allowed;
}



















nsresult
mozSanitizingHTMLSerializer::ParsePrefs(const nsAString& aPref)
{
  char* pref = ToNewCString(aPref);
  char* tags_lasts;
  for (char* iTag = PL_strtok_r(pref, " ", &tags_lasts);
       iTag;
       iTag = PL_strtok_r(NULL, " ", &tags_lasts))
  {
    ParseTagPref(nsCAutoString(iTag));
  }
  delete[] pref;

  return NS_OK;
}





nsresult
mozSanitizingHTMLSerializer::ParseTagPref(const nsCAutoString& tagpref)
{
  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (!parserService)
    return NS_ERROR_OUT_OF_MEMORY;

  
  PRInt32 bracket = tagpref.FindChar('(');
  if (bracket == 0)
  {
    printf(" malformed pref: %s\n", tagpref.get());
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }

  nsAutoString tag;
  CopyUTF8toUTF16(StringHead(tagpref, bracket), tag);

  
  PRInt32 tag_id = parserService->HTMLStringTagToId(tag);
  if (tag_id == eHTMLTag_userdefined)
  {
    printf(" unknown tag <%s>, won't add.\n",
           NS_ConvertUTF16toUTF8(tag).get());
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }
  nsPRUint32Key tag_key(tag_id);

  if (mAllowedTags.Exists(&tag_key))
  {
    printf(" duplicate tag: %s\n", NS_ConvertUTF16toUTF8(tag).get());
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }
  if (bracket == kNotFound)
    

  {
    mAllowedTags.Put(&tag_key, 0);
  }
  else
  {
    

    
    if(tagpref[tagpref.Length() - 1] != ')' ||
       tagpref.Length() < PRUint32(bracket) + 3)
    {
      printf(" malformed pref: %s\n", tagpref.get());
      return NS_ERROR_CANNOT_CONVERT_DATA;
    }
    nsCOMPtr<nsIProperties> attr_bag =
                                 do_CreateInstance(NS_PROPERTIES_CONTRACTID);
    NS_ENSURE_TRUE(attr_bag, NS_ERROR_INVALID_POINTER);
    nsCAutoString attrList;
    attrList.Append(Substring(tagpref,
                              bracket + 1,
                              tagpref.Length() - 2 - bracket));
    char* attrs_lasts;
    for (char* iAttr = PL_strtok_r(attrList.BeginWriting(),
                                   ",", &attrs_lasts);
         iAttr;
         iAttr = PL_strtok_r(NULL, ",", &attrs_lasts))
    {
      attr_bag->Set(iAttr, 0);
    }

    nsIProperties* attr_bag_raw = attr_bag;
    NS_ADDREF(attr_bag_raw);
    mAllowedTags.Put(&tag_key, attr_bag_raw);
  }

  return NS_OK;
}





