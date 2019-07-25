




package org.mozilla.gecko.gfx;

import java.util.LinkedList;
import java.util.Queue;
import android.content.Context;
import android.graphics.PointF;
import android.os.SystemClock;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.view.View.OnTouchListener;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;


























public final class TouchEventHandler implements Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoTouchEventHandler";

    
    
    private final int EVENT_LISTENER_TIMEOUT = ViewConfiguration.getLongPressTimeout();

    private final LayerView mView;
    private final LayerController mController;
    private final GestureDetector mGestureDetector;
    private final SimpleScaleGestureDetector mScaleGestureDetector;

    
    
    private final Queue<MotionEvent> mEventQueue;
    private final ListenerTimeoutProcessor mListenerTimeoutProcessor;

    
    private OnTouchListener mOnTouchListener;

    
    
    private boolean mWaitForTouchListeners;

    
    
    
    private boolean mHoldInQueue;

    
    
    
    
    private boolean mDispatchEvents;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    private int mProcessingBalance;

    TouchEventHandler(Context context, LayerView view, LayerController controller) {
        mView = view;
        mController = controller;

        mEventQueue = new LinkedList<MotionEvent>();
        mGestureDetector = new GestureDetector(context, controller.getGestureListener());
        mScaleGestureDetector = new SimpleScaleGestureDetector(controller.getScaleGestureListener());
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

        if (isDownEvent(event)) {
            
            mHoldInQueue = mWaitForTouchListeners;

            
            
            
            mDispatchEvents = true;
            if (mHoldInQueue) {
                
                
                mView.postDelayed(mListenerTimeoutProcessor, EVENT_LISTENER_TIMEOUT);
            } else {
                
                
                
                
                mProcessingBalance++;
            }
        }

        
        
        
        
        
        if (mHoldInQueue) {
            mEventQueue.add(MotionEvent.obtain(event));
        } else if (mDispatchEvents) {
            dispatchEvent(event);
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

    private boolean isDownEvent(MotionEvent event) {
        int action = (event.getAction() & MotionEvent.ACTION_MASK);
        return (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN);
    }

    


    private void dispatchEvent(MotionEvent event) {
        if (mGestureDetector.onTouchEvent(event)) {
            return;
        }
        mScaleGestureDetector.onTouchEvent(event);
        if (mScaleGestureDetector.isInProgress()) {
            return;
        }
        mController.getPanZoomController().onTouchEvent(event);
    }

    



    private void processEventBlock(boolean allowDefaultAction) {
        if (!allowDefaultAction) {
            
            
            long now = SystemClock.uptimeMillis();
            dispatchEvent(MotionEvent.obtain(now, now, MotionEvent.ACTION_CANCEL, 0, 0, 0));
        }

        
        
        
        

        MotionEvent event = mEventQueue.poll();
        while (true) {
            
            
            if (allowDefaultAction) {
                dispatchEvent(event);
            }
            event = mEventQueue.peek();
            if (event == null) {
                
                
                
                
                
                mHoldInQueue = false;
                mDispatchEvents = allowDefaultAction;
                break;
            }
            if (isDownEvent(event)) {
                
                
                break;
            }
            
            
            mEventQueue.remove();
        }
    }

    private class ListenerTimeoutProcessor implements Runnable {
        
        public void run() {
            if (mProcessingBalance < 0) {
                
                
                
            } else {
                processEventBlock(true);
            }
            mProcessingBalance++;
        }
    }

    

    public void onTabChanged(Tab tab, Tabs.TabEvents msg) {
        if ((Tabs.getInstance().isSelectedTab(tab) && msg == Tabs.TabEvents.STOP) || msg == Tabs.TabEvents.SELECTED) {
            mWaitForTouchListeners = tab.getHasTouchListeners();
        }
    }
}
