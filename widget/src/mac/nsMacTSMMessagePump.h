













































#ifndef NSMACTSMMESSAGEPUMP_h__
#define NSMACTSMMESSAGEPUMP_h__

#include <AppleEvents.h>
#include <TextServices.h>

class nsMacTSMMessagePump {

public:
	nsMacTSMMessagePump();
	~nsMacTSMMessagePump();

	static nsMacTSMMessagePump* GetSingleton();
	static void Shutdown();
	
private:
	static pascal OSErr PositionToOffsetHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
	static pascal OSErr OffsetToPositionHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
	static pascal OSErr UnicodeUpdateHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
	static pascal OSErr UnicodeNotFromInputMethodHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
	static pascal OSErr UnicodeGetSelectedTextHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
	static AEEventHandlerUPP mPos2OffsetUPP;
	static AEEventHandlerUPP mOffset2PosUPP;
	static AEEventHandlerUPP mUpdateUPP;
	static AEEventHandlerUPP mKeyboardUPP;
	static AEEventHandlerUPP mGetSelectedTextUPP;
	
	static nsMacTSMMessagePump* gSingleton;

};


#endif 
