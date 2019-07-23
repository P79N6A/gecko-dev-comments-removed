



































 
#ifndef nsIDocumentTransformer_h__
#define nsIDocumentTransformer_h__

#include "nsISupports.h"

class nsIDOMDocument;
class nsIDOMNode;
class nsILoadGroup;
class nsIURI;
class nsIPrincipal;
class nsString;

#define NS_ITRANSFORMOBSERVER_IID \
{ 0x04b2d17c, 0xe98d, 0x45f5, \
  { 0x9a, 0x67, 0xb7, 0x01, 0x19, 0x59, 0x7d, 0xe7 } }

class nsITransformObserver : public nsISupports
{
public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITRANSFORMOBSERVER_IID)

  NS_IMETHOD OnDocumentCreated(nsIDocument *aResultDocument) = 0;

  NS_IMETHOD OnTransformDone(nsresult aResult,
                             nsIDocument *aResultDocument) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITransformObserver, NS_ITRANSFORMOBSERVER_IID)

#define NS_IDOCUMENTTRANSFORMER_IID \
  {0x43e5a6c6, 0xa53c, 0x4f97, \
    { 0x91, 0x79, 0x47, 0xf2, 0x46, 0xec, 0xd9, 0xd6 }}

class nsIDocumentTransformer : public nsISupports
{
public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENTTRANSFORMER_IID)

  NS_IMETHOD SetTransformObserver(nsITransformObserver* aObserver) = 0;
  NS_IMETHOD LoadStyleSheet(nsIURI* aUri, nsILoadGroup* aLoadGroup,
                            nsIPrincipal* aCallerPrincipal) = 0;
  NS_IMETHOD SetSourceContentModel(nsIDOMNode* aSource) = 0;
  NS_IMETHOD CancelLoads() = 0;

  NS_IMETHOD AddXSLTParamNamespace(const nsString& aPrefix,
                                   const nsString& aNamespace) = 0;
  NS_IMETHOD AddXSLTParam(const nsString& aName,
                          const nsString& aNamespace,
                          const nsString& aValue,
                          const nsString& aSelect,
                          nsIDOMNode* aContextNode) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentTransformer,
                              NS_IDOCUMENTTRANSFORMER_IID)

#endif 
