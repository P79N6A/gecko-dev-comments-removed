




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

namespace mozilla {
namespace dom {
class Element;
} 
} 

nsresult
NS_NewElement(mozilla::dom::Element** aResult,
              already_AddRefed<nsINodeInfo>&& aNodeInfo,
              mozilla::dom::FromParser aFromParser);

nsresult
NS_NewXMLElement(mozilla::dom::Element** aResult,
                 already_AddRefed<nsINodeInfo>&& aNodeInfo);

nsresult
NS_NewHTMLElement(mozilla::dom::Element** aResult,
                  already_AddRefed<nsINodeInfo>&& aNodeInfo,
                  mozilla::dom::FromParser aFromParser);



already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(uint32_t aNodeType,
                  already_AddRefed<nsINodeInfo>&& aNodeInfo,
                  mozilla::dom::FromParser aFromParser);

nsresult
NS_NewMathMLElement(mozilla::dom::Element** aResult,
                    already_AddRefed<nsINodeInfo>&& aNodeInfo);

#ifdef MOZ_XUL
nsresult
NS_NewXULElement(mozilla::dom::Element** aResult,
                 already_AddRefed<nsINodeInfo>&& aNodeInfo);

void
NS_TrustedNewXULElement(nsIContent** aResult,
                        already_AddRefed<nsINodeInfo>&& aNodeInfo);
#endif

nsresult
NS_NewSVGElement(mozilla::dom::Element** aResult,
                 already_AddRefed<nsINodeInfo>&& aNodeInfo,
                 mozilla::dom::FromParser aFromParser);

nsresult
NS_NewGenConImageContent(nsIContent** aResult,
                         already_AddRefed<nsINodeInfo>&& aNodeInfo,
                         imgRequestProxy* aImageRequest);

#endif 
