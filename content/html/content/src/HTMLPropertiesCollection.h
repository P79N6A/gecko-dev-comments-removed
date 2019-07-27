





#ifndef HTMLPropertiesCollection_h_
#define HTMLPropertiesCollection_h_

#include "mozilla/Attributes.h"
#include "mozilla/dom/DOMStringList.h"
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
#include "nsGenericHTMLElement.h"

class nsIDocument;
class nsINode;

namespace mozilla {
namespace dom {

class HTMLPropertiesCollection;
class PropertyNodeList;
class Element;

class PropertyStringList : public DOMStringList
{
public:
  explicit PropertyStringList(HTMLPropertiesCollection* aCollection);
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(PropertyStringList, DOMStringList)

  bool ContainsInternal(const nsAString& aString);

protected:
  virtual ~PropertyStringList();

  virtual void EnsureFresh() MOZ_OVERRIDE;

  nsRefPtr<HTMLPropertiesCollection> mCollection;
};

class HTMLPropertiesCollection : public nsIHTMLCollection,
                                 public nsStubMutationObserver,
                                 public nsWrapperCache
{
  friend class PropertyNodeList;
  friend class PropertyStringList;
public:
  explicit HTMLPropertiesCollection(nsGenericHTMLElement* aRoot);

  
  using nsWrapperCache::GetWrapperPreserveColor;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
protected:
  virtual ~HTMLPropertiesCollection();

  virtual JSObject* GetWrapperPreserveColorInternal() MOZ_OVERRIDE
  {
    return nsWrapperCache::GetWrapperPreserveColor();
  }
public:

  virtual Element* GetElementAt(uint32_t aIndex);

  void SetDocument(nsIDocument* aDocument);
  nsINode* GetParentObject() MOZ_OVERRIDE;

  virtual Element*
  GetFirstNamedElement(const nsAString& aName, bool& aFound) MOZ_OVERRIDE
  {
    
    
    
    aFound = false;
    return nullptr;
  }
  PropertyNodeList* NamedItem(const nsAString& aName);
  PropertyNodeList* NamedGetter(const nsAString& aName, bool& aFound)
  {
    aFound = IsSupportedNamedProperty(aName);
    return aFound ? NamedItem(aName) : nullptr;
  }
  bool NameIsEnumerable(const nsAString& aName)
  {
    return true;
  }
  DOMStringList* Names()
  {
    EnsureFresh();
    return mNames;
  }
  virtual void GetSupportedNames(unsigned,
                                 nsTArray<nsString>& aNames) MOZ_OVERRIDE;

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

  
  nsRefPtr<nsGenericHTMLElement> mRoot;

  
  nsCOMPtr<nsIDocument> mDoc;

  
  bool mIsDirty;
};

class PropertyNodeList : public nsINodeList,
                         public nsStubMutationObserver
{
public:
  PropertyNodeList(HTMLPropertiesCollection* aCollection,
                   nsIContent* aRoot, const nsAString& aName);

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  void SetDocument(nsIDocument* aDocument);

  void GetValues(JSContext* aCx, nsTArray<JS::Value >& aResult,
                 ErrorResult& aError);

  virtual nsIContent* Item(uint32_t aIndex) MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(PropertyNodeList,
                                                         nsINodeList)
  NS_DECL_NSIDOMNODELIST

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  
  virtual int32_t IndexOf(nsIContent* aContent) MOZ_OVERRIDE;
  virtual nsINode* GetParentObject() MOZ_OVERRIDE;

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
  virtual ~PropertyNodeList();

  
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
