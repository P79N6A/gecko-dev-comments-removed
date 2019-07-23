






































#ifndef __AEDOCUMENTCLASS__
#define __AEDOCUMENTCLASS__

#include "nsAEGenericClass.h"


typedef WindowPtr	DocumentReference;		


class AEDocumentClass : public AEGenericClass
{
	friend class AECoreClass;
	
private:
	typedef AEGenericClass	Inherited;
	
protected:
	
						AEDocumentClass();
						~AEDocumentClass();

	void					GetDocumentFromApp(		DescType			desiredClass,		
												const AEDesc*		containerToken,	
												DescType			containerClass,  	 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken);		

	virtual void			GetItemFromContainer(		DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass, 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken);

public:

	static pascal OSErr		DocumentAccessor(			DescType			desiredClass,		
												const AEDesc*		containerToken,	
												DescType			containerClass,  	
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken,		
												long 				refCon);

	static pascal OSErr		PropertyAccessor(			DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass,
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken,
												long 				refCon);

	void					ProcessFormRelativePostition(	const AEDesc* 		anchorToken,
												const AEDesc*		keyData,
												DocumentReference*	document);

	void					CloseWindowSaving(			AEDesc*			token,
												const AEDesc*		saving,
												AEDesc*			savingIn);
	
protected:

	
	
	
	virtual void			GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data);
	void					GetDataFromList(AEDesc *srcList, AEDesc *desiredTypes, AEDesc *dstList);

	virtual void			SetDataForObject(const AEDesc *token, AEDesc *data);
	void 					SetDataForList(const AEDesc *token, AEDesc *data);

	virtual void			CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier);

	virtual Boolean			CanSetProperty(DescType propertyCode);
	virtual Boolean			CanGetProperty(DescType propertyCode);


	void					SetDocumentProperties(DocumentReference document, AEDesc *propertyRecord);

	DocumentReference		GetDocumentReferenceFromToken(const AEDesc *token);		
	DocumentReference		GetDocumentByName(ConstStr255Param docName);
	DocumentReference		GetDocumentByIndex(long index);
	DocumentReference		GetDocumentByID(long docID);
	
	DocumentReference		GetNextDocument(DocumentReference docRef);
	DocumentReference		GetPreviousDocument(DocumentReference docRef);

public:
	static long				CountDocuments();

};


long		GetDocumentID(DocumentReference docRef);
long		GetDocumentIndex(DocumentReference docRef);
void		GetDocumentName(DocumentReference docRef, Str63 docName);
Boolean	DocumentIsModified(DocumentReference docRef);


#endif 
