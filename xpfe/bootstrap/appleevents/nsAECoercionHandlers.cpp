







































#include "nsAEUtils.h"

#include "nsAECoercionHandlers.h"



AECoercionHandlers*	AECoercionHandlers::sAECoercionHandlers = nil;







AECoercionHandlers::AECoercionHandlers()
:	mTextDescToPascalString(nil)
,	mPascalStringDescToText(nil)
{
	OSErr	err;
	
	
	mTextDescToPascalString = NewAECoerceDescUPP(TextToPascalStringCoercion);
	ThrowIfNil(mTextDescToPascalString);

	err = ::AEInstallCoercionHandler(typeChar, typePascalString,
								(AECoercionHandlerUPP) mTextDescToPascalString,
								(long)this,
								true,			
								false );		
	ThrowIfOSErr(err);

	mPascalStringDescToText = NewAECoerceDescUPP(PascalStringToTextCoercion);
	ThrowIfNil(mPascalStringDescToText);

	err = ::AEInstallCoercionHandler(typePascalString, typeChar,
								(AECoercionHandlerUPP) mPascalStringDescToText,
								(long)this,
								true,			
								false );		
	ThrowIfOSErr(err);
}








AECoercionHandlers::~AECoercionHandlers()
{
	if (mTextDescToPascalString)
	{
		AERemoveCoercionHandler(typeChar, typePascalString, (AECoercionHandlerUPP) mTextDescToPascalString, false);
		DisposeAECoerceDescUPP(mTextDescToPascalString);
	}

	if (mPascalStringDescToText)
	{
		AERemoveCoercionHandler(typePascalString, typeChar, (AECoercionHandlerUPP) mPascalStringDescToText, false);
		DisposeAECoerceDescUPP(mPascalStringDescToText);
	}
}


#pragma mark -





pascal OSErr AECoercionHandlers::TextToPascalStringCoercion(const AEDesc *fromDesc, DescType toType, long handlerRefcon, AEDesc *toDesc)
{
	OSErr	err = noErr;
	
	if (toType != typePascalString)
		return errAECoercionFail;

	switch (fromDesc->descriptorType)
	{
		case typeChar:
			Str255		pString;
			DescToPString(fromDesc, pString, 255);
			err = AECreateDesc(typePascalString, pString, pString[0] + 1, toDesc);
			break;
			
		default:
			err = errAECoercionFail;
			break;
	}

	return err;
}






pascal OSErr AECoercionHandlers::PascalStringToTextCoercion(const AEDesc *fromDesc, DescType toType, long handlerRefcon, AEDesc *toDesc)
{
	OSErr	err = noErr;
	
	if (toType != typeChar)
		return errAECoercionFail;

	switch (fromDesc->descriptorType)
	{
		case typePascalString:
			{
				long stringLen = AEGetDescDataSize(fromDesc);
				if (stringLen > 255)
				{
					err = errAECoercionFail;
					break;
				}
				Str255 str;
				AEGetDescData(fromDesc, str, sizeof(str) - 1);
				err = AECreateDesc(typeChar, str + 1, str[0], toDesc);
			}
			break;
			
		default:
			err = errAECoercionFail;
			break;
	}

	return err;
}



#pragma mark -






OSErr CreateCoercionHandlers()
{
	OSErr	err = noErr;
	
	if (AECoercionHandlers::sAECoercionHandlers)
		return noErr;
	
	try
	{
		AECoercionHandlers::sAECoercionHandlers = new AECoercionHandlers;
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch( ... )
	{
		err = paramErr;
	}
	
	return err;

}








OSErr ShutdownCoercionHandlers()
{
	if (!AECoercionHandlers::sAECoercionHandlers)
		return noErr;
	
	try
	{
		delete AECoercionHandlers::sAECoercionHandlers;
	}
	catch(...)
	{
	}
	
	AECoercionHandlers::sAECoercionHandlers = nil;
	return noErr;
}

