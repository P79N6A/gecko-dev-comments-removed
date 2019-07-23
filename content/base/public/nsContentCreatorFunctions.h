






































#ifndef nsContentCreatorFunctions_h__
#define nsContentCreatorFunctions_h__

#include "nscore.h"
#include "nsCOMPtr.h"






class nsAString;
class nsIContent;
class nsIDocument;
class nsINodeInfo;
class imgIRequest;
class nsNodeInfoManager;
class nsGenericHTMLElement;

nsresult
NS_NewElement(nsIContent** aResult, PRInt32 aElementType,
              nsINodeInfo* aNodeInfo);

nsresult
NS_NewXMLElement(nsIContent** aResult, nsINodeInfo* aNodeInfo);




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
NS_NewHTMLElement(nsIContent** aResult, nsINodeInfo *aNodeInfo);



already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(PRUint32 aNodeType, nsINodeInfo *aNodeInfo,
                  PRBool aFromParser);

#ifdef MOZ_MATHML
nsresult
NS_NewMathMLElement(nsIContent** aResult, nsINodeInfo* aNodeInfo);
#endif

#ifdef MOZ_XUL
nsresult
NS_NewXULElement(nsIContent** aResult, nsINodeInfo* aNodeInfo);
#endif

#ifdef MOZ_SVG
nsresult
NS_NewSVGElement(nsIContent** aResult, nsINodeInfo* aNodeInfo);
#endif

nsresult
NS_NewGenConImageContent(nsIContent** aResult, nsINodeInfo* aNodeInfo,
                         imgIRequest* aImageRequest);

nsresult
NS_NewXMLEventsElement(nsIContent** aResult, nsINodeInfo* aNodeInfo);

#endif 
