




package org.mozilla.gecko.gfx;

import org.json.JSONException;

import android.graphics.PointF;
import android.util.Log;
import android.view.MotionEvent;

import java.util.LinkedList;
import java.util.ListIterator;
import java.util.Stack;



















public class SimpleScaleGestureDetector {
    private static final String LOGTAG = "GeckoSimpleScaleGestureDetector";

    private SimpleScaleGestureListener mListener;
    private long mLastEventTime;
    private boolean mScaleResult;

    
    private LinkedList<PointerInfo> mPointerInfo;

    
    public SimpleScaleGestureDetector(SimpleScaleGestureListener listener) {
        mListener = listener;
        mPointerInfo = new LinkedList<PointerInfo>();
    }

    
    public void onTouchEvent(MotionEvent event) {
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_DOWN:
            
            
            if (getPointersDown() > 0)
                onTouchEnd(event);
            onTouchStart(event);
            break;
        case MotionEvent.ACTION_POINTER_DOWN:
            onTouchStart(event);
            break;
        case MotionEvent.ACTION_MOVE:
            onTouchMove(event);
            break;
        case MotionEvent.ACTION_POINTER_UP:
        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_CANCEL:
            onTouchEnd(event);
            break;
        }
    }

    private int getPointersDown() {
        return mPointerInfo.size();
    }

    private int getActionIndex(MotionEvent event) {
        return (event.getAction() & MotionEvent.ACTION_POINTER_INDEX_MASK)
            >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
    }

    private void onTouchStart(MotionEvent event) {
        mLastEventTime = event.getEventTime();
        mPointerInfo.addFirst(PointerInfo.create(event, getActionIndex(event)));
        if (getPointersDown() == 2) {
            sendScaleGesture(EventType.BEGIN);
        }
    }

    private void onTouchMove(MotionEvent event) {
        mLastEventTime = event.getEventTime();
        for (int i = 0; i < event.getPointerCount(); i++) {
            PointerInfo pointerInfo = pointerInfoForEventIndex(event, i);
            if (pointerInfo != null) {
                pointerInfo.populate(event, i);
            }
        }

        if (getPointersDown() == 2) {
            sendScaleGesture(EventType.CONTINUE);
        }
    }

    private void onTouchEnd(MotionEvent event) {
        mLastEventTime = event.getEventTime();

        int action = event.getAction() & MotionEvent.ACTION_MASK;
        boolean isCancel = (action == MotionEvent.ACTION_CANCEL ||
                            action == MotionEvent.ACTION_DOWN);

        int id = event.getPointerId(getActionIndex(event));
        ListIterator<PointerInfo> iterator = mPointerInfo.listIterator();
        while (iterator.hasNext()) {
            PointerInfo pointerInfo = iterator.next();
            if (!(isCancel || pointerInfo.getId() == id)) {
                continue;
            }

            
            
            
            iterator.remove();
            pointerInfo.recycle();
            if (getPointersDown() == 1) {
                sendScaleGesture(EventType.END);
            }
        }
    }

    



    public float getFocusX() {
        switch (getPointersDown()) {
        case 1:
            return mPointerInfo.getFirst().getCurrent().x;
        case 2:
            PointerInfo pointerA = mPointerInfo.getFirst(), pointerB = mPointerInfo.getLast();
            return (pointerA.getCurrent().x + pointerB.getCurrent().x) / 2.0f;
        }

        Log.e(LOGTAG, "No gesture taking place in getFocusX()!");
        return 0.0f;
    }

    



    public float getFocusY() {
        switch (getPointersDown()) {
        case 1:
            return mPointerInfo.getFirst().getCurrent().y;
        case 2:
            PointerInfo pointerA = mPointerInfo.getFirst(), pointerB = mPointerInfo.getLast();
            return (pointerA.getCurrent().y + pointerB.getCurrent().y) / 2.0f;
        }

        Log.e(LOGTAG, "No gesture taking place in getFocusY()!");
        return 0.0f;
    }

    
    public float getCurrentSpan() {
        if (getPointersDown() != 2) {
            Log.e(LOGTAG, "No gesture taking place in getCurrentSpan()!");
            return 0.0f;
        }

        PointerInfo pointerA = mPointerInfo.getFirst(), pointerB = mPointerInfo.getLast();
        return PointUtils.distance(pointerA.getCurrent(), pointerB.getCurrent());
    }

    
    public float getPreviousSpan() {
        if (getPointersDown() != 2) {
            Log.e(LOGTAG, "No gesture taking place in getPreviousSpan()!");
            return 0.0f;
        }

        PointerInfo pointerA = mPointerInfo.getFirst(), pointerB = mPointerInfo.getLast();
        PointF a = pointerA.getPrevious(), b = pointerB.getPrevious();
        if (a == null || b == null) {
            a = pointerA.getCurrent();
            b = pointerB.getCurrent();
        }

        return PointUtils.distance(a, b);
    }

    
    public long getEventTime() {
        return mLastEventTime;
    }

    
    public boolean isInProgress() {
        return getPointersDown() == 2;
    }

    
    private void sendScaleGesture(EventType eventType) {
        switch (eventType) {
        case BEGIN:
            mScaleResult = mListener.onScaleBegin(this);
            break;
        case CONTINUE:
            if (mScaleResult) {
                mListener.onScale(this);
            }
            break;
        case END:
            if (mScaleResult) {
                mListener.onScaleEnd(this);
            }
            break;
        }
    }

    



    private PointerInfo pointerInfoForEventIndex(MotionEvent event, int index) {
        int id = event.getPointerId(index);
        for (PointerInfo pointerInfo : mPointerInfo) {
            if (pointerInfo.getId() == id) {
                return pointerInfo;
            }
        }
        return null;
    }

    private enum EventType {
        BEGIN,
        CONTINUE,
        END,
    }

    
    private static class PointerInfo {
        
        private static Stack<PointerInfo> sPointerInfoFreeList;

        private int mId;
        private PointF mCurrent, mPrevious;

        private PointerInfo() {
            
        }

        
        public static PointerInfo create(MotionEvent event, int index) {
            if (sPointerInfoFreeList == null) {
                sPointerInfoFreeList = new Stack<PointerInfo>();
            }

            PointerInfo pointerInfo;
            if (sPointerInfoFreeList.empty()) {
                pointerInfo = new PointerInfo();
            } else {
                pointerInfo = sPointerInfoFreeList.pop();
            }

            pointerInfo.populate(event, index);
            return pointerInfo;
        }

        



        public void populate(MotionEvent event, int index) {
            mId = event.getPointerId(index);
            mPrevious = mCurrent;
            mCurrent = new PointF(event.getX(index), event.getY(index));
        }

        public void recycle() {
            mId = -1;
            mPrevious = mCurrent = null;
            sPointerInfoFreeList.push(this);
        }

        public int getId() { return mId; }
        public PointF getCurrent() { return mCurrent; }
        public PointF getPrevious() { return mPrevious; }

        @Override
        public String toString() {
            if (mId == -1) {
                return "(up)";
            }

            try {
                String prevString;
                if (mPrevious == null) {
                    prevString = "n/a";
                } else {
                    prevString = PointUtils.toJSON(mPrevious).toString();
                }

                
                String currentString = PointUtils.toJSON(mCurrent).toString();
                return "id=" + mId + " cur=" + currentString + " prev=" + prevString;
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public static interface SimpleScaleGestureListener {
        public boolean onScale(SimpleScaleGestureDetector detector);
        public boolean onScaleBegin(SimpleScaleGestureDetector detector);
        public void onScaleEnd(SimpleScaleGestureDetector detector);
    }
}

