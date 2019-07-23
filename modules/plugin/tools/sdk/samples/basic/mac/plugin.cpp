




































#include "plugin.h"

#include <string.h>

#if !TARGET_API_MAC_CARBON
extern QDGlobals*	gQDPtr;
#endif






NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}





nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}





nsPluginInstance::nsPluginInstance(NPP aInstance) : nsPluginInstanceBase(),
  mInstance(aInstance),
  mInitialized(FALSE)
{
  mWindow = NULL;
}

nsPluginInstance::~nsPluginInstance()
{
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;
  
  mWindow = aWindow;
  mInitialized = TRUE;
  mSaveClip = NewRgn();
  return TRUE;
}

void nsPluginInstance::shut()
{
  mWindow = NULL;
  mInitialized = FALSE;
}

NPBool nsPluginInstance::isInitialized()
{
  return mInitialized;
}

const char * nsPluginInstance::getVersion()
{
  return NPN_UserAgent(mInstance);
}





void
nsPluginInstance::DrawString(const unsigned char* text, 
                             short width, 
                             short height, 
                             short centerX, 
                             Rect drawRect)
{
	short length, textHeight, textWidth;
 
	if(text == NULL)
		return;
	
	length = strlen((char*)text);
	TextFont(1);
	TextFace(bold);
	TextMode(srcCopy);
	TextSize(12);
	
	FontInfo fontInfo;
	GetFontInfo(&fontInfo);

	textHeight = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
	textWidth = TextWidth(text, 0, length);
		
	if (width > textWidth && height > textHeight)
	{
		MoveTo(centerX - (textWidth >> 1), height >> 1);
		DrawText(text, 0, length);
	}		
}





void 
nsPluginInstance::DoDraw(void)
{
	Rect drawRect;
  RGBColor	black = { 0x0000, 0x0000, 0x0000 };
  RGBColor	white = { 0xFFFF, 0xFFFF, 0xFFFF };
	SInt32		height = mWindow->height;
	SInt32		width = mWindow->width;
	SInt32		centerX = (width) >> 1;
	SInt32		centerY = (height) >> 1;

	const char * ua = getVersion();
	char* pascalString = (char*) NPN_MemAlloc(strlen(ua) + 1);
	strcpy(pascalString, ua);
	UInt8		*pTheText = (unsigned char*) ua;

	drawRect.top = 0;
	drawRect.left = 0;
	drawRect.bottom = drawRect.top + height;
	drawRect.right = drawRect.left + width;

  PenNormal();
  RGBForeColor(&black);
  RGBBackColor(&white);

#if !TARGET_API_MAC_CARBON
  FillRect(&drawRect, &(gQDPtr->white));
#else
  Pattern qdWhite;
  FillRect(&drawRect, GetQDGlobalsWhite(&qdWhite));
#endif

	FrameRect(&drawRect);
  DrawString(pTheText, width, height, centerX, drawRect);
}





NPError
nsPluginInstance::SetWindow(NPWindow* window)
{
	mWindow = window;
	if( StartDraw(window) ) {
		DoDraw();
		EndDraw(window);
	}
	return NPERR_NO_ERROR;
}





uint16
nsPluginInstance::HandleEvent(void* event)
{
	int16 eventHandled = FALSE;
	
	EventRecord* ev = (EventRecord*) event;
	if (event != NULL)
	{
		switch (ev->what)
		{
			


			case updateEvt:
				if( StartDraw(mWindow) ) {
					DoDraw();
					EndDraw(mWindow);
				}
				eventHandled = true;
				break;

			default:
				break;
		}
	}
	return eventHandled;
}





NPBool
nsPluginInstance::StartDraw(NPWindow* window)
{
	if (mWindow == NULL)
		return false;
		
	NP_Port* npport = (NP_Port*) mWindow->window;
	CGrafPtr ourPort = npport->port;
	
	if (mWindow->clipRect.left < mWindow->clipRect.right)
	{
		GetPort(&mSavePort);
		SetPort((GrafPtr) ourPort);
    Rect portRect;
#if !TARGET_API_MAC_CARBON
    portRect = ourPort->portRect;
#else
    GetPortBounds(ourPort, &portRect);
#endif
		mSavePortTop = portRect.top;
		mSavePortLeft = portRect.left;
		GetClip(mSaveClip);
		
		mRevealedRect.top = mWindow->clipRect.top + npport->porty;
		mRevealedRect.left = mWindow->clipRect.left + npport->portx;
		mRevealedRect.bottom = mWindow->clipRect.bottom + npport->porty;
		mRevealedRect.right = mWindow->clipRect.right + npport->portx;
		SetOrigin(npport->portx, npport->porty);
		ClipRect(&mRevealedRect);

		return true;
	}
	else
		return false;
}





void
nsPluginInstance::EndDraw(NPWindow* window)
{
	SetOrigin(mSavePortLeft, mSavePortTop);
	SetClip(mSaveClip);
	SetPort(mSavePort);
}



