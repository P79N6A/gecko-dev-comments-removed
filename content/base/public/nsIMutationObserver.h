




#ifndef nsIMutationObserver_h
#define nsIMutationObserver_h

#include "nsISupports.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsINode;

namespace mozilla {
namespace dom {
class Element;
} 
} 

#define NS_IMUTATION_OBSERVER_IID \
{ 0x16fe5e3e, 0xeadc, 0x4312, \
  { 0x9d, 0x44, 0xb6, 0xbe, 0xdd, 0x6b, 0x54, 0x74 } }






struct CharacterDataChangeInfo
{
  


  bool mAppend;

  


  uint32_t mChangeStart;

  




  uint32_t mChangeEnd;

  



  uint32_t mReplaceLength;

  







  struct Details {
    enum {
      eMerge,  
      eSplit   
    } mType;
    



    nsIContent* mNextSibling;
  };

  


  Details* mDetails;
};















class nsIMutationObserver : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMUTATION_OBSERVER_IID)

  

















  virtual void CharacterDataWillChange(nsIDocument *aDocument,
                                       nsIContent* aContent,
                                       CharacterDataChangeInfo* aInfo) = 0;

  

















  virtual void CharacterDataChanged(nsIDocument *aDocument,
                                    nsIContent* aContent,
                                    CharacterDataChangeInfo* aInfo) = 0;

  




















  virtual void AttributeWillChange(nsIDocument* aDocument,
                                   mozilla::dom::Element* aElement,
                                   int32_t      aNameSpaceID,
                                   nsIAtom*     aAttribute,
                                   int32_t      aModType) = 0;

  
















  virtual void AttributeChanged(nsIDocument* aDocument,
                                mozilla::dom::Element* aElement,
                                int32_t      aNameSpaceID,
                                nsIAtom*     aAttribute,
                                int32_t      aModType) = 0;

  








  virtual void AttributeSetToCurrentValue(nsIDocument* aDocument,
                                          mozilla::dom::Element* aElement,
                                          int32_t aNameSpaceID,
                                          nsIAtom* aAttribute) {}

  
















  virtual void ContentAppended(nsIDocument *aDocument,
                               nsIContent* aContainer,
                               nsIContent* aFirstNewContent,
                               int32_t     aNewIndexInContainer) = 0;

  


















  virtual void ContentInserted(nsIDocument *aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               int32_t aIndexInContainer) = 0;

  





















  virtual void ContentRemoved(nsIDocument *aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              int32_t aIndexInContainer,
                              nsIContent* aPreviousSibling) = 0;

 
















  virtual void NodeWillBeDestroyed(const nsINode *aNode) = 0;

  















  virtual void ParentChainChanged(nsIContent *aContent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMutationObserver, NS_IMUTATION_OBSERVER_IID)

#define NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATAWILLCHANGE                  \
    virtual void CharacterDataWillChange(nsIDocument* aDocument,             \
                                         nsIContent* aContent,               \
                                         CharacterDataChangeInfo* aInfo);

#define NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED                     \
    virtual void CharacterDataChanged(nsIDocument* aDocument,                \
                                      nsIContent* aContent,                  \
                                      CharacterDataChangeInfo* aInfo);

#define NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE                      \
    virtual void AttributeWillChange(nsIDocument* aDocument,                 \
                                     mozilla::dom::Element* aElement,        \
                                     int32_t aNameSpaceID,                   \
                                     nsIAtom* aAttribute,                    \
                                     int32_t aModType);

#define NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED                         \
    virtual void AttributeChanged(nsIDocument* aDocument,                    \
                                  mozilla::dom::Element* aElement,           \
                                  int32_t aNameSpaceID,                      \
                                  nsIAtom* aAttribute,                       \
                                  int32_t aModType);

#define NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED                          \
    virtual void ContentAppended(nsIDocument* aDocument,                     \
                                 nsIContent* aContainer,                     \
                                 nsIContent* aFirstNewContent,               \
                                 int32_t aNewIndexInContainer);

#define NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED                          \
    virtual void ContentInserted(nsIDocument* aDocument,                     \
                                 nsIContent* aContainer,                     \
                                 nsIContent* aChild,                         \
                                 int32_t aIndexInContainer);

#define NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED                           \
    virtual void ContentRemoved(nsIDocument* aDocument,                      \
                                nsIContent* aContainer,                      \
                                nsIContent* aChild,                          \
                                int32_t aIndexInContainer,                   \
                                nsIContent* aPreviousSibling);

#define NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED                      \
    virtual void NodeWillBeDestroyed(const nsINode* aNode);

#define NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED                       \
    virtual void ParentChainChanged(nsIContent *aContent);

#define NS_DECL_NSIMUTATIONOBSERVER                                          \
    NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATAWILLCHANGE                      \
    NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED                         \
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE                          \
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED                             \
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED                              \
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED                              \
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED                               \
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED                          \
    NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED

#define NS_IMPL_NSIMUTATIONOBSERVER_CORE_STUB(_class)                     \
void                                                                      \
_class::NodeWillBeDestroyed(const nsINode* aNode)                               \
{                                                                         \
}

#define NS_IMPL_NSIMUTATIONOBSERVER_CONTENT(_class)                       \
void                                                                      \
_class::CharacterDataWillChange(nsIDocument* aDocument,                   \
                                nsIContent* aContent,                     \
                                CharacterDataChangeInfo* aInfo)           \
{                                                                         \
}                                                                         \
void                                                                      \
_class::CharacterDataChanged(nsIDocument* aDocument,                      \
                             nsIContent* aContent,                        \
                             CharacterDataChangeInfo* aInfo)              \
{                                                                         \
}                                                                         \
void                                                                      \
_class::AttributeWillChange(nsIDocument* aDocument,                       \
                            mozilla::dom::Element* aElement,              \
                            int32_t aNameSpaceID,                         \
                            nsIAtom* aAttribute,                          \
                            int32_t aModType)                             \
{                                                                         \
}                                                                         \
void                                                                      \
_class::AttributeChanged(nsIDocument* aDocument,                          \
                         mozilla::dom::Element* aElement,                 \
                         int32_t aNameSpaceID,                            \
                         nsIAtom* aAttribute,                             \
                         int32_t aModType)                                \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ContentAppended(nsIDocument* aDocument,                           \
                        nsIContent* aContainer,                           \
                        nsIContent* aFirstNewContent,                     \
                        int32_t aNewIndexInContainer)                     \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ContentInserted(nsIDocument* aDocument,                           \
                        nsIContent* aContainer,                           \
                        nsIContent* aChild,                               \
                        int32_t aIndexInContainer)                        \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ContentRemoved(nsIDocument* aDocument,                            \
                       nsIContent* aContainer,                            \
                       nsIContent* aChild,                                \
                       int32_t aIndexInContainer,                         \
                       nsIContent* aPreviousSibling)                      \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ParentChainChanged(nsIContent *aContent)                          \
{                                                                         \
}


#endif 
