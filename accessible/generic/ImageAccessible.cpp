




#include "ImageAccessible.h"

#include "nsAccUtils.h"
#include "Role.h"
#include "AccIterator.h"
#include "States.h"

#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsGenericHTMLElement.h"
#include "nsIDocument.h"
#include "nsIImageLoadingContent.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIPersistentProperties2.h"
#include "nsPIDOMWindow.h"
#include "nsIURI.h"

using namespace mozilla::a11y;





ImageAccessible::
  ImageAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  LinkableAccessible(aContent, aDoc)
{
  mType = eImageType;
}

ImageAccessible::~ImageAccessible()
{
}




uint64_t
ImageAccessible::NativeState()
{
  
  

  uint64_t state = LinkableAccessible::NativeState();

  nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(mContent));
  nsCOMPtr<imgIRequest> imageRequest;

  if (content)
    content->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                        getter_AddRefs(imageRequest));

  nsCOMPtr<imgIContainer> imgContainer;
  if (imageRequest)
    imageRequest->GetImage(getter_AddRefs(imgContainer));

  if (imgContainer) {
    bool animated;
    imgContainer->GetAnimated(&animated);
    if (animated)
      state |= states::ANIMATED;
  }

  return state;
}

ENameValueFlag
ImageAccessible::NativeName(nsString& aName)
{
  bool hasAltAttrib =
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt, aName);
  if (!aName.IsEmpty())
    return eNameOK;

  ENameValueFlag nameFlag = Accessible::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  
  
  
  
  return hasAltAttrib ? eNoNameOnPurpose : eNameOK;
}

role
ImageAccessible::NativeRole()
{
  return roles::GRAPHIC;
}




uint8_t
ImageAccessible::ActionCount()
{
  uint8_t actionCount = LinkableAccessible::ActionCount();
  return HasLongDesc() ? actionCount + 1 : actionCount;
}

void
ImageAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();
  if (IsLongDescIndex(aIndex) && HasLongDesc())
    aName.AssignLiteral("showlongdesc"); 
  else
    LinkableAccessible::ActionNameAt(aIndex, aName);
}

bool
ImageAccessible::DoAction(uint8_t aIndex)
{
  
  if (!IsLongDescIndex(aIndex))
    return LinkableAccessible::DoAction(aIndex);

  nsCOMPtr<nsIURI> uri = GetLongDescURI();
  if (!uri)
    return false;

  nsAutoCString utf8spec;
  uri->GetSpec(utf8spec);
  NS_ConvertUTF8toUTF16 spec(utf8spec);

  nsIDocument* document = mContent->OwnerDoc();
  nsCOMPtr<nsPIDOMWindow> piWindow = document->GetWindow();
  nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(piWindow);
  if (!win)
    return false;

  nsCOMPtr<nsIDOMWindow> tmp;
  return NS_SUCCEEDED(win->Open(spec, EmptyString(), EmptyString(),
                                getter_AddRefs(tmp)));
}




nsIntPoint
ImageAccessible::Position(uint32_t aCoordType)
{
  nsIntRect rect = Bounds();
  nsAccUtils::ConvertScreenCoordsTo(&rect.x, &rect.y, aCoordType, this);
  return rect.TopLeft();
}

nsIntSize
ImageAccessible::Size()
{
  return Bounds().Size();
}


already_AddRefed<nsIPersistentProperties>
ImageAccessible::NativeAttributes()
{
  nsCOMPtr<nsIPersistentProperties> attributes =
    LinkableAccessible::NativeAttributes();

  nsAutoString src;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src);
  if (!src.IsEmpty())
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::src, src);

  return attributes.forget();
}




already_AddRefed<nsIURI>
ImageAccessible::GetLongDescURI() const
{
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::longdesc)) {
    
    nsAutoString longdesc;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::longdesc, longdesc);
    if (longdesc.FindChar(' ') != -1 || longdesc.FindChar('\t') != -1 ||
        longdesc.FindChar('\r') != -1 || longdesc.FindChar('\n') != -1) {
      return nullptr;
    }
    nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
    nsCOMPtr<nsIURI> uri;
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri), longdesc,
                                              mContent->OwnerDoc(), baseURI);
    return uri.forget();
  }

  DocAccessible* document = Document();
  if (document) {
    IDRefsIterator iter(document, mContent, nsGkAtoms::aria_describedby);
    while (nsIContent* target = iter.NextElem()) {
      if ((target->IsHTML(nsGkAtoms::a) || target->IsHTML(nsGkAtoms::area)) &&
          target->HasAttr(kNameSpaceID_None, nsGkAtoms::href)) {
        nsGenericHTMLElement* element =
          nsGenericHTMLElement::FromContent(target);

        nsCOMPtr<nsIURI> uri;
        element->GetURIAttr(nsGkAtoms::href, nullptr, getter_AddRefs(uri));
        return uri.forget();
      }
    }
  }

  return nullptr;
}

bool
ImageAccessible::IsLongDescIndex(uint8_t aIndex)
{
  return aIndex == LinkableAccessible::ActionCount();
}

