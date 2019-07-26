





#ifndef mozilla_a11y_sdnAccessible_h_
#define mozilla_a11y_sdnAccessible_h_

#include "ISimpleDOMNode.h"
#include "AccessibleWrap.h"

#include "mozilla/Attributes.h"

namespace mozilla {
namespace a11y {

class sdnAccessible MOZ_FINAL : public ISimpleDOMNode
{
public:
  sdnAccessible(nsINode* aNode) : mNode(aNode) { }
  ~sdnAccessible() { }

  


  bool IsDefunct() const { return !GetDocument(); }

  


  DocAccessible* GetDocument() const;

  


  Accessible* GetAccessible() const;

  
  DECL_IUNKNOWN

  virtual  HRESULT STDMETHODCALLTYPE get_nodeInfo(
     BSTR __RPC_FAR* aNodeName,
     short __RPC_FAR* aNameSpaceID,
     BSTR __RPC_FAR* aNodeValue,
     unsigned int __RPC_FAR* aNumChildren,
     unsigned int __RPC_FAR* aUniqueID,
     unsigned short __RPC_FAR* aNodeType);

  virtual  HRESULT STDMETHODCALLTYPE get_attributes(
     unsigned short aMaxAttribs,
     BSTR __RPC_FAR* aAttribNames,
     short __RPC_FAR* aNameSpaceIDs,
     BSTR __RPC_FAR* aAttribValues,
     unsigned short __RPC_FAR* aNumAttribs);

  virtual  HRESULT STDMETHODCALLTYPE get_attributesForNames(
     unsigned short aMaxAttribs,
     BSTR __RPC_FAR* aAttribNames,
     short __RPC_FAR* aNameSpaceID,
     BSTR __RPC_FAR* aAttribValues);

  virtual  HRESULT STDMETHODCALLTYPE get_computedStyle(
     unsigned short aMaxStyleProperties,
     boolean aUseAlternateView,
     BSTR __RPC_FAR* aStyleProperties,
     BSTR __RPC_FAR* aStyleValues,
     unsigned short __RPC_FAR* aNumStyleProperties);

  virtual  HRESULT STDMETHODCALLTYPE get_computedStyleForProperties(
     unsigned short aNumStyleProperties,
     boolean aUseAlternateView,
     BSTR __RPC_FAR* aStyleProperties,
     BSTR __RPC_FAR* aStyleValues);

  virtual HRESULT STDMETHODCALLTYPE scrollTo( boolean aScrollTopLeft);

  virtual  HRESULT STDMETHODCALLTYPE get_parentNode(
     ISimpleDOMNode __RPC_FAR *__RPC_FAR* aNode);

  virtual  HRESULT STDMETHODCALLTYPE get_firstChild(
     ISimpleDOMNode __RPC_FAR *__RPC_FAR* aNode);

  virtual  HRESULT STDMETHODCALLTYPE get_lastChild(
     ISimpleDOMNode __RPC_FAR *__RPC_FAR* aNode);

  virtual  HRESULT STDMETHODCALLTYPE get_previousSibling(
     ISimpleDOMNode __RPC_FAR *__RPC_FAR* aNode);

  virtual  HRESULT STDMETHODCALLTYPE get_nextSibling(
     ISimpleDOMNode __RPC_FAR *__RPC_FAR* aNode);

  virtual  HRESULT STDMETHODCALLTYPE get_childAt(
     unsigned aChildIndex,
     ISimpleDOMNode __RPC_FAR *__RPC_FAR* aNode);

  virtual  HRESULT STDMETHODCALLTYPE get_innerHTML(
     BSTR __RPC_FAR* aInnerHTML);

  virtual  HRESULT STDMETHODCALLTYPE get_localInterface(
     void __RPC_FAR *__RPC_FAR* aLocalInterface);

  virtual  HRESULT STDMETHODCALLTYPE get_language(
     BSTR __RPC_FAR* aLanguage);

private:
  nsCOMPtr<nsINode> mNode;
};

} 
} 

#endif 
