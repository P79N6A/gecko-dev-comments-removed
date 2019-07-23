






































#ifndef __AEWORDCLASS__
#define __AEWORDCLASS__

#include "AEGenericClass.h"

class AEWordClass : public AEGenericClass
{
	friend class AECoreClass;

private:
	typedef AEGenericClass	Inherited;

public:
						AEWordClass();
						~AEWordClass();


	virtual void			GetItemFromContainer(	DescType			desiredClass,
											const AEDesc*		containerToken,
											DescType			containerClass, 
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken);

	virtual void 			CountObjects(			DescType 		 	desiredType,
											DescType 		 	containerClass,
											const AEDesc *		container,
							   				long *			result);

	
	
	

	virtual void			CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier);

	virtual void			GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data);
	virtual void			SetDataForObject(const AEDesc *token, AEDesc *data);

	virtual Boolean			CanSetProperty(DescType propertyCode);
	virtual Boolean			CanGetProperty(DescType propertyCode);

};


class AEWordIterator : public AEUnnamedClassIterator
{
public:					AEWordIterator()
						: AEUnnamedClassIterator(cWord)
						{
						}

	virtual UInt32			GetNumItems(const AEDesc* containerToken);
	virtual ItemRef			GetIndexedItemReference(const AEDesc* containerToken, TAEListIndex itemIndex);
	
	virtual TAEListIndex		GetIndexFromItemID(const AEDesc* containerToken, ItemID itemID);

	virtual ItemID			GetIndexedItemID(const AEDesc* containerToken, TAEListIndex itemIndex);

	
	virtual ItemID			GetIDFromReference(const AEDesc* containerToken, ItemRef itemRef);
	virtual ItemRef			GetReferenceFromID(const AEDesc* containerToken, ItemID itemID);

	virtual ItemID			GetItemIDFromToken(const AEDesc* token);
	virtual void			SetItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* tokenRecord, ItemID itemID);

};


#endif 

