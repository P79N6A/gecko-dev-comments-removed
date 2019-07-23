






































#ifndef __AECLASSDISPATCHER__
#define __AECLASSDISPATCHER__


#include "patricia.h"




class AEGenericClass;

class AEDispatchHandler
{
public:
						AEDispatchHandler(DescType handlerClass, AEGenericClass* handler, Boolean deleteOnRemove = true);
						~AEDispatchHandler();
						
						
	void					DispatchEvent(					AEDesc *			token,
													const AppleEvent *	appleEvent,
													AppleEvent *		reply);

	void					GetProperty(					DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass,
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken);
													
	void					GetDataFromListOrObject(			const AEDesc *		tokenOrTokenList,
													AEDesc *			desiredTypes,
													AEDesc *			data);

	void					GetItemFromContainer(			DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass, 
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken);

	void					CompareObjects(				DescType			comparisonOperator,
													const AEDesc *		object,
													const AEDesc *		descriptorOrObject,
													Boolean *			result);


	void					CountObjects(					DescType 		 	desiredType,
													DescType 		 	containerClass,
													const AEDesc *		container,
									   				long *			result);

	void					CreateSelfSpecifier(				const AEDesc *		token,
													AEDesc *			outSpecifier);
													
protected:
	
	Boolean				mDeleteHandler;
	DescType				mHandlerClass;
	AEGenericClass*		mHandler;
};



class AEDispatchTree
{
public:
						AEDispatchTree();
						~AEDispatchTree();
						
	void					InsertHandler(DescType handlerClass, AEGenericClass *handler, Boolean isDuplicate = false);
	AEDispatchHandler*		FindHandler(DescType handlerClass);
	
protected:

	enum {
		kDuplicateKeyError = 750
	};
	
	static int 				FreeDispatchTreeNodeData(void *nodeData, unsigned char *key, void *refCon);
	static int 				ReplaceDispatchTreeNode(void *nodeData, unsigned char *key, void *replaceData);
	
	
protected:

	PatriciaTreeRef		mTree;
};



#endif

