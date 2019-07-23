





































#include "nsAEUtils.h"
#include "nsAETokens.h"
#include "nsAECoreClass.h"
#include "nsAEApplicationClass.h"

#include "nsAEDocumentClass.h"






AEDocumentClass::AEDocumentClass()
:	AEGenericClass(cDocument, typeNull)
{
}






AEDocumentClass::~AEDocumentClass()
{
}

#pragma mark -





pascal OSErr AEDocumentClass::PropertyAccessor(		DescType			desiredClass,
											const AEDesc*		containerToken,
											DescType			containerClass,
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken,
											long 				refCon)
{
	AEDocumentClass*	docClass = reinterpret_cast<AEDocumentClass *>(refCon);
	if (!docClass) return paramErr;
	
	OSErr		err = noErr;
	
	try
	{
		docClass->GetProperty(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch(...)
	{
		err = paramErr;
	}
	
	return err;
}






void AEDocumentClass::GetItemFromContainer(		DescType			desiredClass,
										const AEDesc*		containerToken,
										DescType			containerClass, 
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)
{
	ThrowIfOSErr(errAEEventNotHandled);
}





void AEDocumentClass::GetDocumentFromApp(		DescType			desiredClass,		
										const AEDesc*		containerToken,	
										DescType			containerClass,  	 
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)		
{
	OSErr				err			= noErr;
	DescType				keyDataType 	= keyData->descriptorType;
	long					index;
	long					numItems;
	Boolean				wantsAllItems	= false;
	StAEDesc 				startObject;	
	StAEDesc 				stopObject;
	DocumentReference		document;
	Str63				documentName;
	CoreTokenRecord 		token;
	
	numItems = CountDocuments();

	switch (keyForm) 
	{
		case formName:											
			{
				if (DescToPString(keyData, documentName, 63) != noErr)
				{
					err = errAECoercionFail;
				}
				else
				{
					document = GetDocumentByName(documentName);
					if (document == nil)
						err = errAENoSuchObject;
				}
			}
			break;
		
		case formAbsolutePosition:									
			err = NormalizeAbsoluteIndex(keyData, &index, numItems, &wantsAllItems);
			if ((err == noErr) && (wantsAllItems == false))
			{
				document = GetDocumentByIndex(index);
				if (document == nil)
					err = errAEIllegalIndex;
			}
			break;	
		
		case formRelativePosition:
			ProcessFormRelativePostition(containerToken, keyData, &document);
			break;	
		
		case formRange:
			switch (keyDataType)
			{		
				case typeRangeDescriptor:
					err = ProcessFormRange((AEDesc *)keyData, &startObject, &stopObject);
					if (err == noErr)
					{
						ConstAETokenDesc	startTokenDesc(&startObject);
						ConstAETokenDesc	stopTokenDesc(&startObject);
						
						DescType startType = startTokenDesc.GetDispatchClass();
						DescType stopType  = stopTokenDesc.GetDispatchClass();
	 
						if (startType != cDocument || stopType != cDocument)
							err = errAEWrongDataType;
					}
					break;

				default:
					err = errAEWrongDataType;
					break;	
			}
			break;	

		default:
			err = errAEEventNotHandled;
			break;
	}
	
	
	

	if (wantsAllItems && (err == errAENoSuchObject || err == errAEIllegalIndex))
	{
		err = AECreateList(NULL, 0, false, (AEDescList*)resultToken);
		ThrowIfOSErr(err);
		return;
	}

	ThrowIfOSErr(err);
	
	

	token.dispatchClass  	= GetClass();
	token.objectClass	= GetClass();
	token.propertyCode 	= typeNull;

	if (wantsAllItems)
	{						
		err = AECreateList(NULL, 0, false, (AEDescList*)resultToken);
		
		if (err == noErr)
		{
			for (index = 1; index <= numItems; index++)
			{
				document = GetDocumentByIndex(index);
				ThrowIfOSErr(errAEEventNotHandled);
				
				token.documentID = GetDocumentID(document);
				
				err = AEPutPtr(resultToken, 0, desiredClass, &token, sizeof(token));
				ThrowIfOSErr(err);
			}
		}
	}
	else if (keyForm == formRange)
	{			
		DocumentReference	beginDocument;
		DocumentReference	endDocument;
		long				beginIndex;
		long				endIndex;
		
		beginDocument = GetDocumentReferenceFromToken(&startObject);
		beginIndex = GetDocumentIndex(beginDocument);

		endDocument = GetDocumentReferenceFromToken(&stopObject);
		endIndex = GetDocumentIndex(endDocument);
								
		err = AECreateList(NULL, 0, false, (AEDescList*)resultToken);
		ThrowIfOSErr(err);
			
		if (beginIndex > endIndex) 
		{
			DocumentReference temp;
			temp			= beginDocument;
			beginDocument	= endDocument;
			endDocument	= temp;
		}
		
		document = beginDocument;
		while (document != nil)
		{
			token.documentID = GetDocumentID(document);
			
			err = AEPutPtr(resultToken, 0, desiredClass, &token, sizeof(token));
			ThrowIfOSErr(err);

			if (document == endDocument)
				break;
			document = GetNextDocument(document);
		}
	}
	else
	{
		token.documentID = GetDocumentID(document);
		err = AECreateDesc(desiredClass, &token, sizeof(token), resultToken);
	}

	ThrowIfOSErr(err);		
}







pascal OSErr AEDocumentClass::DocumentAccessor(		DescType			desiredClass,		
											const AEDesc*		containerToken,	
											DescType			containerClass,  	 
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken,		
											long 				refCon)
{
	AEDocumentClass*		docClass = reinterpret_cast<AEDocumentClass *>(refCon);
	if (!docClass) return paramErr;
	
	OSErr		err = noErr;
	
	try
	{
		docClass->GetDocumentFromApp(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch(...)
	{
		err = paramErr;
	}
	
	return err;
}

											


#pragma mark -





void AEDocumentClass::ProcessFormRelativePostition(const AEDesc* anchorToken, const AEDesc *keyData, DocumentReference *document)
{
	ConstAETokenDesc		tokenDesc(anchorToken);
	OSErr			err = noErr;
	DescType 			positionEnum;
	DocumentReference	anchorDocument;
	DocumentReference	relativeDocument = nil;
	
	*document = nil;
	
	anchorDocument = GetDocumentReferenceFromToken(anchorToken);

	if (err == noErr)
	{
	
		switch (keyData->descriptorType)
		{
		   case typeEnumerated:
				if (DescToDescType((AEDesc*)keyData, &positionEnum) != noErr)
				{
					err = errAECoercionFail;
				}
				else
				{
					switch (positionEnum)
					{
						case kAENext:						
							*document = GetPreviousDocument(anchorDocument);
							if (*document == nil)
								err = errAENoSuchObject;
							break;
							
						case kAEPrevious:					
							*document = GetNextDocument(anchorDocument);
							if (*document == nil)
								err = errAENoSuchObject;
							break;
							
						default:
							err = errAEEventNotHandled;
							break;
					}
				}
				break;
				
			default:
				err = errAECoercionFail;
				break;
		}
	}
	
	ThrowIfOSErr(err);
}

#pragma mark -





Boolean AEDocumentClass::CanGetProperty(DescType property)
{
	Boolean	result = false;
	
	switch (property)
	{
		
		
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
		
		case pName:
		case pProperties:
		case pIsModified:
			result = true;
			break;
			
		
		default:
			result = Inherited::CanGetProperty(property);
			break;
	}

	return result;
}






Boolean AEDocumentClass::CanSetProperty(DescType property)
{
	Boolean	result = false;
	
	switch (property)
	{
		
		
		case pName:
			result = true;
			break;
			
		

		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
		
		case pProperties:
		case pIsModified:
			result = false;
			break;
			
		default:
			result = Inherited::CanSetProperty(property);
			break;
	}

	return result;
}


#pragma mark -






void AEDocumentClass::GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data)
{
	OSErr			err				= noErr;
	Boolean			usePropertyCode	= false;	
	DocumentReference	document 			= nil;
	ConstAETokenDesc		tokenDesc(token);
	DescType			aType			= cDocument;
	Str63 			documentName;
	Boolean 			isModified;
	
	
	document = GetDocumentReferenceFromToken(token);
	ThrowIfOSErr(err);
	if (document == nil)
		ThrowIfOSErr(paramErr);

	GetDocumentName(document, documentName);						
	isModified = DocumentIsModified(document);						

	usePropertyCode = tokenDesc.UsePropertyCode();
	
	DescType propertyCode = tokenDesc.GetPropertyCode();
	
	switch (propertyCode)
	{
		case pProperties:
			err = AECreateList(NULL, 0L, true, data);
			ThrowIfOSErr(err);

			err = AEPutKeyPtr(data, pObjectType,	typeType, 	&aType, 		 	sizeof(DescType));
			err = AEPutKeyPtr(data, pName, 		typeChar, 	&documentName[1], 	documentName[0]);
			err = AEPutKeyPtr(data, pIsModified, 	typeBoolean, 	&isModified, 		sizeof(Boolean));
			break;
			
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
			err = AECreateDesc(typeType, &aType, sizeof(DescType), data);
			break;
			
		case pName:
			err = AECreateDesc(typeChar, &documentName[1], documentName[0], data);
			break;
			
		case pIsModified:
			err = AECreateDesc(typeBoolean, &isModified, sizeof(Boolean), data);
			break;
			
		default:
			Inherited::GetDataFromObject(token, desiredTypes, data);
			break;
	}
	
	ThrowIfOSErr(err);
}





void AEDocumentClass::SetDataForObject(const AEDesc *token, AEDesc *data)
{
	OSErr				err = noErr;		
	Boolean				usePropertyCode;
	DescType				propertyCode;
	DocumentReference 		document = nil;
	ConstAETokenDesc			tokenDesc(token);
	StAEDesc 				propertyRecord;

	usePropertyCode = tokenDesc.UsePropertyCode();
	document = GetDocumentReferenceFromToken(token);
		
	if (usePropertyCode == false)
	{
		err = errAEWriteDenied;
	}
	else
	{
		propertyCode = tokenDesc.GetPropertyCode();
		
		if (data->descriptorType == typeAERecord)
		{		
			SetDocumentProperties(document, data);
		}
		else	
		{
			err = AECreateList(NULL, 0L, true, &propertyRecord);
			ThrowIfOSErr(err);
			
			err = AEPutKeyDesc(&propertyRecord, propertyCode, data);
			ThrowIfOSErr(err);
		
			SetDocumentProperties(document, &propertyRecord);
		}
	}

	ThrowIfOSErr(err);
}






void	AEDocumentClass::SetDocumentProperties(DocumentReference document, AEDesc *propertyRecord)
{

}






long AEDocumentClass::CountDocuments()
{
	return 0;
}






DocumentReference  AEDocumentClass::GetDocumentByName(ConstStr255Param docName)
{
	return nil;
}





DocumentReference  AEDocumentClass::GetDocumentByIndex(long index)
{
	return nil;
}





DocumentReference AEDocumentClass::GetDocumentByID(long docID)
{
	return nil;
}





DocumentReference AEDocumentClass::GetNextDocument(DocumentReference docRef)
{
	return nil;
}






DocumentReference AEDocumentClass::GetPreviousDocument(DocumentReference docRef)
{
	return nil;
}






DocumentReference AEDocumentClass::GetDocumentReferenceFromToken(const AEDesc *token)
{
	ConstAETokenDesc	tokenDesc(token);
	long			docID = tokenDesc.GetDocumentID();
	
	return GetDocumentByID(docID);
}





void AEDocumentClass::CloseWindowSaving(AEDesc *token, const AEDesc *saving, AEDesc *savingIn)
{
	OSErr			err			= noErr;
	DocumentReference	document;

	document = GetDocumentReferenceFromToken(token);
	
	if (document != nil)
	{
		
	}
	else
		err = errAEEventNotHandled;
	
	ThrowIfOSErr(err);
}

#pragma mark -





void AEDocumentClass::CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier)
{
	ThrowIfOSErr(errAENoSuchObject);
}

#pragma mark -





long GetDocumentID(DocumentReference docRef)
{
	return 0;
}

	




long GetDocumentIndex(DocumentReference docRef)
{
	return 0;
}





void	GetDocumentName(DocumentReference docRef, Str63 docName)
{
	docName[0] = 0;
}






Boolean DocumentIsModified(DocumentReference docRef)
{
	return false;
}

