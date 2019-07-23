











































#pragma once

#ifndef CALL_NOT_IN_CARBON
	#define CALL_NOT_IN_CARBON 1
#endif

#include "jni.h"
#include "JManager.h"

struct EventRecord;
struct nsPluginPrint;

class MRJFrame {
public:
	MRJFrame(JMFrameRef frameRef);
	virtual ~MRJFrame();
	
	
	virtual void setSize(const Rect* newSize);
	virtual void invalRect(const Rect* invalidRect);
	virtual void showHide(Boolean visible);
	virtual void setTitle(const StringPtr title);
	virtual void checkUpdate();
	virtual void reorder(ReorderRequest request);
	virtual void setResizeable(Boolean resizeable);
	
	
	virtual Boolean handleEvent(const EventRecord* event);
	
	virtual void idle(SInt16 modifiers);
	virtual void update();
	virtual void activate(Boolean active);
	virtual void resume(Boolean inFront);
	virtual void click(const EventRecord* event);
	virtual void click(const EventRecord* event, Point localWhere);
	virtual void keyPress(UInt32 message, SInt16 modifiers);
	virtual void keyRelease(UInt32 message, SInt16 modifiers);

	virtual void focusEvent(Boolean gotFocus);
	virtual void menuSelected(UInt32 message, SInt16 modifiers);

	virtual void print(GrafPtr printingPort, Point frameOrigin);

protected:
	virtual GrafPtr getPort() = 0;

protected:
	JMFrameRef mFrameRef;
	Boolean mActive;
	Boolean mFocused;
};
