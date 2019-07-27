





#include "InterfaceInitFuncs.h"

#include "Accessible-inl.h"
#include "AccessibleWrap.h"
#include "DocAccessible.h"
#include "nsMai.h"
#include "mozilla/Likely.h"

using namespace mozilla::a11y;

static const char* const kDocTypeName = "W3C-doctype";
static const char* const kDocUrlName = "DocURL";
static const char* const kMimeTypeName = "MimeType";


extern "C" {

static const gchar* getDocumentLocaleCB(AtkDocument* aDocument);
static AtkAttributeSet* getDocumentAttributesCB(AtkDocument* aDocument);
static const gchar* getDocumentAttributeValueCB(AtkDocument* aDocument,
                                                const gchar* aAttrName);

void
documentInterfaceInitCB(AtkDocumentIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid Interface");
    if(MOZ_UNLIKELY(!aIface))
        return;

    




    aIface->get_document_attributes = getDocumentAttributesCB;
    aIface->get_document_attribute_value = getDocumentAttributeValueCB;
    aIface->get_document_locale = getDocumentLocaleCB;
}

const gchar *
getDocumentLocaleCB(AtkDocument *aDocument)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
  if (!accWrap)
    return nullptr;

  nsAutoString locale;
  accWrap->Language(locale);
  return locale.IsEmpty() ? nullptr : AccessibleWrap::ReturnString(locale);
}

static inline GSList *
prependToList(GSList *aList, const char *const aName, const nsAutoString &aValue)
{
  if (aValue.IsEmpty())
    return aList;

    
    AtkAttribute *atkAttr = (AtkAttribute *)g_malloc(sizeof(AtkAttribute));
    atkAttr->name = g_strdup(aName);
    atkAttr->value = g_strdup(NS_ConvertUTF16toUTF8(aValue).get());
    return g_slist_prepend(aList, atkAttr);
}

AtkAttributeSet *
getDocumentAttributesCB(AtkDocument *aDocument)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
  if (!accWrap || !accWrap->IsDoc())
    return nullptr;

  
  GSList* attributes = nullptr;
  DocAccessible* document = accWrap->AsDoc();
  nsAutoString aURL;
  document->URL(aURL);
  attributes = prependToList(attributes, kDocUrlName, aURL);

  nsAutoString aW3CDocType;
  document->GetDocType(aW3CDocType);
  attributes = prependToList(attributes, kDocTypeName, aW3CDocType);

  nsAutoString aMimeType;
  document->MimeType(aMimeType);
  attributes = prependToList(attributes, kMimeTypeName, aMimeType);

  return attributes;
}

const gchar *
getDocumentAttributeValueCB(AtkDocument *aDocument,
                            const gchar *aAttrName)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
  if (!accWrap || !accWrap->IsDoc())
    return nullptr;

  DocAccessible* document = accWrap->AsDoc();
  nsresult rv;
  nsAutoString attrValue;
  if (!strcasecmp(aAttrName, kDocTypeName))
    rv = document->GetDocType(attrValue);
  else if (!strcasecmp(aAttrName, kDocUrlName))
    document->URL(attrValue);
  else if (!strcasecmp(aAttrName, kMimeTypeName))
    document->MimeType(attrValue);
  else
    return nullptr;

  NS_ENSURE_SUCCESS(rv, nullptr);
  return attrValue.IsEmpty() ? nullptr : AccessibleWrap::ReturnString(attrValue);
}
}
