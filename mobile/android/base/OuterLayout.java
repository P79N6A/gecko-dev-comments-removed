


package org.mozilla.gecko;

import android.content.Context;
import android.support.v4.view.ViewCompat;
import android.support.v4.widget.ViewDragHelper;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.RelativeLayout;




public class OuterLayout extends RelativeLayout {
    private final double AUTO_OPEN_SPEED_LIMIT = 800.0;
    private ViewDragHelper mDragHelper;
    private int mDraggingBorder;
    private int mDragRange;
    private boolean mIsOpen = false;
    private int mDraggingState = ViewDragHelper.STATE_IDLE;
    private DragCallback mDragCallback;

    public static interface DragCallback {
        public void startDrag(boolean wasOpen);
        public void stopDrag(boolean stoppingToOpen);
        public int getDragRange();
        public int getOrderedChildIndex(int index);
        public boolean canDrag(MotionEvent event);
        public boolean canInterceptEventWhileOpen(MotionEvent event);
        public void onDragProgress(float progress);
        public View getViewToDrag();
        public int getLowerLimit();
    }

    private class DragHelperCallback extends ViewDragHelper.Callback {
        @Override
        public void onViewDragStateChanged(int newState) {
            if (newState == mDraggingState) { 
                return;
            }

            
            if ((mDraggingState == ViewDragHelper.STATE_DRAGGING || mDraggingState == ViewDragHelper.STATE_SETTLING) &&
                 newState == ViewDragHelper.STATE_IDLE) {

                final float rangeToCheck = mDragRange;
                final float lowerLimit = mDragCallback.getLowerLimit();
                if (mDraggingBorder == lowerLimit) {
                    mIsOpen = false;
                    mDragCallback.onDragProgress(0);
                } else if (mDraggingBorder == rangeToCheck) {
                    mIsOpen = true;
                    mDragCallback.onDragProgress(1);
                }
                mDragCallback.stopDrag(mIsOpen);
            }

            
            if (newState == ViewDragHelper.STATE_DRAGGING && !isMoving()) {
                mDragCallback.startDrag(mIsOpen);
                updateRanges();
            }

            mDraggingState = newState;
        }

        @Override
        public void onViewPositionChanged(View changedView, int left, int top, int dx, int dy) {
            mDraggingBorder = top;
            final float progress = Math.min(1, ((float) top) / mDragRange);
            mDragCallback.onDragProgress(progress);
        }

        @Override
        public int getViewVerticalDragRange(View child) {
            return mDragRange;
        }

        @Override
        public int getOrderedChildIndex(int index) {
            return mDragCallback.getOrderedChildIndex(index);
        }

        @Override
        public boolean tryCaptureView(View view, int i) {
            return (view.getId() == mDragCallback.getViewToDrag().getId());
        }

        @Override
        public int clampViewPositionVertical(View child, int top, int dy) {
            return top;
        }

        @Override
        public void onViewReleased(View releasedChild, float xvel, float yvel) {
            final float rangeToCheck = mDragRange;
            final float speedToCheck = yvel;

            if (mDraggingBorder == mDragCallback.getLowerLimit()) {
                return;
            }

            if (mDraggingBorder == rangeToCheck) {
                return;
            }

            boolean settleToOpen = false;
            
            if (speedToCheck > AUTO_OPEN_SPEED_LIMIT) {
                settleToOpen = true;
            } else if (speedToCheck < -AUTO_OPEN_SPEED_LIMIT) {
                settleToOpen = false;
            } else if (mDraggingBorder > rangeToCheck / 2) {
                settleToOpen = true;
            } else if (mDraggingBorder < rangeToCheck / 2) {
                settleToOpen = false;
            }

            final int settleDestX;
            final int settleDestY;
            if (settleToOpen) {
                settleDestX = 0;
                settleDestY = mDragRange;
            } else {
                settleDestX = 0;
                settleDestY = mDragCallback.getLowerLimit();
            }

            if(mDragHelper.settleCapturedViewAt(settleDestX, settleDestY)) {
                ViewCompat.postInvalidateOnAnimation(OuterLayout.this);
            }
        }
    }

    public OuterLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    private void updateRanges() {
        
        mDragRange = mDragCallback.getDragRange() + mDragCallback.getLowerLimit();
    }

    private void updateOrientation() {
        mDragHelper.setEdgeTrackingEnabled(0);
    }

    @Override
    protected void onFinishInflate() {
        mDragHelper = ViewDragHelper.create(this, 1.0f, new DragHelperCallback());
        mIsOpen = false;
        super.onFinishInflate();
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (mDragCallback.canDrag(event)) {
            if (mDragHelper.shouldInterceptTouchEvent(event)) {
                return true;
            }

            
            if (mIsOpen && mDragCallback.canInterceptEventWhileOpen(event)) {
                return true;
            }
        }
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        
        if (mDragCallback.canDrag(event) || mDraggingState == ViewDragHelper.STATE_DRAGGING) {
            mDragHelper.processTouchEvent(event);
        }
        return true;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        
        

        if (mDragRange == 0) {
            mDragRange = h / 2;
        }
        super.onSizeChanged(w, h, oldw, oldh);
    }

    @Override
    public void computeScroll() { 
        if (mDragHelper.continueSettling(true)) {
            ViewCompat.postInvalidateOnAnimation(this);
        }
    }

    


    public void setClosed() {
        mIsOpen = false;
        mDragHelper.abort();
    }

    


    public void setOpen() {
        mIsOpen = true;
        mDragHelper.abort();
    }

    public void setDraggableCallback(DragCallback dragCallback) {
        mDragCallback = dragCallback;
        updateOrientation();
    }

    
    public void reset() {
        updateOrientation();
        if (isMoving()) {
            mDragHelper.abort();
            if (mDragCallback != null) {
                mDragCallback.stopDrag(false);
                mDragCallback.onDragProgress(0f);
            }
        }
    }

    public void updateDragHelperParameters() {
        mDragRange = mDragCallback.getDragRange() + mDragCallback.getLowerLimit();
        updateOrientation();
    }

    public boolean isMoving() {
        return (mDraggingState == ViewDragHelper.STATE_DRAGGING ||
                mDraggingState == ViewDragHelper.STATE_SETTLING);
    }

    public boolean isOpen() {
        return mIsOpen;
    }

    public View findTopChildUnder(MotionEvent event) {
        return mDragHelper.findTopChildUnder((int) event.getX(), (int) event.getY());
    }

    public void restoreTargetViewPosition() {
        mDragCallback.getViewToDrag().offsetTopAndBottom(mDraggingBorder);
    }
}
