




#ifndef nsContentCreatorFunctions_h__
#define nsContentCreatorFunctions_h__

#include "nsError.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/FromParser.h"






class nsAString;
class nsIContent;
class nsINodeInfo;
class imgRequestProxy;
class nsNodeInfoManager;
class nsGenericHTMLElement;

nsresult
NS_NewElement(nsIContent** aResult,
              already_AddRefed<nsINodeInfo> aNodeInfo,
              mozilla::dom::FromParser aFromParser);

nsresult
NS_NewXMLElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo);

nsresult
NS_NewHTMLElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser);



already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(uint32_t aNodeType, already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser);

nsresult
NS_NewMathMLElement(nsIContent** aResult,
                     already_AddRefed<nsINodeInfo> aNodeInfo);

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
                         imgRequestProxy* aImageRequest);

#endif 
