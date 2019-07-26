




#ifndef nsContentListDeclarations_h
#define nsContentListDeclarations_h

#include "mozilla/StandardInteger.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"

class nsContentList;
class nsIAtom;
class nsIContent;
class nsINode;



#define kNameSpaceID_Wildcard INT32_MIN





typedef bool (*nsContentListMatchFunc)(nsIContent* aContent,
                                       int32_t aNamespaceID,
                                       nsIAtom* aAtom,
                                       void* aData);

typedef void (*nsContentListDestroyFunc)(void* aData);






typedef void* (*nsFuncStringContentListDataAllocator)(nsINode* aRootNode,
                                                      const nsString* aString);






already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode,
                  int32_t aMatchNameSpaceId,
                  const nsAString& aTagname);

already_AddRefed<nsContentList>
NS_GetFuncStringNodeList(nsINode* aRootNode,
                         nsContentListMatchFunc aFunc,
                         nsContentListDestroyFunc aDestroyFunc,
                         nsFuncStringContentListDataAllocator aDataAllocator,
                         const nsAString& aString);
already_AddRefed<nsContentList>
NS_GetFuncStringHTMLCollection(nsINode* aRootNode,
                               nsContentListMatchFunc aFunc,
                               nsContentListDestroyFunc aDestroyFunc,
                               nsFuncStringContentListDataAllocator aDataAllocator,
                               const nsAString& aString);

#endif 
