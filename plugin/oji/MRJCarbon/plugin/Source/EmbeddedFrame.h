













































#pragma once

#include "MRJFrame.h"

#ifndef __MACWINDOWS__
#include <MacWindows.h>
#endif

class MRJPluginInstance;
class EmbeddedFramePluginInstance;
class JSEvaluator;

class EmbeddedFrame : public MRJFrame {
public:
	EmbeddedFrame(MRJPluginInstance* pluginInstance, JMFrameRef frameRef, JMFrameKind kind, const Rect* initialBounds, Boolean resizeable);
	virtual ~EmbeddedFrame();

	virtual void setSize(const Rect* newSize);
	virtual void invalRect(const Rect* invalidRect);
	virtual void showHide(Boolean visible);
	virtual void setTitle(const StringPtr title);
	virtual void checkUpdate();
	virtual void reorder(ReorderRequest request);
	virtual void setResizeable(Boolean resizeable);
	
	virtual void activate(Boolean active);
	virtual void click(const EventRecord* event);
	
	void setPluginInstance(EmbeddedFramePluginInstance* embeddedInstance);
	
	void setWindow(WindowRef window);
	WindowRef getWindow();

protected:
	virtual GrafPtr getPort();

private:
	EmbeddedFramePluginInstance* mPluginInstance;
	JSEvaluator* mEvaluator;
	WindowRef mWindow;
	Rect mBounds;
};
