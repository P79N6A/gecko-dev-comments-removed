






































#include "nsAEUtils.h"

#include "nsAEGenericClass.h"
#include "nsAEClassDispatcher.h"








AEDispatchHandler::AEDispatchHandler(DescType handlerClass, AEGenericClass* handler, Boolean deleteOnRemove  )
:	mDeleteHandler(deleteOnRemove)
,	mHandlerClass(handlerClass)
,	mHandler(handler)
{
	AE_ASSERT(mHandler, "No handler");
}






AEDispatchHandler::~AEDispatchHandler()
{
	if (mDeleteHandler)
		delete mHandler;
}






void AEDispatchHandler::DispatchEvent(				AEDesc *			token,
											const AppleEvent *	appleEvent,
											AppleEvent *		reply)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->DispatchEvent(token, appleEvent, reply);
}





void AEDispatchHandler::GetProperty(				DescType			desiredClass,
											const AEDesc*		containerToken,
											DescType			containerClass,
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->GetProperty(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
}





void	AEDispatchHandler::GetDataFromListOrObject(		const AEDesc *		tokenOrTokenList,
											AEDesc *			desiredTypes,
											AEDesc *			data)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->GetDataFromListOrObject(tokenOrTokenList, desiredTypes, data);
}






void	AEDispatchHandler::GetItemFromContainer(			DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass, 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->GetItemFromContainer(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
}






void AEDispatchHandler::CompareObjects(					DescType			comparisonOperator,
												const AEDesc *		object,
												const AEDesc *		descriptorOrObject,
												Boolean *			result)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->CompareObjects(comparisonOperator, object, descriptorOrObject, result);
}







void AEDispatchHandler::CountObjects(					DescType 		 	desiredType,
												DescType 		 	containerClass,
												const AEDesc *		container,
								   				long *			result)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->CountObjects(desiredType, containerClass, container, result);
}






void AEDispatchHandler::CreateSelfSpecifier(				const AEDesc *		token,
												AEDesc *			outSpecifier)
{
	AE_ASSERT(mHandler, "No handler");
	mHandler->CreateSelfSpecifier(token, outSpecifier);
}

#pragma mark -






AEDispatchTree::AEDispatchTree()
:	mTree(nil)
{
	
	mTree = PatriciaInitTree(8 * sizeof(DescType));
	ThrowIfNil(mTree);
}






AEDispatchTree::~AEDispatchTree()
{
	if (mTree)
		PatriciaFreeTree(mTree, FreeDispatchTreeNodeData, this);
}






void AEDispatchTree::InsertHandler(DescType handlerClass, AEGenericClass *handler, Boolean isDuplicate )
{
	AEDispatchHandler	*newHandler = new AEDispatchHandler(handlerClass, handler, !isDuplicate);
	unsigned char		key[5] = {0};				
	int				result;
	
	*(DescType *)key = handlerClass;
	
	result = PatriciaInsert(mTree, nil, key, newHandler, nil);
	if (result == kDuplicateKeyError || result == 1)
	{
		ThrowIfOSErr(kDuplicateKeyError);
	}
	else if (result != 0)
	{
		ThrowIfOSErr(result);
	}
}







AEDispatchHandler* AEDispatchTree::FindHandler(DescType handlerClass)
{
	AEDispatchHandler*	foundClass = nil;
	unsigned char		key[5] = {0};				
	
	*(DescType *)key = handlerClass;

	(void)PatriciaSearch(mTree, key, (void**)&foundClass);
	
	return foundClass;
}










int AEDispatchTree::ReplaceDispatchTreeNode(void *nodeData, unsigned char *key, void *replaceData)
{
	return kDuplicateKeyError;
}






int AEDispatchTree::FreeDispatchTreeNodeData(void *nodeData, unsigned char *key, void *refCon)
{
	AEDispatchTree*	dispatchTree = reinterpret_cast<AEDispatchTree *>(refCon);
	AEDispatchHandler*	handler = reinterpret_cast<AEDispatchHandler *>(nodeData);
	
	delete handler;
	return 0;
}

