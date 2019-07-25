






































#ifndef nsContentCreatorFunctions_h__
#define nsContentCreatorFunctions_h__

#include "nscore.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/FromParser.h"






class nsAString;
class nsIContent;
class nsIDocument;
class nsINodeInfo;
class imgIRequest;
class nsNodeInfoManager;
class nsGenericHTMLElement;

nsresult
NS_NewElement(nsIContent** aResult, PRInt32 aElementType,
              already_AddRefed<nsINodeInfo> aNodeInfo,
              mozilla::dom::FromParser aFromParser);

nsresult
NS_NewXMLElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo);




nsresult
NS_NewTextNode(nsIContent **aResult, nsNodeInfoManager *aNodeInfoManager);




nsresult
NS_NewCommentNode(nsIContent **aResult, nsNodeInfoManager *aNodeInfoManager);




nsresult
NS_NewXMLProcessingInstruction(nsIContent** aInstancePtrResult,
                               nsNodeInfoManager *aNodeInfoManager,
                               const nsAString& aTarget,
                               const nsAString& aData);




nsresult
NS_NewXMLStylesheetProcessingInstruction(nsIContent** aInstancePtrResult,
                                         nsNodeInfoManager *aNodeInfoManager,
                                         const nsAString& aData);




nsresult
NS_NewXMLCDATASection(nsIContent** aInstancePtrResult,
                      nsNodeInfoManager *aNodeInfoManager);

nsresult
NS_NewHTMLElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser);



already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(PRUint32 aNodeType, already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser);

#ifdef MOZ_MATHML
nsresult
NS_NewMathMLElement(nsIContent** aResult,
                     already_AddRefed<nsINodeInfo> aNodeInfo);
#endif

#ifdef MOZ_XUL
nsresult
NS_NewXULElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo);

void
NS_TrustedNewXULElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo);
#endif

nsresult
NS_NewSVGElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo,
                 mozilla::dom::FromParser aFromParser);

nsresult
NS_NewGenConImageContent(nsIContent** aResult,
                         already_AddRefed<nsINodeInfo> aNodeInfo,
                         imgIRequest* aImageRequest);

nsresult
NS_NewXMLEventsElement(nsIContent** aResult,
                       already_AddRefed<nsINodeInfo> aNodeInfo);

#endif 
