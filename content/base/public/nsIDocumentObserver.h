



































#ifndef nsIDocumentObserver_h___
#define nsIDocumentObserver_h___

#include "nsISupports.h"
#include "nsIMutationObserver.h"

class nsIAtom;
class nsIContent;
class nsIPresShell;
class nsIStyleSheet;
class nsIStyleRule;
class nsString;
class nsIDocument;

#define NS_IDOCUMENT_OBSERVER_IID \
{ 0x4e14e321, 0xa4bb, 0x49f8, \
  { 0xa5, 0x7a, 0x23, 0x63, 0x66, 0x8d, 0x14, 0xd0 } }

typedef PRUint32 nsUpdateType;

#define UPDATE_CONTENT_MODEL 0x00000001
#define UPDATE_STYLE         0x00000002
#define UPDATE_CONTENT_STATE 0x00000004
#define UPDATE_ALL (UPDATE_CONTENT_MODEL | UPDATE_STYLE | UPDATE_CONTENT_STATE)


class nsIDocumentObserver : public nsIMutationObserver
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_OBSERVER_IID)

  



  virtual void BeginUpdate(nsIDocument *aDocument,
                           nsUpdateType aUpdateType) = 0;

  



  virtual void EndUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType) = 0;

  


  virtual void BeginLoad(nsIDocument *aDocument) = 0;

  




  virtual void EndLoad(nsIDocument *aDocument) = 0;

  






















  virtual void ContentStatesChanged(nsIDocument* aDocument,
                                    nsIContent* aContent1,
                                    nsIContent* aContent2,
                                    PRInt32 aStateMask) = 0;

  










  virtual void StyleSheetAdded(nsIDocument *aDocument,
                               nsIStyleSheet* aStyleSheet,
                               PRBool aDocumentSheet) = 0;

  










  virtual void StyleSheetRemoved(nsIDocument *aDocument,
                                 nsIStyleSheet* aStyleSheet,
                                 PRBool aDocumentSheet) = 0;
  
  











  virtual void StyleSheetApplicableStateChanged(nsIDocument *aDocument,
                                                nsIStyleSheet* aStyleSheet,
                                                PRBool aApplicable) = 0;

  






















  virtual void StyleRuleChanged(nsIDocument *aDocument,
                                nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule) = 0;

  










  virtual void StyleRuleAdded(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) = 0;

  










  virtual void StyleRuleRemoved(nsIDocument *aDocument,
                                nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentObserver, NS_IDOCUMENT_OBSERVER_IID)

#define NS_DECL_NSIDOCUMENTOBSERVER                                          \
    virtual void BeginUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType);\
    virtual void EndUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType);\
    virtual void BeginLoad(nsIDocument* aDocument);                          \
    virtual void EndLoad(nsIDocument* aDocument);                            \
    virtual void ContentStatesChanged(nsIDocument* aDocument,                \
                                      nsIContent* aContent1,                 \
                                      nsIContent* aContent2,                 \
                                      PRInt32 aStateMask);                   \
    virtual void StyleSheetAdded(nsIDocument* aDocument,                     \
                                 nsIStyleSheet* aStyleSheet,                 \
                                 PRBool aDocumentSheet);                     \
    virtual void StyleSheetRemoved(nsIDocument* aDocument,                   \
                                   nsIStyleSheet* aStyleSheet,               \
                                   PRBool aDocumentSheet);                   \
    virtual void StyleSheetApplicableStateChanged(nsIDocument* aDocument,    \
                                                  nsIStyleSheet* aStyleSheet,\
                                                  PRBool aApplicable);       \
    virtual void StyleRuleChanged(nsIDocument* aDocument,                    \
                                  nsIStyleSheet* aStyleSheet,                \
                                  nsIStyleRule* aOldStyleRule,               \
                                  nsIStyleRule* aNewStyleRule);              \
    virtual void StyleRuleAdded(nsIDocument* aDocument,                      \
                                nsIStyleSheet* aStyleSheet,                  \
                                nsIStyleRule* aStyleRule);                   \
    virtual void StyleRuleRemoved(nsIDocument* aDocument,                    \
                                  nsIStyleSheet* aStyleSheet,                \
                                  nsIStyleRule* aStyleRule);                 \
    NS_DECL_NSIMUTATIONOBSERVER


#define NS_IMPL_NSIDOCUMENTOBSERVER_CORE_STUB(_class)                     \
void                                                                      \
_class::BeginUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType)     \
{                                                                         \
}                                                                         \
void                                                                      \
_class::EndUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType)       \
{                                                                         \
}                                                                         \
NS_IMPL_NSIMUTATIONOBSERVER_CORE_STUB(_class)

#define NS_IMPL_NSIDOCUMENTOBSERVER_LOAD_STUB(_class)                     \
void                                                                      \
_class::BeginLoad(nsIDocument* aDocument)                                 \
{                                                                         \
}                                                                         \
void                                                                      \
_class::EndLoad(nsIDocument* aDocument)                                   \
{                                                                         \
}

#define NS_IMPL_NSIDOCUMENTOBSERVER_STATE_STUB(_class)                    \
void                                                                      \
_class::ContentStatesChanged(nsIDocument* aDocument,                      \
                             nsIContent* aContent1,                       \
                             nsIContent* aContent2,                       \
                             PRInt32 aStateMask)                          \
{                                                                         \
}

#define NS_IMPL_NSIDOCUMENTOBSERVER_CONTENT(_class)                       \
NS_IMPL_NSIMUTATIONOBSERVER_CONTENT(_class)

#define NS_IMPL_NSIDOCUMENTOBSERVER_STYLE_STUB(_class)                    \
void                                                                      \
_class::StyleSheetAdded(nsIDocument* aDocument,                           \
                        nsIStyleSheet* aStyleSheet,                       \
                        PRBool aDocumentSheet)                            \
{                                                                         \
}                                                                         \
void                                                                      \
_class::StyleSheetRemoved(nsIDocument* aDocument,                         \
                          nsIStyleSheet* aStyleSheet,                     \
                          PRBool aDocumentSheet)                          \
{                                                                         \
}                                                                         \
void                                                                      \
_class::StyleSheetApplicableStateChanged(nsIDocument* aDocument,          \
                                         nsIStyleSheet* aStyleSheet,      \
                                         PRBool aApplicable)              \
{                                                                         \
}                                                                         \
void                                                                      \
_class::StyleRuleChanged(nsIDocument* aDocument,                          \
                         nsIStyleSheet* aStyleSheet,                      \
                         nsIStyleRule* aOldStyleRule,                     \
                         nsIStyleRule* aNewStyleRule)                     \
{                                                                         \
}                                                                         \
void                                                                      \
_class::StyleRuleAdded(nsIDocument* aDocument,                            \
                       nsIStyleSheet* aStyleSheet,                        \
                       nsIStyleRule* aStyleRule)                          \
{                                                                         \
}                                                                         \
void                                                                      \
_class::StyleRuleRemoved(nsIDocument* aDocument,                          \
                         nsIStyleSheet* aStyleSheet,                      \
                         nsIStyleRule* aStyleRule)                        \
{                                                                         \
}

#endif 
