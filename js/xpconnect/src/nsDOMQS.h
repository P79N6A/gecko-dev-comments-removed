





#ifndef nsDOMQS_h__
#define nsDOMQS_h__

#include "nsDOMClassInfoID.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "mozilla/dom/HTMLImageElement.h"
#include "mozilla/dom/HTMLOptionElement.h"
#include "HTMLOptGroupElement.h"
#include "mozilla/dom/HTMLVideoElement.h"
#include "nsHTMLDocument.h"
#include "nsICSSDeclaration.h"
#include "nsSVGElement.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/UIEvent.h"
#include "mozilla/dom/MouseEvent.h"
#include "mozilla/dom/EventTargetBinding.h"
#include "mozilla/dom/NodeBinding.h"
#include "mozilla/dom/ElementBinding.h"
#include "mozilla/dom/HTMLElementBinding.h"
#include "mozilla/dom/DocumentBinding.h"
#include "mozilla/dom/SVGElementBinding.h"
#include "mozilla/dom/HTMLDocumentBinding.h"
#include "XPCQuickStubs.h"
#include "nsGlobalWindow.h"

inline nsISupports*
ToSupports(nsContentList *p)
{
    return static_cast<nsINodeList*>(p);
}

inline nsISupports*
ToCanonicalSupports(nsContentList *p)
{
    return static_cast<nsINodeList*>(p);
}

#endif 
