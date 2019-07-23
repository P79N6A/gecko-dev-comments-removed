







































#include <StringCompare.h>

#include "nsAEUtils.h"
#include "nsAECompare.h"



Boolean AEComparisons::CompareTexts(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	Boolean	result = false;
	
	short	compareResult;
	Handle  lhsHandle = 0, rhsHandle = 0;
	char  	*lhs;
	char  	*rhs;
	long		lhsSize;
	long		rhsSize;
	
	
	if (DescToTextHandle(desc1, &lhsHandle) != noErr || DescToTextHandle(desc2, &rhsHandle) != noErr)
	    goto fail;
	
	lhsSize = GetHandleSize(lhsHandle);
	HLock(lhsHandle);
	lhs = *(lhsHandle);
	
	rhsSize = GetHandleSize(rhsHandle);
	HLock(rhsHandle);
	rhs = *(rhsHandle);

	compareResult = ::CompareText(lhs, rhs, lhsSize, rhsSize, nil);

	switch (oper) 
	{
		case kAEEquals:
			result = (compareResult == 0);
			break;
		
		case kAELessThan:
			result = (compareResult < 0);
			break;
		
		case kAELessThanEquals:
			result = (compareResult <= 0);
			break;
		
		case kAEGreaterThan:
			result = (compareResult > 0);
			break;
		
		case kAEGreaterThanEquals:
			result = (compareResult >= 0);
			break;
		
		case kAEBeginsWith:
			if (rhsSize > lhsSize)
			{
				result = false;
			}
			else
			{
				
				
				compareResult = CompareText(lhs, rhs, rhsSize, rhsSize, nil);
				result = (compareResult == 0);
			}
			break;
			
		case kAEEndsWith:
			if (rhsSize > lhsSize)
			{
				result = false;
			}
			else
			{
				
				
				
				
				lhs += (lhsSize - rhsSize);
				compareResult = CompareText(lhs, rhs, rhsSize, rhsSize, nil);
				result = (compareResult == 0);
			}
			break;

		case kAEContains:
			
			
			
			
			result = false;
			while (lhsSize >= rhsSize)
			{
				compareResult = CompareText(lhs, rhs, rhsSize, rhsSize, nil);
				if (compareResult == 0)
				{
					result = true;
					break;
				}
				lhs++;
				lhsSize--;
			}
			break;

		default:
			ThrowOSErr(errAEBadTestKey);
	}

fail:
    if (lhsHandle) DisposeHandle(lhsHandle);
    if (rhsHandle) DisposeHandle(rhsHandle);
	
	return result;
}



Boolean AEComparisons::CompareEnumeration(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr		err	= noErr;
	Boolean		result = false;
	long	 		lhs;
	long   		rhs;
	StAEDesc 		charDesc;
	
	
	
	
	err = AECoerceDesc(desc1, typeChar, &charDesc);
	ThrowIfOSErr(err);

	lhs = **(long **)(charDesc.dataHandle);
	AEDisposeDesc(&charDesc);
	
	err = AECoerceDesc(desc2, typeChar, &charDesc);
	ThrowIfOSErr(err);

	rhs = **(long **)charDesc.dataHandle;
	AEDisposeDesc(&charDesc);
	
	switch (oper) 
	{
		case kAEEquals:
			result = (lhs == rhs);	
			break;
		
		default:
			ThrowOSErr(errAEBadTestKey);
	}

	return result;
}



Boolean AEComparisons::CompareInteger(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr		err	= noErr;
	Boolean		result = false;
	long	 		lhs;
	long   		rhs;
	StAEDesc 		longDesc;
	
	
	
	
	err = AECoerceDesc(desc1, typeLongInteger, &longDesc);
	ThrowIfOSErr(err);

	lhs = **(long **)(longDesc.dataHandle);
	AEDisposeDesc(&longDesc);
	
	err = AECoerceDesc(desc2, typeLongInteger, &longDesc);
	ThrowIfOSErr(err);

	rhs = **(long **)longDesc.dataHandle;
	AEDisposeDesc(&longDesc);
	
	switch (oper) 
	{
		case kAEEquals:
			result = (lhs == rhs);
			break;
		
		case kAELessThan:
			result = (lhs < rhs);
			break;
		
		case kAELessThanEquals:
			result = (lhs <= rhs);
			break;
		
		case kAEGreaterThan:
			result = (lhs > rhs);
			break;
		
		case kAEGreaterThanEquals:
			result = (lhs >= rhs);
			break;
		
		default:
			ThrowOSErr(errAEBadTestKey);
	}

	return result;
}



Boolean AEComparisons::CompareFixed(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr		err	= noErr;
	Boolean		result = false;
	Fixed		lhs;
	Fixed 		rhs;
	
	err = DescToFixed(desc1, &lhs);
	ThrowIfOSErr(err);
		
	err = DescToFixed(desc2, &rhs);
	ThrowIfOSErr(err);
	
	switch (oper) 
	{
		case kAEEquals:
			result = (lhs == rhs);
			break;
		
		case kAELessThan:
			result = (lhs < rhs);
			break;
		
		case kAELessThanEquals:
			result = (lhs <= rhs);
			break;
		
		case kAEGreaterThan:
			result = (lhs > rhs);
			break;
		
			case kAEGreaterThanEquals:
			result = (lhs >= rhs);
			break;
		
		default:
			ThrowOSErr(errAEBadTestKey);
	}
	
	return result;
}



Boolean AEComparisons::CompareFloat(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr	err	= noErr;
	Boolean	result = false;
	float		 lhs;
	float 	 rhs;
	
	err = DescToFloat(desc1, &lhs);
	ThrowIfOSErr(err);
		
	err = DescToFloat(desc2, &rhs);
	ThrowIfOSErr(err);
	
	switch (oper) 
	{
		case kAEEquals:
			result = (lhs == rhs);
			break;
		
		case kAELessThan:
			result = (lhs < rhs);
			break;
		
		case kAELessThanEquals:
			result = (lhs <= rhs);
			break;
		
		case kAEGreaterThan:
			result = (lhs > rhs);
			break;
		
			case kAEGreaterThanEquals:
			result = (lhs >= rhs);
			break;
		
		default:
			ThrowOSErr(errAEBadTestKey);
	}
	
	return result;
}




Boolean AEComparisons::CompareBoolean(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	Boolean	result = false;
	
	Boolean 	bool1	= ((**(char **)desc1->dataHandle) != 0);
	Boolean 	bool2	= ((**(char **)desc2->dataHandle) != 0);
		
	if (oper == kAEEquals) 
		result = (bool1 == bool2);
	else
		ThrowOSErr(errAEBadTestKey);		

	return result;
}



Boolean AEComparisons::CompareRGBColor(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr	err = noErr;
	Boolean	result = false;
	
	RGBColor lhs;
	RGBColor rhs;
	
	err = DescToRGBColor(desc1, &lhs);
	ThrowIfOSErr(err);
		
	err = DescToRGBColor(desc2, &rhs);
	ThrowIfOSErr(err);

	if (oper == kAEEquals) 
		result = EqualRGB(lhs, rhs);
	else
		ThrowOSErr(errAEBadTestKey);		

	return result;
}



Boolean AEComparisons::ComparePoint(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr	err = noErr;
	Boolean	result = false;
	
	Point		lhs;
	Point		rhs;
	
	err = DescToPoint(desc1, &lhs);
	ThrowIfOSErr(err);
		
	err = DescToPoint(desc2, &rhs);
	ThrowIfOSErr(err);
		
	switch (oper)
	{
		case kAEEquals:
			result = (lhs.h = rhs.h && lhs.v == rhs.v);
			break;
			
		case kAELessThan:
			result = (lhs.h < rhs.h && lhs.v < rhs.v);
			break;
		
		case kAELessThanEquals:
			result = (lhs.h <= rhs.h && lhs.v <= rhs.v);
			break;
		
		case kAEGreaterThan:
			result = (lhs.h > rhs.h && lhs.v > rhs.v);
			break;
		
		case kAEGreaterThanEquals:
			result = (lhs.h >= rhs.h && lhs.v >= rhs.v);
			break;
		
		default:
			ThrowOSErr(errAEBadTestKey);		
	}

	return result;
}



Boolean AEComparisons::CompareRect(DescType oper, const AEDesc *desc1, const AEDesc *desc2)
{
	OSErr	err = noErr;
	Boolean	result = false;
	Rect		lhs;
	Rect		rhs;
	
	err = DescToRect(desc1, &lhs);
	ThrowIfOSErr(err);
		
	err = DescToRect(desc2, &rhs);
	ThrowIfOSErr(err);
		
	switch (oper)
	{
		
		case kAEEquals:	
			result = ((lhs.top == rhs.top) && (lhs.left == rhs.left) && (lhs.bottom == rhs.bottom) && (lhs.right == rhs.right));
			break;
			
		
		case kAELessThan:	
			result = (((lhs.bottom - lhs.top) < (rhs.bottom - rhs.top)) && ((lhs.right - lhs.left) < (lhs.right - rhs.left)));
			break;
		
		case kAELessThanEquals:
			result = (((lhs.bottom - lhs.top) < (rhs.bottom - rhs.top)) && ((lhs.right - lhs.left) < (lhs.right - rhs.left)));
			break;
		
		case kAEGreaterThan:
			result = (((lhs.bottom - lhs.top) < (rhs.bottom - rhs.top)) && ((lhs.right - lhs.left) < (lhs.right - rhs.left)));
			break;
		
		case kAEGreaterThanEquals:
			result = (((lhs.bottom - lhs.top) < (rhs.bottom - rhs.top)) && ((lhs.right - lhs.left) < (lhs.right - rhs.left)));
			break;
		
		case kAEContains:
			
			result = ((lhs.top <= rhs.top) && (lhs.left <= rhs.left) && (lhs.bottom >= rhs.bottom) && (lhs.right >= rhs.right));
			break;
			
		default:
			ThrowOSErr(errAEBadTestKey);		
	}

	return result;
}

#pragma mark -






Boolean AEComparisons::TryPrimitiveComparison(DescType comparisonOperator, const AEDesc *desc1, const AEDesc *desc2)
{
	Boolean 	result = false;
	
	
	
	switch (desc1->descriptorType) 
	{
		case typeChar:
			result = CompareTexts(comparisonOperator, desc1, desc2);
			break;
		
		case typeShortInteger:		
		case typeLongInteger:		
		case typeMagnitude:			
			result = CompareInteger(comparisonOperator, desc1, desc2);
			break;

		case typeEnumerated:
			result = CompareEnumeration(comparisonOperator, desc1, desc2);
			break;
		
		case typeFixed:
			result = CompareFixed(comparisonOperator, desc1, desc2);
			break;

		case typeFloat:
			result = CompareFloat(comparisonOperator, desc1, desc2);
			break;
		
		case typeBoolean:
			result = CompareBoolean(comparisonOperator, desc1, desc2);
			break;
				
		case typeRGBColor:
			result = CompareRGBColor(comparisonOperator, desc1, desc2);
			break;
				
		case typeQDRectangle:
			result = CompareRect(comparisonOperator, desc1, desc2);
			break;
				
		case typeQDPoint:
			result = ComparePoint(comparisonOperator, desc1, desc2);
			break;
				
		default:
			ThrowOSErr(errAEWrongDataType);
	}

	return result;
}
