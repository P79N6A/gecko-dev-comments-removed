






































#ifndef AECoercionHandlers_h_
#define AECoercionHandlers_h_

#ifdef __cplusplus

class AECoercionHandlers
{
public:
			
						AECoercionHandlers();
						~AECoercionHandlers();
				
	enum {
		typePascalString = 'PStr'		
	};


protected:

	static pascal OSErr TextToPascalStringCoercion(const AEDesc *fromDesc, DescType toType, long handlerRefcon, AEDesc *toDesc);
	static pascal OSErr PascalStringToTextCoercion(const AEDesc *fromDesc, DescType toType, long handlerRefcon, AEDesc *toDesc);
	
	
protected:

	AECoerceDescUPP		mTextDescToPascalString;
	AECoerceDescUPP		mPascalStringDescToText;

public:

	static AECoercionHandlers*		GetAECoercionHandlers() { return sAECoercionHandlers; }
	static AECoercionHandlers*		sAECoercionHandlers;

};

#endif	

#ifdef __cplusplus
extern "C" {
#endif

OSErr CreateCoercionHandlers();
OSErr ShutdownCoercionHandlers();

#ifdef __cplusplus
}
#endif


#endif
