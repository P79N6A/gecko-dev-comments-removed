







































class AEComparisons
{
public:

	static Boolean	CompareTexts(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean	CompareEnumeration(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean 	CompareInteger(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean 	CompareFixed(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean 	CompareFloat(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean 	CompareBoolean(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean 	CompareRGBColor(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean	ComparePoint(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	static Boolean 	CompareRect(DescType oper, const AEDesc *desc1, const AEDesc *desc2);
	
	static Boolean 	TryPrimitiveComparison(DescType comparisonOperator, const AEDesc *desc1, const AEDesc *desc2);
	
protected:
	static Boolean EqualRGB(RGBColor colorA, RGBColor colorB)
	{
		return((colorA.red == colorB.red) && (colorA.green == colorB.green) && (colorA.blue == colorB.blue));
	}
};

