




#ifndef nsContentCreatorFunctions_h__
#define nsContentCreatorFunctions_h__

#include "nsError.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/FromParser.h"






class nsIContent;
class imgRequestProxy;
class nsGenericHTMLElement;

namespace mozilla {
namespace dom {
class Element;
class NodeInfo;
} 
} 

nsresult
NS_NewElement(mozilla::dom::Element** aResult,
              already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
              mozilla::dom::FromParser aFromParser);

nsresult
NS_NewXMLElement(mozilla::dom::Element** aResult,
                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

nsresult
NS_NewHTMLElement(mozilla::dom::Element** aResult,
                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                  mozilla::dom::FromParser aFromParser);



already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(uint32_t aNodeType,
                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                  mozilla::dom::FromParser aFromParser);

nsresult
NS_NewMathMLElement(mozilla::dom::Element** aResult,
                    already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

#ifdef MOZ_XUL
nsresult
NS_NewXULElement(mozilla::dom::Element** aResult,
                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

void
NS_TrustedNewXULElement(nsIContent** aResult,
                        already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);
#endif

nsresult
NS_NewSVGElement(mozilla::dom::Element** aResult,
                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                 mozilla::dom::FromParser aFromParser);

nsresult
NS_NewGenConImageContent(nsIContent** aResult,
                         already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                         imgRequestProxy* aImageRequest);

#endif 
