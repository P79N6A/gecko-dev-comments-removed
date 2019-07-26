





#ifndef HTMLPropertiesCollection_h_
#define HTMLPropertiesCollection_h_

#include "nsDOMLists.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIMutationObserver.h"
#include "nsStubMutationObserver.h"
#include "nsBaseHashtable.h"
#include "nsINodeList.h"
#include "nsIHTMLCollection.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"
#include "jsapi.h"

class nsGenericHTMLElement;
class nsIDocument;
class nsINode;

namespace mozilla {
namespace dom {

class HTMLPropertiesCollection;
class PropertyNodeList;
class Element;

class PropertyStringList : public nsDOMStringList
{
public:
  PropertyStringList(HTMLPropertiesCollection* aCollection);
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(PropertyStringList)
  NS_DECL_NSIDOMDOMSTRINGLIST

  bool ContainsInternal(const nsAString& aString);

protected:
  nsRefPtr<HTMLPropertiesCollection> mCollection;
};

class HTMLPropertiesCollection : public nsIHTMLCollection,
                                 public nsStubMutationObserver,
                                 public nsWrapperCache
{
  friend class PropertyNodeList;
  friend class PropertyStringList;
public:
  HTMLPropertiesCollection(nsGenericHTMLElement* aRoot);
  virtual ~HTMLPropertiesCollection();

  using nsWrapperCache::GetWrapperPreserveColor;
  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  virtual Element* GetElementAt(uint32_t aIndex);

  void SetDocument(nsIDocument* aDocument);
  nsINode* GetParentObject();
  virtual JSObject* NamedItem(JSContext* cx, const nsAString& name,
                              mozilla::ErrorResult& error);
  PropertyNodeList* NamedItem(const nsAString& aName);
  PropertyNodeList* NamedGetter(const nsAString& aName, bool& aFound)
  {
    aFound = IsSupportedNamedProperty(aName);
    return aFound ? NamedItem(aName) : nullptr;
  }
  nsDOMStringList* Names()
  {
    EnsureFresh();
    return mNames;
  }
  virtual void GetSupportedNames(nsTArray<nsString>& aNames);

  NS_DECL_NSIDOMHTMLCOLLECTION

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(HTMLPropertiesCollection,
                                                         nsIHTMLCollection)

protected:
  
  void EnsureFresh();

  
  void CrawlProperties();

  
  void CrawlSubtree(Element* startNode);

  bool IsSupportedNamedProperty(const nsAString& aName)
  {
    EnsureFresh();
    return mNames->ContainsInternal(aName);
  }

  
  nsTArray<nsRefPtr<nsGenericHTMLElement> > mProperties;

  
  nsRefPtr<PropertyStringList> mNames;

  
  nsRefPtrHashtable<nsStringHashKey, PropertyNodeList> mNamedItemEntries;

  
  nsCOMPtr<nsGenericHTMLElement> mRoot;

  
  nsCOMPtr<nsIDocument> mDoc;

  
  bool mIsDirty;
};

class PropertyNodeList : public nsINodeList,
                         public nsStubMutationObserver
{
public:
  PropertyNodeList(HTMLPropertiesCollection* aCollection,
                   nsIContent* aRoot, const nsAString& aName);
  virtual ~PropertyNodeList();

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  void SetDocument(nsIDocument* aDocument);

  void GetValues(JSContext* aCx, nsTArray<JS::Value >& aResult,
                 ErrorResult& aError);

  virtual nsIContent* Item(uint32_t aIndex);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(PropertyNodeList,
                                                         nsINodeList)
  NS_DECL_NSIDOMNODELIST

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  
  virtual int32_t IndexOf(nsIContent* aContent);
  virtual nsINode* GetParentObject();

  void AppendElement(nsGenericHTMLElement* aElement)
  {
    mElements.AppendElement(aElement);
  }

  void Clear()
  {
    mElements.Clear();
  }

  void SetDirty() { mIsDirty = true; }

protected:
  
  void EnsureFresh();

  
  nsString mName;

  
  nsCOMPtr<nsIDocument> mDoc;

  
  nsRefPtr<HTMLPropertiesCollection> mCollection;

  
  nsCOMPtr<nsINode> mParent;

  
  nsTArray<nsRefPtr<nsGenericHTMLElement> > mElements;

  
  bool mIsDirty;
};

} 
} 
#endif 
