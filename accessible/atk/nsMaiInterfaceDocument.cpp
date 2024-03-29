





#include "InterfaceInitFuncs.h"

#include "Accessible-inl.h"
#include "AccessibleWrap.h"
#include "DocAccessible.h"
#include "nsMai.h"
#include "ProxyAccessible.h"
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
  nsAutoString locale;
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
  if (accWrap) {
    accWrap->Language(locale);
  } else if (ProxyAccessible* proxy = GetProxy(ATK_OBJECT(aDocument))) {
    proxy->Language(locale);
  }

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
  nsAutoString url;
  nsAutoString w3cDocType;
  nsAutoString mimeType;
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
  if (accWrap) {
    if (!accWrap->IsDoc()) {
      return nullptr;
    }

    DocAccessible* document = accWrap->AsDoc();
    document->URL(url);
    document->DocType(w3cDocType);
    document->MimeType(mimeType);
  } else if (ProxyAccessible* proxy = GetProxy(ATK_OBJECT(aDocument))) {
    proxy->URLDocTypeMimeType(url, w3cDocType, mimeType);
  } else {
    return nullptr;
  }

  
  GSList* attributes = nullptr;
  attributes = prependToList(attributes, kDocUrlName, url);
  attributes = prependToList(attributes, kDocTypeName, w3cDocType);
  attributes = prependToList(attributes, kMimeTypeName, mimeType);

  return attributes;
}

const gchar *
getDocumentAttributeValueCB(AtkDocument *aDocument,
                            const gchar *aAttrName)
{
  ProxyAccessible* proxy = nullptr;
  DocAccessible* document = nullptr;
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aDocument));
  if (accWrap) {
    if (!accWrap->IsDoc()) {
      return nullptr;
    }

    document = accWrap->AsDoc();
  } else {
    proxy = GetProxy(ATK_OBJECT(aDocument));
    if (!proxy) {
      return nullptr;
    }
  }

  nsAutoString attrValue;
  if (!strcasecmp(aAttrName, kDocTypeName)) {
    if (document) {
      document->DocType(attrValue);
    } else {
      proxy->DocType(attrValue);
    }
  } else if (!strcasecmp(aAttrName, kDocUrlName)) {
    if (document) {
      document->URL(attrValue);
    } else {
      proxy->URL(attrValue);
    }
  } else if (!strcasecmp(aAttrName, kMimeTypeName)) {
    if (document) {
      document->MimeType(attrValue);
    } else {
      proxy->MimeType(attrValue);
    }
  } else {
    return nullptr;
  }

  return attrValue.IsEmpty() ? nullptr : AccessibleWrap::ReturnString(attrValue);
}
}
