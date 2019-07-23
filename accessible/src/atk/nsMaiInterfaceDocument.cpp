







































#include "nsAccessibleWrap.h"
#include "nsMaiInterfaceDocument.h"

const char *const kDocTypeName = "W3C-doctype";
const char *const kDocUrlName = "DocURL";
const char *const kMimeTypeName = "MimeType";

void
documentInterfaceInitCB(AtkDocumentIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid Interface");
    if(!aIface)
        return;

    




    aIface->get_document_attributes = getDocumentAttributesCB;
    aIface->get_document_attribute_value = getDocumentAttributeValueCB;
}

const gchar *
getDocumentTypeCB(AtkDocument *aDocument)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsCOMPtr<nsIAccessibleDocument> accDocument;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleDocument),
                            getter_AddRefs(accDocument));
    NS_ENSURE_TRUE(accDocument, nsnull);

    nsAutoString aMimeType;
    nsresult rv = accDocument->GetMimeType(aMimeType);
    NS_ENSURE_SUCCESS(rv, nsnull);
    return nsAccessibleWrap::ReturnString(aMimeType);
}

static inline GSList *
prependToList(GSList *aList, const char *const aName, const nsAutoString &aValue)
{
    
    AtkAttribute *atkAttr = (AtkAttribute *)g_malloc(sizeof(AtkAttribute));
    atkAttr->name = g_strdup(aName);
    atkAttr->value = g_strdup(NS_ConvertUTF16toUTF8(aValue).get());
    return g_slist_prepend(aList, atkAttr);
}

AtkAttributeSet *
getDocumentAttributesCB(AtkDocument *aDocument)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsCOMPtr<nsIAccessibleDocument> accDocument;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleDocument),
                            getter_AddRefs(accDocument));
    NS_ENSURE_TRUE(accDocument, nsnull);

    
    GSList *attributes = nsnull;

    nsAutoString aURL;
    nsresult rv = accDocument->GetURL(aURL);
    if (NS_SUCCEEDED(rv)) {
        attributes = prependToList(attributes, kDocUrlName, aURL);
    }
    nsAutoString aW3CDocType;
    rv = accDocument->GetDocType(aW3CDocType);
    if (NS_SUCCEEDED(rv)) {
        attributes = prependToList(attributes, kDocTypeName, aW3CDocType);
    }
    nsAutoString aMimeType;
    rv = accDocument->GetMimeType(aMimeType);
    if (NS_SUCCEEDED(rv)) {
        attributes = prependToList(attributes, kMimeTypeName, aMimeType);
    }
    
    return attributes;
}

const gchar *
getDocumentAttributeValueCB(AtkDocument *aDocument,
                            const gchar *aAttrName)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsCOMPtr<nsIAccessibleDocument> accDocument;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleDocument),
                            getter_AddRefs(accDocument));
    NS_ENSURE_TRUE(accDocument, nsnull);

    nsresult rv;
    nsAutoString attrValue;
    if (!g_ascii_strcasecmp(aAttrName, kDocTypeName)) {
        rv = accDocument->GetDocType(attrValue);
        NS_ENSURE_SUCCESS(rv, nsnull);
    }
    else if (!g_ascii_strcasecmp(aAttrName, kDocUrlName)) {
        rv = accDocument->GetURL(attrValue);
        NS_ENSURE_SUCCESS(rv, nsnull);
    }
    else if (!g_ascii_strcasecmp(aAttrName, kMimeTypeName)) {
        rv = accDocument->GetMimeType(attrValue);
        NS_ENSURE_SUCCESS(rv, nsnull);
    }
    else {
        return nsnull;
    }
    return nsAccessibleWrap::ReturnString(attrValue);
}
