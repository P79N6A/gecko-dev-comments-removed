




package org.mozilla.gecko.gfx;

import java.util.LinkedList;
import java.util.Queue;
import android.content.Context;
import android.os.SystemClock;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View.OnTouchListener;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;































public final class TouchEventHandler implements Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoTouchEventHandler";

    
    
    private final int EVENT_LISTENER_TIMEOUT = 200;

    private final LayerView mView;
    private final GestureDetector mGestureDetector;
    private final SimpleScaleGestureDetector mScaleGestureDetector;
    private final PanZoomController mPanZoomController;

    
    
    private final Queue<MotionEvent> mEventQueue;
    private final ListenerTimeoutProcessor mListenerTimeoutProcessor;

    
    private OnTouchListener mOnTouchListener;

    
    
    private boolean mWaitForTouchListeners;

    
    
    
    private boolean mHoldInQueue;

    
    
    
    
    private boolean mDispatchEvents;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    private int mProcessingBalance;

    TouchEventHandler(Context context, LayerView view, LayerController controller) {
        mView = view;

        mEventQueue = new LinkedList<MotionEvent>();
        mGestureDetector = new GestureDetector(context, controller.getGestureListener());
        mScaleGestureDetector = new SimpleScaleGestureDetector(controller.getScaleGestureListener());
        mPanZoomController = controller.getPanZoomController();
        mListenerTimeoutProcessor = new ListenerTimeoutProcessor();
        mDispatchEvents = true;

        mGestureDetector.setOnDoubleTapListener(controller.getDoubleTapListener());

        Tabs.registerOnTabsChangedListener(this);
    }

    
    public boolean handleEvent(MotionEvent event) {
        
        
        if (mOnTouchListener == null) {
            dispatchEvent(event);
            return true;
        }

        
        if (isHoverEvent(event)) {
            mOnTouchListener.onTouch(mView, event);
            return true;
        }

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

        
        mOnTouchListener.onTouch(mView, event);

        return true;
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

    
    public void setOnTouchListener(OnTouchListener onTouchListener) {
        mOnTouchListener = onTouchListener;
    }

    private boolean isHoverEvent(MotionEvent event) {
        int action = (event.getAction() & MotionEvent.ACTION_MASK);
        return (action == MotionEvent.ACTION_HOVER_ENTER || action == MotionEvent.ACTION_HOVER_MOVE || action == MotionEvent.ACTION_HOVER_EXIT);
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
        mPanZoomController.onTouchEvent(event);
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

    private final class ListenerTimeoutProcessor implements Runnable {
        
        public void run() {
            if (mProcessingBalance < 0) {
                
                
                
            } else {
                processEventBlock(true);
            }
            mProcessingBalance++;
        }
    }

    

    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        if ((Tabs.getInstance().isSelectedTab(tab) && msg == Tabs.TabEvents.STOP) || msg == Tabs.TabEvents.SELECTED) {
            mWaitForTouchListeners = tab.getHasTouchListeners();
        }
    }
}
