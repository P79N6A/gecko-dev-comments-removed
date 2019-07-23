













































#pragma once

#include "MRJFrame.h"

#ifndef __MACWINDOWS__
#include <MacWindows.h>
#endif

class nsIEventHandler;

class TopLevelFrame : public MRJFrame {
public:
	TopLevelFrame(nsIEventHandler* handler, JMFrameRef frameRef, JMFrameKind kind, const Rect* initialBounds, Boolean resizeable);
	virtual ~TopLevelFrame();

	virtual void setSize(const Rect* newSize);
	virtual void invalRect(const Rect* invalidRect);
	virtual void showHide(Boolean visible);
	virtual void setTitle(const StringPtr title);
	virtual void checkUpdate();
	virtual void reorder(ReorderRequest request);
	virtual void setResizeable(Boolean resizeable);
	
	virtual void activate(Boolean active);
	virtual void click(const EventRecord* event);
	
	WindowRef getWindow();

protected:
	virtual GrafPtr getPort();

private:
	nsIEventHandler* mHandler;
	WindowRef mWindow;
	Rect mBounds;
};
