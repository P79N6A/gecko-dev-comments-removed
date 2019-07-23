



































#ifndef nsIMutationObserver_h___
#define nsIMutationObserver_h___

#include "nsISupports.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsINode;

#define NS_IMUTATION_OBSERVER_IID \
{0x365d600b, 0x868a, 0x452a, \
  {0x8d, 0xe8, 0xf4, 0x6f, 0xad, 0x8f, 0xee, 0x53 } }






struct CharacterDataChangeInfo
{
  


  PRBool mAppend;

  


  PRUint32 mChangeStart;

  




  PRUint32 mChangeEnd;

  



  PRUint32 mReplaceLength;

  






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
                                   nsIContent*  aContent,
                                   PRInt32      aNameSpaceID,
                                   nsIAtom*     aAttribute,
                                   PRInt32      aModType) = 0;

  










  virtual void AttributeChanged(nsIDocument* aDocument,
                                nsIContent*  aContent,
                                PRInt32      aNameSpaceID,
                                nsIAtom*     aAttribute,
                                PRInt32      aModType) = 0;

  









  virtual void ContentAppended(nsIDocument *aDocument,
                               nsIContent* aContainer,
                               PRInt32     aNewIndexInContainer) = 0;

  












  virtual void ContentInserted(nsIDocument *aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer) = 0;

  













  virtual void ContentRemoved(nsIDocument *aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer) = 0;

 












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
                                     nsIContent* aContent,                   \
                                     PRInt32 aNameSpaceID,                   \
                                     nsIAtom* aAttribute,                    \
                                     PRInt32 aModType);

#define NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED                         \
    virtual void AttributeChanged(nsIDocument* aDocument,                    \
                                  nsIContent* aContent,                      \
                                  PRInt32 aNameSpaceID,                      \
                                  nsIAtom* aAttribute,                       \
                                  PRInt32 aModType);

#define NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED                          \
    virtual void ContentAppended(nsIDocument* aDocument,                     \
                                 nsIContent* aContainer,                     \
                                 PRInt32 aNewIndexInContainer);

#define NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED                          \
    virtual void ContentInserted(nsIDocument* aDocument,                     \
                                 nsIContent* aContainer,                     \
                                 nsIContent* aChild,                         \
                                 PRInt32 aIndexInContainer);

#define NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED                           \
    virtual void ContentRemoved(nsIDocument* aDocument,                      \
                                nsIContent* aContainer,                      \
                                nsIContent* aChild,                          \
                                PRInt32 aIndexInContainer);

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
                            nsIContent* aContent,                         \
                            PRInt32 aNameSpaceID,                         \
                            nsIAtom* aAttribute,                          \
                            PRInt32 aModType)                             \
{                                                                         \
}                                                                         \
void                                                                      \
_class::AttributeChanged(nsIDocument* aDocument,                          \
                         nsIContent* aContent,                            \
                         PRInt32 aNameSpaceID,                            \
                         nsIAtom* aAttribute,                             \
                         PRInt32 aModType)                                \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ContentAppended(nsIDocument* aDocument,                           \
                        nsIContent* aContainer,                           \
                        PRInt32 aNewIndexInContainer)                     \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ContentInserted(nsIDocument* aDocument,                           \
                        nsIContent* aContainer,                           \
                        nsIContent* aChild,                               \
                        PRInt32 aIndexInContainer)                        \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ContentRemoved(nsIDocument* aDocument,                            \
                       nsIContent* aContainer,                            \
                       nsIContent* aChild,                                \
                       PRInt32 aIndexInContainer)                         \
{                                                                         \
}                                                                         \
void                                                                      \
_class::ParentChainChanged(nsIContent *aContent)                          \
{                                                                         \
}


#endif 
