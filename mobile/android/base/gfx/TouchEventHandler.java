




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;

import android.content.Context;
import android.os.SystemClock;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

import java.util.LinkedList;
import java.util.Queue;































final class TouchEventHandler implements Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoTouchEventHandler";

    
    
    private final int EVENT_LISTENER_TIMEOUT = 200;

    private final View mView;
    private final GestureDetector mGestureDetector;
    private final SimpleScaleGestureDetector mScaleGestureDetector;
    private final JavaPanZoomController mPanZoomController;

    
    
    private final Queue<MotionEvent> mEventQueue;
    private final ListenerTimeoutProcessor mListenerTimeoutProcessor;

    
    
    private boolean mWaitForTouchListeners;

    
    
    
    private boolean mHoldInQueue;

    
    
    
    
    private boolean mDispatchEvents;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    private int mProcessingBalance;

    TouchEventHandler(Context context, View view, JavaPanZoomController panZoomController) {
        mView = view;

        mEventQueue = new LinkedList<MotionEvent>();
        mPanZoomController = panZoomController;
        mGestureDetector = new GestureDetector(context, mPanZoomController);
        mScaleGestureDetector = new SimpleScaleGestureDetector(mPanZoomController);
        mListenerTimeoutProcessor = new ListenerTimeoutProcessor();
        mDispatchEvents = true;

        mGestureDetector.setOnDoubleTapListener(mPanZoomController);

        Tabs.registerOnTabsChangedListener(this);
    }

    public void destroy() {
        Tabs.unregisterOnTabsChangedListener(this);
    }

    
    public boolean handleEvent(MotionEvent event) {
        if (isDownEvent(event)) {
            
            mHoldInQueue = mWaitForTouchListeners;

            
            
            
            mDispatchEvents = true;
            if (mHoldInQueue) {
                
                
                
                if (mEventQueue.isEmpty()) {
                    mPanZoomController.startingNewEventBlock(event, true);
                }
            } else {
                
                
                
                
                mEventQueue.add(null);
                mPanZoomController.startingNewEventBlock(event, false);
            }

            
            
            mView.postDelayed(mListenerTimeoutProcessor, EVENT_LISTENER_TIMEOUT);
        }

        
        
        
        
        
        if (mHoldInQueue) {
            mEventQueue.add(MotionEvent.obtain(event));
        } else if (mDispatchEvents) {
            dispatchEvent(event);
        } else if (touchFinished(event)) {
            mPanZoomController.preventedTouchFinished();
        }

        return false;
    }

    







    public void handleEventListenerAction(boolean allowDefaultAction) {
        if (mProcessingBalance > 0) {
            
            
            
            
        } else {
            processEventBlock(allowDefaultAction);
        }
        mProcessingBalance--;
    }

    
    public void setWaitForTouchListeners(boolean aValue) {
        mWaitForTouchListeners = aValue;
    }

    private boolean isDownEvent(MotionEvent event) {
        int action = (event.getAction() & MotionEvent.ACTION_MASK);
        return (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN);
    }

    private boolean touchFinished(MotionEvent event) {
        int action = (event.getAction() & MotionEvent.ACTION_MASK);
        return (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL);
    }

    


    private void dispatchEvent(MotionEvent event) {
        if (mGestureDetector.onTouchEvent(event)) {
            return;
        }
        mScaleGestureDetector.onTouchEvent(event);
        if (mScaleGestureDetector.isInProgress()) {
            return;
        }
        mPanZoomController.handleEvent(event);
    }

    



    private void processEventBlock(boolean allowDefaultAction) {
        if (!allowDefaultAction) {
            
            
            long now = SystemClock.uptimeMillis();
            dispatchEvent(MotionEvent.obtain(now, now, MotionEvent.ACTION_CANCEL, 0, 0, 0));
        }

        if (mEventQueue.isEmpty()) {
            Log.e(LOGTAG, "Unexpected empty event queue in processEventBlock!", new Exception());
            return;
        }

        
        
        
        

        MotionEvent event = mEventQueue.poll();
        while (true) {
            
            

            if (event != null) {
                
                
                if (allowDefaultAction) {
                    dispatchEvent(event);
                } else if (touchFinished(event)) {
                    mPanZoomController.preventedTouchFinished();
                }
            }
            if (mEventQueue.isEmpty()) {
                
                
                
                
                
                mHoldInQueue = false;
                mDispatchEvents = allowDefaultAction;
                break;
            }
            event = mEventQueue.peek();
            if (event == null || isDownEvent(event)) {
                
                
                if (event != null) {
                    mPanZoomController.startingNewEventBlock(event, true);
                }
                break;
            }
            
            
            mEventQueue.remove();
        }
    }

    private class ListenerTimeoutProcessor implements Runnable {
        
        @Override
        public void run() {
            if (mProcessingBalance < 0) {
                
                
                
            } else {
                processEventBlock(true);
            }
            mProcessingBalance++;
        }
    }

    

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        if ((Tabs.getInstance().isSelectedTab(tab) && msg == Tabs.TabEvents.STOP) || msg == Tabs.TabEvents.SELECTED) {
            mWaitForTouchListeners = tab.getHasTouchListeners();
        }
    }
}
