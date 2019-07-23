





































 

#ifndef _NS_ASEAPP_H_
#define _NS_ASEAPP_H_

#include <Events.h>
#include <Dialogs.h>
#include <Navigation.h>
#include <MacTypes.h>

class nsASEApp
{

public:
	nsASEApp();
	~nsASEApp();
	
	OSErr					Run();
	
	static void				SetCompletionStatus(Boolean aVal);
	static Boolean			GetCompletionStatus(void);
	static void				FatalError(short aErrID);
	static OSErr			GotRequiredParams(AppleEvent *appEvent);
	
private:
	void					InitManagers(void);
	void					InitAEHandlers(void);
	void					MakeMenus(void);
	
	WindowPtr				mWindow;
	AEEventHandlerUPP		mEncodeUPP;
	AEEventHandlerUPP		mDecodeUPP;
	AEEventHandlerUPP		mQuitUPP;
};




#ifdef __cplusplus
extern "C" {
#endif

pascal OSErr EncodeEvent(AppleEvent *appEvent, AppleEvent *reply, SInt32 handlerRefCon);
pascal OSErr DecodeEvent(AppleEvent *appEvent, AppleEvent *reply, SInt32 handlerRefCon);
pascal OSErr QuitEvent(AppleEvent *appEvent, AppleEvent *reply, SInt32 handlerRefCon);

#ifdef __cplusplus
}
#endif

extern Boolean gDone;





#define navLoadErr 128
#define aeInitErr  130




#define rMenuBar 				128
#define rMenuApple				128
#define rMenuItemAbout			1
#define rMenuFile 				129
#define rMenuItemASEncode		1
#define rMenuItemASDecode		2
#define rMenuItemASEncodeFolder 3
#define rMenuItemASDecodeFolder 4
#define rMenuItemQuit			6
#define rMenuEdit 				130

#define rAboutBox				129





#define kASEncoderEventClass	FOUR_CHAR_CODE('ASEn')
#define kAEEncode				FOUR_CHAR_CODE('enco')
#define kAEDecode				FOUR_CHAR_CODE('deco')

#endif