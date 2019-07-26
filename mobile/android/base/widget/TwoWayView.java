




















package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;

import java.util.ArrayList;
import java.util.List;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.TypedArray;
import android.database.DataSetObserver;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.SystemClock;
import android.support.v4.util.LongSparseArray;
import android.support.v4.util.SparseArrayCompat;
import android.support.v4.view.MotionEventCompat;
import android.support.v4.view.VelocityTrackerCompat;
import android.support.v4.view.ViewCompat;
import android.support.v4.widget.EdgeEffectCompat;
import android.util.AttributeSet;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.ViewTreeObserver;
import android.widget.AdapterView;
import android.widget.Checkable;
import android.widget.ListAdapter;
import android.widget.Scroller;



















public class TwoWayView extends AdapterView<ListAdapter> implements
        ViewTreeObserver.OnTouchModeChangeListener {
    private static final String LOGTAG = "TwoWayView";

    private static final int NO_POSITION = -1;
    private static final int INVALID_POINTER = -1;

    public static final int[] STATE_NOTHING = new int[] { 0 };

    private static final int TOUCH_MODE_REST = -1;
    private static final int TOUCH_MODE_DOWN = 0;
    private static final int TOUCH_MODE_TAP = 1;
    private static final int TOUCH_MODE_DONE_WAITING = 2;
    private static final int TOUCH_MODE_DRAGGING = 3;
    private static final int TOUCH_MODE_FLINGING = 4;
    private static final int TOUCH_MODE_OVERSCROLL = 5;

    private static final int TOUCH_MODE_UNKNOWN = -1;
    private static final int TOUCH_MODE_ON = 0;
    private static final int TOUCH_MODE_OFF = 1;

    private static final int LAYOUT_NORMAL = 0;
    private static final int LAYOUT_FORCE_TOP = 1;
    private static final int LAYOUT_SET_SELECTION = 2;
    private static final int LAYOUT_FORCE_BOTTOM = 3;
    private static final int LAYOUT_SPECIFIC = 4;
    private static final int LAYOUT_SYNC = 5;
    private static final int LAYOUT_MOVE_SELECTION = 6;

    private static final int SYNC_SELECTED_POSITION = 0;
    private static final int SYNC_FIRST_POSITION = 1;

    private static final int SYNC_MAX_DURATION_MILLIS = 100;

    private static final int CHECK_POSITION_SEARCH_DISTANCE = 20;

    public static enum ChoiceMode {
        NONE,
        SINGLE,
        MULTIPLE
    }

    public static enum Orientation {
        HORIZONTAL,
        VERTICAL;
    };

    private ListAdapter mAdapter;

    private boolean mIsVertical;

    private int mItemMargin;

    private boolean mInLayout;
    private boolean mBlockLayoutRequests;

    private boolean mIsAttached;

    private final RecycleBin mRecycler;
    private AdapterDataSetObserver mDataSetObserver;

    final boolean[] mIsScrap = new boolean[1];

    private boolean mDataChanged;
    private int mItemCount;
    private int mOldItemCount;
    private boolean mHasStableIds;
    private boolean mAreAllItemsSelectable;

    private int mFirstPosition;
    private int mSpecificStart;

    private SavedState mPendingSync;

    private final int mTouchSlop;
    private final int mMaximumVelocity;
    private final int mFlingVelocity;
    private float mLastTouchPos;
    private float mTouchRemainderPos;
    private int mActivePointerId;

    private Rect mTouchFrame;
    private int mMotionPosition;
    private CheckForTap mPendingCheckForTap;
    private CheckForLongPress mPendingCheckForLongPress;
    private PerformClick mPerformClick;
    private Runnable mTouchModeReset;
    private int mResurrectToPosition;

    private boolean mIsChildViewEnabled;

    private boolean mDrawSelectorOnTop;
    private Drawable mSelector;
    private int mSelectorPosition;
    private final Rect mSelectorRect;

    private int mOverScroll;
    private final int mOverscrollDistance;

    private SelectionNotifier mSelectionNotifier;

    private boolean mNeedSync;
    private int mSyncMode;
    private int mSyncPosition;
    private long mSyncRowId;
    private long mSyncHeight;
    private int mSelectedStart;

    private int mNextSelectedPosition;
    private long mNextSelectedRowId;
    private int mSelectedPosition;
    private long mSelectedRowId;
    private int mOldSelectedPosition;
    private long mOldSelectedRowId;

    private ChoiceMode mChoiceMode;
    private int mCheckedItemCount;
    private SparseBooleanArray mCheckStates;
    LongSparseArray<Integer> mCheckedIdStates;

    private ContextMenuInfo mContextMenuInfo;

    private int mLayoutMode;
    private int mTouchMode;
    private int mLastTouchMode;
    private VelocityTracker mVelocityTracker;
    private final Scroller mScroller;

    private EdgeEffectCompat mStartEdge;
    private EdgeEffectCompat mEndEdge;

    private OnScrollListener mOnScrollListener;
    private int mLastScrollState;

    private View mEmptyView;

    public interface OnScrollListener {

        



        public static int SCROLL_STATE_IDLE = 0;

        


        public static int SCROLL_STATE_TOUCH_SCROLL = 1;

        



        public static int SCROLL_STATE_FLING = 2;

        










        public void onScrollStateChanged(TwoWayView view, int scrollState);

        








        public void onScroll(TwoWayView view, int firstVisibleItem, int visibleItemCount,
                int totalItemCount);
    }

    







    public static interface RecyclerListener {
        






        void onMovedToScrapHeap(View view);
    }

    public TwoWayView(Context context) {
        this(context, null);
    }

    public TwoWayView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TwoWayView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mNeedSync = false;
        mVelocityTracker = null;

        mLayoutMode = LAYOUT_NORMAL;
        mTouchMode = TOUCH_MODE_REST;
        mLastTouchMode = TOUCH_MODE_UNKNOWN;

        mIsAttached = false;

        mContextMenuInfo = null;

        mOnScrollListener = null;
        mLastScrollState = OnScrollListener.SCROLL_STATE_IDLE;

        final ViewConfiguration vc = ViewConfiguration.get(context);
        mTouchSlop = vc.getScaledTouchSlop();
        mMaximumVelocity = vc.getScaledMaximumFlingVelocity();
        mFlingVelocity = vc.getScaledMinimumFlingVelocity();
        mOverscrollDistance = getScaledOverscrollDistance(vc);

        mOverScroll = 0;

        mScroller = new Scroller(context);

        mIsVertical = true;

        mSelectorPosition = INVALID_POSITION;

        mSelectorRect = new Rect();
        mSelectedStart = 0;

        mResurrectToPosition = INVALID_POSITION;

        mSelectedStart = 0;
        mNextSelectedPosition = INVALID_POSITION;
        mNextSelectedRowId = INVALID_ROW_ID;
        mSelectedPosition = INVALID_POSITION;
        mSelectedRowId = INVALID_ROW_ID;
        mOldSelectedPosition = INVALID_POSITION;
        mOldSelectedRowId = INVALID_ROW_ID;

        mChoiceMode = ChoiceMode.NONE;
        mCheckedItemCount = 0;
        mCheckedIdStates = null;
        mCheckStates = null;

        mRecycler = new RecycleBin();
        mDataSetObserver = null;

        mAreAllItemsSelectable = true;

        mStartEdge = null;
        mEndEdge = null;

        setClickable(true);
        setFocusableInTouchMode(true);
        setWillNotDraw(false);
        setAlwaysDrawnWithCacheEnabled(false);
        setWillNotDraw(false);
        setClipToPadding(false);

        ViewCompat.setOverScrollMode(this, ViewCompat.OVER_SCROLL_IF_CONTENT_SCROLLS);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TwoWayView, defStyle, 0);
        initializeScrollbars(a);

        mDrawSelectorOnTop = a.getBoolean(
                R.styleable.TwoWayView_android_drawSelectorOnTop, false);

        Drawable d = a.getDrawable(R.styleable.TwoWayView_android_listSelector);
        if (d != null) {
            setSelector(d);
        }

        int orientation = a.getInt(R.styleable.TwoWayView_android_orientation, -1);
        if (orientation >= 0) {
            setOrientation(Orientation.values()[orientation]);
        }

        int choiceMode = a.getInt(R.styleable.TwoWayView_android_choiceMode, -1);
        if (choiceMode >= 0) {
            setChoiceMode(ChoiceMode.values()[choiceMode]);
        }

        a.recycle();

        updateScrollbarsDirection();
    }

    public void setOrientation(Orientation orientation) {
        final boolean isVertical = (orientation.compareTo(Orientation.VERTICAL) == 0);
        if (mIsVertical == isVertical) {
            return;
        }

        mIsVertical = isVertical;

        updateScrollbarsDirection();
        resetState();
        mRecycler.clear();

        requestLayout();
    }

    public Orientation getOrientation() {
        return (mIsVertical ? Orientation.VERTICAL : Orientation.HORIZONTAL);
    }

    public void setItemMargin(int itemMargin) {
        if (mItemMargin == itemMargin) {
            return;
        }

        mItemMargin = itemMargin;
        requestLayout();
    }

    public int getItemMargin() {
        return mItemMargin;
    }

    




    public void setOnScrollListener(OnScrollListener l) {
        mOnScrollListener = l;
        invokeOnItemScrollListener();
    }

    










    public void setRecyclerListener(RecyclerListener l) {
        mRecycler.mRecyclerListener = l;
    }

    








    public void setDrawSelectorOnTop(boolean drawSelectorOnTop) {
        mDrawSelectorOnTop = drawSelectorOnTop;
    }

    






    public void setSelector(int resID) {
        setSelector(getResources().getDrawable(resID));
    }

    






    public void setSelector(Drawable selector) {
        if (mSelector != null) {
            mSelector.setCallback(null);
            unscheduleDrawable(mSelector);
        }

        mSelector = selector;
        Rect padding = new Rect();
        selector.getPadding(padding);

        selector.setCallback(this);
        updateSelectorState();
    }

    





    public Drawable getSelector() {
        return mSelector;
    }

    


    @Override
    public int getSelectedItemPosition() {
        return mNextSelectedPosition;
    }

    


    @Override
    public long getSelectedItemId() {
        return mNextSelectedRowId;
    }

    












    public int getCheckedItemCount() {
        return mCheckedItemCount;
    }

    










    public boolean isItemChecked(int position) {
        if (mChoiceMode.compareTo(ChoiceMode.NONE) == 0 && mCheckStates != null) {
            return mCheckStates.get(position);
        }

        return false;
    }

    








    public int getCheckedItemPosition() {
        if (mChoiceMode.compareTo(ChoiceMode.SINGLE) == 0 &&
                mCheckStates != null && mCheckStates.size() == 1) {
            return mCheckStates.keyAt(0);
        }

        return INVALID_POSITION;
    }

    








    public SparseBooleanArray getCheckedItemPositions() {
        if (mChoiceMode.compareTo(ChoiceMode.NONE) != 0) {
            return mCheckStates;
        }

        return null;
    }

    







    public long[] getCheckedItemIds() {
        if (mChoiceMode.compareTo(ChoiceMode.NONE) == 0 ||
                mCheckedIdStates == null || mAdapter == null) {
            return new long[0];
        }

        final LongSparseArray<Integer> idStates = mCheckedIdStates;
        final int count = idStates.size();
        final long[] ids = new long[count];

        for (int i = 0; i < count; i++) {
            ids[i] = idStates.keyAt(i);
        }

        return ids;
    }

    







    public void setItemChecked(int position, boolean value) {
        if (mChoiceMode.compareTo(ChoiceMode.NONE) == 0) {
            return;
        }

        if (mChoiceMode.compareTo(ChoiceMode.MULTIPLE) == 0) {
            boolean oldValue = mCheckStates.get(position);
            mCheckStates.put(position, value);

            if (mCheckedIdStates != null && mAdapter.hasStableIds()) {
                if (value) {
                    mCheckedIdStates.put(mAdapter.getItemId(position), position);
                } else {
                    mCheckedIdStates.delete(mAdapter.getItemId(position));
                }
            }

            if (oldValue != value) {
                if (value) {
                    mCheckedItemCount++;
                } else {
                    mCheckedItemCount--;
                }
            }
        } else {
            boolean updateIds = mCheckedIdStates != null && mAdapter.hasStableIds();

            
            
            if (value || isItemChecked(position)) {
                mCheckStates.clear();

                if (updateIds) {
                    mCheckedIdStates.clear();
                }
            }

            
            
            if (value) {
                mCheckStates.put(position, true);

                if (updateIds) {
                    mCheckedIdStates.put(mAdapter.getItemId(position), position);
                }

                mCheckedItemCount = 1;
            } else if (mCheckStates.size() == 0 || !mCheckStates.valueAt(0)) {
                mCheckedItemCount = 0;
            }
        }

        
        if (!mInLayout && !mBlockLayoutRequests) {
            mDataChanged = true;
            rememberSyncState();
            requestLayout();
        }
    }

    


    public void clearChoices() {
        if (mCheckStates != null) {
            mCheckStates.clear();
        }

        if (mCheckedIdStates != null) {
            mCheckedIdStates.clear();
        }

        mCheckedItemCount = 0;
    }

    




    public ChoiceMode getChoiceMode() {
        return mChoiceMode;
    }

    








    public void setChoiceMode(ChoiceMode choiceMode) {
        mChoiceMode = choiceMode;

        if (mChoiceMode.compareTo(ChoiceMode.NONE) != 0) {
            if (mCheckStates == null) {
                mCheckStates = new SparseBooleanArray();
            }

            if (mCheckedIdStates == null && mAdapter != null && mAdapter.hasStableIds()) {
                mCheckedIdStates = new LongSparseArray<Integer>();
            }
        }
    }

    @Override
    public ListAdapter getAdapter() {
        return mAdapter;
    }

    @Override
    public void setAdapter(ListAdapter adapter) {
        if (mAdapter != null && mDataSetObserver != null) {
            mAdapter.unregisterDataSetObserver(mDataSetObserver);
        }

        resetState();
        mRecycler.clear();

        mAdapter = adapter;
        mDataChanged = true;

        mOldSelectedPosition = INVALID_POSITION;
        mOldSelectedRowId = INVALID_ROW_ID;

        if (mCheckStates != null) {
            mCheckStates.clear();
        }

        if (mCheckedIdStates != null) {
            mCheckedIdStates.clear();
        }

        if (mAdapter != null) {
            mOldItemCount = mItemCount;
            mItemCount = adapter.getCount();

            mDataSetObserver = new AdapterDataSetObserver();
            mAdapter.registerDataSetObserver(mDataSetObserver);

            mRecycler.setViewTypeCount(adapter.getViewTypeCount());

            mHasStableIds = adapter.hasStableIds();
            mAreAllItemsSelectable = adapter.areAllItemsEnabled();

            if (mChoiceMode.compareTo(ChoiceMode.NONE) != 0 && mHasStableIds &&
                    mCheckedIdStates == null) {
                mCheckedIdStates = new LongSparseArray<Integer>();
            }

            final int position = lookForSelectablePosition(0);
            setSelectedPositionInt(position);
            setNextSelectedPositionInt(position);

            if (mItemCount == 0) {
                checkSelectionChanged();
            }
        } else {
            mItemCount = 0;
            mHasStableIds = false;
            mAreAllItemsSelectable = true;

            checkSelectionChanged();
        }

        updateEmptyStatus();
        requestLayout();
    }

    @Override
    public int getFirstVisiblePosition() {
        return mFirstPosition;
    }

    @Override
    public int getLastVisiblePosition() {
        return mFirstPosition + getChildCount() - 1;
    }

    @Override
    public int getPositionForView(View view) {
        View child = view;
        try {
            View v;
            while (!(v = (View) child.getParent()).equals(this)) {
                child = v;
            }
        } catch (ClassCastException e) {
            
            return INVALID_POSITION;
        }

        
        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            if (getChildAt(i).equals(child)) {
                return mFirstPosition + i;
            }
        }

        
        return INVALID_POSITION;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        final ViewTreeObserver treeObserver = getViewTreeObserver();
        treeObserver.addOnTouchModeChangeListener(this);

        if (mAdapter != null && mDataSetObserver == null) {
            mDataSetObserver = new AdapterDataSetObserver();
            mAdapter.registerDataSetObserver(mDataSetObserver);

            
            mDataChanged = true;
            mOldItemCount = mItemCount;
            mItemCount = mAdapter.getCount();
        }

        mIsAttached = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        
        mRecycler.clear();

        final ViewTreeObserver treeObserver = getViewTreeObserver();
        treeObserver.removeOnTouchModeChangeListener(this);

        if (mAdapter != null) {
            mAdapter.unregisterDataSetObserver(mDataSetObserver);
            mDataSetObserver = null;
        }

        if (mPerformClick != null) {
            removeCallbacks(mPerformClick);
        }

        if (mTouchModeReset != null) {
            removeCallbacks(mTouchModeReset);
            mTouchModeReset.run();
        }

        mIsAttached = false;
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);

        final int touchMode = isInTouchMode() ? TOUCH_MODE_ON : TOUCH_MODE_OFF;

        if (!hasWindowFocus) {
            if (touchMode == TOUCH_MODE_OFF) {
                
                mResurrectToPosition = mSelectedPosition;
            }
        } else {
            
            if (touchMode != mLastTouchMode && mLastTouchMode != TOUCH_MODE_UNKNOWN) {
                
                if (touchMode == TOUCH_MODE_OFF) {
                    
                    resurrectSelection();

                
                } else {
                    hideSelector();
                    mLayoutMode = LAYOUT_NORMAL;
                    layoutChildren();
                }
            }
        }

        mLastTouchMode = touchMode;
    }

    @Override
    protected void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        boolean needsInvalidate = false;

        if (mIsVertical && mOverScroll != scrollY) {
            onScrollChanged(getScrollX(), scrollY, getScrollX(), mOverScroll);
            mOverScroll = scrollY;
            needsInvalidate = true;
        } else if (!mIsVertical && mOverScroll != scrollX) {
            onScrollChanged(scrollX, getScrollY(), mOverScroll, getScrollY());
            mOverScroll = scrollX;
            needsInvalidate = true;
        }

        if (needsInvalidate) {
            invalidate();
            awakenScrollbarsInternal();
        }
    }

    @TargetApi(9)
    private boolean overScrollByInternal(int deltaX, int deltaY,
            int scrollX, int scrollY,
            int scrollRangeX, int scrollRangeY,
            int maxOverScrollX, int maxOverScrollY,
            boolean isTouchEvent) {
        if (Build.VERSION.SDK_INT < 9) {
            return false;
        }

        return super.overScrollBy(deltaX, deltaY, scrollX, scrollY, scrollRangeX,
                scrollRangeY, maxOverScrollX, maxOverScrollY, isTouchEvent);
    }

    @Override
    @TargetApi(9)
    public void setOverScrollMode(int mode) {
        if (Build.VERSION.SDK_INT < 9) {
            return;
        }

        if (mode != ViewCompat.OVER_SCROLL_NEVER) {
            if (mStartEdge == null) {
                Context context = getContext();

                mStartEdge = new EdgeEffectCompat(context);
                mEndEdge = new EdgeEffectCompat(context);
            }
        } else {
            mStartEdge = null;
            mEndEdge = null;
        }

        super.setOverScrollMode(mode);
    }

    public int pointToPosition(int x, int y) {
        Rect frame = mTouchFrame;
        if (frame == null) {
            mTouchFrame = new Rect();
            frame = mTouchFrame;
        }

        final int count = getChildCount();
        for (int i = count - 1; i >= 0; i--) {
            final View child = getChildAt(i);

            if (child.getVisibility() == View.VISIBLE) {
                child.getHitRect(frame);

                if (frame.contains(x, y)) {
                    return mFirstPosition + i;
                }
            }
        }
        return INVALID_POSITION;
    }

    @Override
    protected int computeVerticalScrollExtent() {
        final int count = getChildCount();
        if (count == 0) {
            return 0;
        }

        int extent = count * 100;

        View child = getChildAt(0);
        final int childTop = child.getTop();

        int childHeight = child.getHeight();
        if (childHeight > 0) {
            extent += (childTop * 100) / childHeight;
        }

        child = getChildAt(count - 1);
        final int childBottom = child.getBottom();

        childHeight = child.getHeight();
        if (childHeight > 0) {
            extent -= ((childBottom - getHeight()) * 100) / childHeight;
        }

        return extent;
    }

    @Override
    protected int computeHorizontalScrollExtent() {
        final int count = getChildCount();
        if (count == 0) {
            return 0;
        }

        int extent = count * 100;

        View child = getChildAt(0);
        final int childLeft = child.getLeft();

        int childWidth = child.getWidth();
        if (childWidth > 0) {
            extent += (childLeft * 100) / childWidth;
        }

        child = getChildAt(count - 1);
        final int childRight = child.getRight();

        childWidth = child.getWidth();
        if (childWidth > 0) {
            extent -= ((childRight - getWidth()) * 100) / childWidth;
        }

        return extent;
    }

    @Override
    protected int computeVerticalScrollOffset() {
        final int firstPosition = mFirstPosition;
        final int childCount = getChildCount();

        if (firstPosition < 0 || childCount == 0) {
            return 0;
        }

        final View child = getChildAt(0);
        final int childTop = child.getTop();

        int childHeight = child.getHeight();
        if (childHeight > 0) {
            return Math.max(firstPosition * 100 - (childTop * 100) / childHeight, 0);
        }

        return 0;
    }

    @Override
    protected int computeHorizontalScrollOffset() {
        final int firstPosition = mFirstPosition;
        final int childCount = getChildCount();

        if (firstPosition < 0 || childCount == 0) {
            return 0;
        }

        final View child = getChildAt(0);
        final int childLeft = child.getLeft();

        int childWidth = child.getWidth();
        if (childWidth > 0) {
            return Math.max(firstPosition * 100 - (childLeft * 100) / childWidth, 0);
        }

        return 0;
    }

    @Override
    protected int computeVerticalScrollRange() {
        int result = Math.max(mItemCount * 100, 0);

        if (mIsVertical && mOverScroll != 0) {
            
            result += Math.abs((int) ((float) mOverScroll / getHeight() * mItemCount * 100));
        }

        return result;
    }

    @Override
    protected int computeHorizontalScrollRange() {
        int result = Math.max(mItemCount * 100, 0);

        if (!mIsVertical && mOverScroll != 0) {
            
            result += Math.abs((int) ((float) mOverScroll / getWidth() * mItemCount * 100));
        }

        return result;
    }

    @Override
    public boolean showContextMenuForChild(View originalView) {
        final int longPressPosition = getPositionForView(originalView);
        if (longPressPosition >= 0) {
            final long longPressId = mAdapter.getItemId(longPressPosition);
            boolean handled = false;

            OnItemLongClickListener listener = getOnItemLongClickListener();
            if (listener != null) {
                handled = listener.onItemLongClick(TwoWayView.this, originalView,
                        longPressPosition, longPressId);
            }

            if (!handled) {
                mContextMenuInfo = createContextMenuInfo(
                        getChildAt(longPressPosition - mFirstPosition),
                        longPressPosition, longPressId);

                handled = super.showContextMenuForChild(originalView);
            }

            return handled;
        }

        return false;
    }

    @Override
    public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
        if (disallowIntercept) {
            recycleVelocityTracker();
        }

        super.requestDisallowInterceptTouchEvent(disallowIntercept);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (!mIsAttached) {
            return false;
        }

        final int action = ev.getAction() & MotionEventCompat.ACTION_MASK;
        switch (action) {
        case MotionEvent.ACTION_DOWN:
            initOrResetVelocityTracker();
            mVelocityTracker.addMovement(ev);

            mScroller.abortAnimation();

            final float x = ev.getX();
            final float y = ev.getY();

            mLastTouchPos = (mIsVertical ? y : x);

            final int motionPosition = findMotionRowOrColumn((int) mLastTouchPos);

            mActivePointerId = MotionEventCompat.getPointerId(ev, 0);
            mTouchRemainderPos = 0;

            if (mTouchMode == TOUCH_MODE_FLINGING) {
                return true;
            } else if (motionPosition >= 0) {
                mMotionPosition = motionPosition;
                mTouchMode = TOUCH_MODE_DOWN;
            }

            break;

        case MotionEvent.ACTION_MOVE: {
            if (mTouchMode != TOUCH_MODE_DOWN) {
                break;
            }

            initVelocityTrackerIfNotExists();
            mVelocityTracker.addMovement(ev);

            final int index = MotionEventCompat.findPointerIndex(ev, mActivePointerId);
            if (index < 0) {
                Log.e(LOGTAG, "onInterceptTouchEvent could not find pointer with id " +
                        mActivePointerId + " - did TwoWayView receive an inconsistent " +
                        "event stream?");
                return false;
            }

            final float pos;
            if (mIsVertical) {
                pos = MotionEventCompat.getY(ev, index);
            } else {
                pos = MotionEventCompat.getX(ev, index);
            }

            final float diff = pos - mLastTouchPos + mTouchRemainderPos;
            final int delta = (int) diff;
            mTouchRemainderPos = diff - delta;

            if (maybeStartScrolling(delta)) {
                return true;
            }
        }

        case MotionEvent.ACTION_CANCEL:
        case MotionEvent.ACTION_UP:
            mActivePointerId = INVALID_POINTER;
            mTouchMode = TOUCH_MODE_REST;
            recycleVelocityTracker();
            reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);

            break;
        }

        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (!isEnabled()) {
            
            
            return isClickable() || isLongClickable();
        }

        if (!mIsAttached) {
            return false;
        }

        boolean needsInvalidate = false;

        initVelocityTrackerIfNotExists();
        mVelocityTracker.addMovement(ev);

        final int action = ev.getAction() & MotionEventCompat.ACTION_MASK;
        switch (action) {
        case MotionEvent.ACTION_DOWN: {
            if (mDataChanged) {
                break;
            }

            mVelocityTracker.clear();
            mScroller.abortAnimation();

            final float x = ev.getX();
            final float y = ev.getY();

            mLastTouchPos = (mIsVertical ? y : x);

            int motionPosition = pointToPosition((int) x, (int) y);

            mActivePointerId = MotionEventCompat.getPointerId(ev, 0);
            mTouchRemainderPos = 0;

            if (mDataChanged) {
                break;
            }

            if (mTouchMode == TOUCH_MODE_FLINGING) {
                mTouchMode = TOUCH_MODE_DRAGGING;
                reportScrollStateChange(OnScrollListener.SCROLL_STATE_TOUCH_SCROLL);
                motionPosition = findMotionRowOrColumn((int) mLastTouchPos);
                return true;
            } else if (mMotionPosition >= 0 && mAdapter.isEnabled(mMotionPosition)) {
                mTouchMode = TOUCH_MODE_DOWN;
                triggerCheckForTap();
            }

            mMotionPosition = motionPosition;

            break;
        }

        case MotionEvent.ACTION_MOVE: {
            final int index = MotionEventCompat.findPointerIndex(ev, mActivePointerId);
            if (index < 0) {
                Log.e(LOGTAG, "onInterceptTouchEvent could not find pointer with id " +
                        mActivePointerId + " - did TwoWayView receive an inconsistent " +
                        "event stream?");
                return false;
            }

            final float pos;
            if (mIsVertical) {
                pos = MotionEventCompat.getY(ev, index);
            } else {
                pos = MotionEventCompat.getX(ev, index);
            }

            if (mDataChanged) {
                
                
                layoutChildren();
            }

            final float diff = pos - mLastTouchPos + mTouchRemainderPos;
            final int delta = (int) diff;
            mTouchRemainderPos = diff - delta;

            switch (mTouchMode) {
            case TOUCH_MODE_DOWN:
            case TOUCH_MODE_TAP:
            case TOUCH_MODE_DONE_WAITING:
                
                
                maybeStartScrolling(delta);
                break;

            case TOUCH_MODE_DRAGGING:
            case TOUCH_MODE_OVERSCROLL:
                mLastTouchPos = pos;
                maybeScroll(delta);
                break;
            }

            break;
        }

        case MotionEvent.ACTION_CANCEL:
            cancelCheckForTap();
            mTouchMode = TOUCH_MODE_REST;
            reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);

            setPressed(false);
            View motionView = this.getChildAt(mMotionPosition - mFirstPosition);
            if (motionView != null) {
                motionView.setPressed(false);
            }

            if (mStartEdge != null && mEndEdge != null) {
                needsInvalidate = mStartEdge.onRelease() | mEndEdge.onRelease();
            }

            recycleVelocityTracker();

            break;

        case MotionEvent.ACTION_UP: {
            switch (mTouchMode) {
            case TOUCH_MODE_DOWN:
            case TOUCH_MODE_TAP:
            case TOUCH_MODE_DONE_WAITING: {
                final int motionPosition = mMotionPosition;
                final View child = getChildAt(motionPosition - mFirstPosition);

                final float x = ev.getX();
                final float y = ev.getY();

                boolean inList = false;
                if (mIsVertical) {
                    inList = x > getPaddingLeft() && x < getWidth() - getPaddingRight();
                } else {
                    inList = y > getPaddingTop() && y < getHeight() - getPaddingBottom();
                }

                if (child != null && !child.hasFocusable() && inList) {
                    if (mTouchMode != TOUCH_MODE_DOWN) {
                        child.setPressed(false);
                    }

                    if (mPerformClick == null) {
                        mPerformClick = new PerformClick();
                    }

                    final PerformClick performClick = mPerformClick;
                    performClick.mClickMotionPosition = motionPosition;
                    performClick.rememberWindowAttachCount();

                    mResurrectToPosition = motionPosition;

                    if (mTouchMode == TOUCH_MODE_DOWN || mTouchMode == TOUCH_MODE_TAP) {
                        if (mTouchMode == TOUCH_MODE_DOWN) {
                            cancelCheckForTap();
                        } else {
                            cancelCheckForLongPress();
                        }

                        mLayoutMode = LAYOUT_NORMAL;

                        if (!mDataChanged && mAdapter.isEnabled(motionPosition)) {
                            mTouchMode = TOUCH_MODE_TAP;

                            setPressed(true);
                            positionSelector(mMotionPosition, child);
                            child.setPressed(true);

                            if (mSelector != null) {
                                Drawable d = mSelector.getCurrent();
                                if (d != null && d instanceof TransitionDrawable) {
                                    ((TransitionDrawable) d).resetTransition();
                                }
                            }

                            if (mTouchModeReset != null) {
                                removeCallbacks(mTouchModeReset);
                            }

                            mTouchModeReset = new Runnable() {
                                @Override
                                public void run() {
                                    mTouchMode = TOUCH_MODE_REST;

                                    setPressed(false);
                                    child.setPressed(false);

                                    if (!mDataChanged) {
                                        performClick.run();
                                    }

                                    mTouchModeReset = null;
                                }
                            };

                            postDelayed(mTouchModeReset,
                                    ViewConfiguration.getPressedStateDuration());
                        } else {
                            mTouchMode = TOUCH_MODE_REST;
                            updateSelectorState();
                        }
                    } else if (!mDataChanged && mAdapter.isEnabled(motionPosition)) {
                        performClick.run();
                    }
                }

                mTouchMode = TOUCH_MODE_REST;
                updateSelectorState();

                break;
            }

            case TOUCH_MODE_DRAGGING:
                if (contentFits()) {
                    mTouchMode = TOUCH_MODE_REST;
                    reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
                    break;
                }

                mVelocityTracker.computeCurrentVelocity(1000, mMaximumVelocity);

                final float velocity;
                if (mIsVertical) {
                    velocity = VelocityTrackerCompat.getYVelocity(mVelocityTracker,
                            mActivePointerId);
                } else {
                    velocity = VelocityTrackerCompat.getXVelocity(mVelocityTracker,
                            mActivePointerId);
                }

                if (Math.abs(velocity) >= mFlingVelocity) {
                    mTouchMode = TOUCH_MODE_FLINGING;
                    reportScrollStateChange(OnScrollListener.SCROLL_STATE_FLING);

                    mScroller.fling(0, 0,
                                    (int) (mIsVertical ? 0 : velocity),
                                    (int) (mIsVertical ? velocity : 0),
                                    (mIsVertical ? 0 : Integer.MIN_VALUE),
                                    (mIsVertical ? 0 : Integer.MAX_VALUE),
                                    (mIsVertical ? Integer.MIN_VALUE : 0),
                                    (mIsVertical ? Integer.MAX_VALUE : 0));

                    mLastTouchPos = 0;
                    needsInvalidate = true;
                } else {
                    mTouchMode = TOUCH_MODE_REST;
                    reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
                }

                break;

            case TOUCH_MODE_OVERSCROLL:
                mTouchMode = TOUCH_MODE_REST;
                reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
            }

            cancelCheckForTap();
            cancelCheckForLongPress();
            setPressed(false);

            if (mStartEdge != null && mEndEdge != null) {
                needsInvalidate |= mStartEdge.onRelease() | mEndEdge.onRelease();
            }

            recycleVelocityTracker();

            break;
        }
        }

        if (needsInvalidate) {
            ViewCompat.postInvalidateOnAnimation(this);
        }

        return true;
    }

    @Override
    public void onTouchModeChanged(boolean isInTouchMode) {
        if (isInTouchMode) {
            
            hideSelector();

            
            
            
            if (getWidth() > 0 && getHeight() > 0 && getChildCount() > 0) {
                layoutChildren();
            }

            updateSelectorState();
        } else {
            final int touchMode = mTouchMode;
            if (touchMode == TOUCH_MODE_OVERSCROLL) {
                if (mOverScroll != 0) {
                    mOverScroll = 0;
                    finishEdgeGlows();
                    invalidate();
                }
            }
        }
    }

    private void initOrResetVelocityTracker() {
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        } else {
            mVelocityTracker.clear();
        }
    }

    private void initVelocityTrackerIfNotExists() {
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
    }

    private void recycleVelocityTracker() {
        if (mVelocityTracker != null) {
            mVelocityTracker.recycle();
            mVelocityTracker = null;
        }
    }

    


    private void invokeOnItemScrollListener() {
        if (mOnScrollListener != null) {
            mOnScrollListener.onScroll(this, mFirstPosition, getChildCount(), mItemCount);
        }

        
        onScrollChanged(0, 0, 0, 0);
    }

    private void reportScrollStateChange(int newState) {
        if (newState == mLastScrollState) {
            return;
        }

        if (mOnScrollListener != null) {
            mLastScrollState = newState;
            mOnScrollListener.onScrollStateChanged(this, newState);
        }
    }

    private boolean maybeStartScrolling(int delta) {
        final boolean isOverScroll = (mOverScroll != 0);
        if (Math.abs(delta) <= mTouchSlop && !isOverScroll) {
            return false;
        }

        if (isOverScroll) {
            mTouchMode = TOUCH_MODE_OVERSCROLL;
        } else {
            mTouchMode = TOUCH_MODE_DRAGGING;
        }

        
        
        final ViewParent parent = getParent();
        if (parent != null) {
            parent.requestDisallowInterceptTouchEvent(true);
        }

        cancelCheckForLongPress();

        setPressed(false);
        View motionView = getChildAt(mMotionPosition - mFirstPosition);
        if (motionView != null) {
            motionView.setPressed(false);
        }

        reportScrollStateChange(OnScrollListener.SCROLL_STATE_TOUCH_SCROLL);

        return true;
    }

    private void maybeScroll(int delta) {
        if (mTouchMode == TOUCH_MODE_DRAGGING) {
            handleDragChange(delta);
        } else if (mTouchMode == TOUCH_MODE_OVERSCROLL) {
            handleOverScrollChange(delta);
        }
    }

    private void handleDragChange(int delta) {
        
        
        final ViewParent parent = getParent();
        if (parent != null) {
            parent.requestDisallowInterceptTouchEvent(true);
        }

        final int motionIndex;
        if (mMotionPosition >= 0) {
            motionIndex = mMotionPosition - mFirstPosition;
        } else {
            
            
            motionIndex = getChildCount() / 2;
        }

        int motionViewPrevStart = 0;
        View motionView = this.getChildAt(motionIndex);
        if (motionView != null) {
            motionViewPrevStart = (mIsVertical ? motionView.getTop() : motionView.getLeft());
        }

        boolean atEdge = trackMotionScroll(delta);

        motionView = this.getChildAt(motionIndex);
        if (motionView != null) {
            final int motionViewRealStart =
                    (mIsVertical ? motionView.getTop() : motionView.getLeft());

            if (atEdge) {
                final int overscroll = -delta - (motionViewRealStart - motionViewPrevStart);
                updateOverScrollState(delta, overscroll);
            }
        }
    }

    private void updateOverScrollState(int delta, int overscroll) {
        overScrollByInternal((mIsVertical ? 0 : overscroll),
                             (mIsVertical ? overscroll : 0),
                             (mIsVertical ? 0 : mOverScroll),
                             (mIsVertical ? mOverScroll : 0),
                             0, 0,
                             (mIsVertical ? 0 : mOverscrollDistance),
                             (mIsVertical ? mOverscrollDistance : 0),
                             true);

        if (Math.abs(mOverscrollDistance) == Math.abs(mOverScroll)) {
            
            if (mVelocityTracker != null) {
                mVelocityTracker.clear();
            }
        }

        final int overscrollMode = ViewCompat.getOverScrollMode(this);
        if (overscrollMode == ViewCompat.OVER_SCROLL_ALWAYS ||
                (overscrollMode == ViewCompat.OVER_SCROLL_IF_CONTENT_SCROLLS && !contentFits())) {
            mTouchMode = TOUCH_MODE_OVERSCROLL;

            float pull = (float) overscroll / (mIsVertical ? getHeight() : getWidth());
            if (delta > 0) {
                mStartEdge.onPull(pull);

                if (!mEndEdge.isFinished()) {
                    mEndEdge.onRelease();
                }
            } else if (delta < 0) {
                mEndEdge.onPull(pull);

                if (!mStartEdge.isFinished()) {
                    mStartEdge.onRelease();
                }
            }

            if (delta != 0) {
                ViewCompat.postInvalidateOnAnimation(this);
            }
        }
    }

    private void handleOverScrollChange(int delta) {
        final int oldOverScroll = mOverScroll;
        final int newOverScroll = oldOverScroll - delta;

        int overScrollDistance = -delta;
        if ((newOverScroll < 0 && oldOverScroll >= 0) ||
                (newOverScroll > 0 && oldOverScroll <= 0)) {
            overScrollDistance = -oldOverScroll;
            delta += overScrollDistance;
        } else {
            delta = 0;
        }

        if (overScrollDistance != 0) {
            updateOverScrollState(delta, overScrollDistance);
        }

        if (delta != 0) {
            if (mOverScroll != 0) {
                mOverScroll = 0;
                ViewCompat.postInvalidateOnAnimation(this);
            }

            trackMotionScroll(delta);
            mTouchMode = TOUCH_MODE_DRAGGING;

            
            
            mMotionPosition = findClosestMotionRowOrColumn((int) mLastTouchPos);
            mTouchRemainderPos = 0;
        }
    }

    int findMotionRowOrColumn(int motionPos) {
        int childCount = getChildCount();
        if (childCount == 0) {
            return INVALID_POSITION;
        }

        for (int i = 0; i < childCount; i++) {
            View v = getChildAt(i);

            if ((mIsVertical && motionPos <= v.getBottom()) ||
                    (!mIsVertical && motionPos <= v.getRight())) {
                return mFirstPosition + i;
            }
        }

        return INVALID_POSITION;
    }

    private int findClosestMotionRowOrColumn(int motionPos) {
        final int childCount = getChildCount();
        if (childCount == 0) {
            return INVALID_POSITION;
        }

        final int motionRow = findMotionRowOrColumn(motionPos);
        if (motionRow != INVALID_POSITION) {
            return motionRow;
        } else {
            return mFirstPosition + childCount - 1;
        }
    }

    @TargetApi(9)
    private int getScaledOverscrollDistance(ViewConfiguration vc) {
        if (Build.VERSION.SDK_INT < 9) {
            return 0;
        }

        return vc.getScaledOverscrollDistance();
    }

    private boolean contentFits() {
        final int childCount = getChildCount();
        if (childCount == 0) {
            return true;
        }

        if (childCount != mItemCount) {
            return false;
        }

        View first = getChildAt(0);
        View last = getChildAt(childCount - 1);

        if (mIsVertical) {
            return first.getTop() >= getPaddingTop() &&
                    last.getBottom() <= getHeight() - getPaddingBottom();
        } else {
            return first.getLeft() >= getPaddingLeft() &&
                    last.getRight() <= getWidth() - getPaddingRight();
        }
    }

    private void updateScrollbarsDirection() {
        setHorizontalScrollBarEnabled(!mIsVertical);
        setVerticalScrollBarEnabled(mIsVertical);
    }

    private void triggerCheckForTap() {
        if (mPendingCheckForTap == null) {
            mPendingCheckForTap = new CheckForTap();
        }

        postDelayed(mPendingCheckForTap, ViewConfiguration.getTapTimeout());
    }

    private void cancelCheckForTap() {
        if (mPendingCheckForTap == null) {
            return;
        }

        removeCallbacks(mPendingCheckForTap);
    }

    private void triggerCheckForLongPress() {
        if (mPendingCheckForLongPress == null) {
            mPendingCheckForLongPress = new CheckForLongPress();
        }

        mPendingCheckForLongPress.rememberWindowAttachCount();

        postDelayed(mPendingCheckForLongPress,
                ViewConfiguration.getLongPressTimeout());
    }

    private void cancelCheckForLongPress() {
        if (mPendingCheckForLongPress == null) {
            return;
        }

        removeCallbacks(mPendingCheckForLongPress);
    }

    boolean trackMotionScroll(int incrementalDelta) {
        final int childCount = getChildCount();
        if (childCount == 0) {
            return true;
        }

        final View first = getChildAt(0);
        final int firstStart = (mIsVertical ? first.getTop() : first.getLeft());

        final View last = getChildAt(childCount - 1);
        final int lastEnd = (mIsVertical ? last.getBottom() : last.getRight());

        final int paddingTop = getPaddingTop();
        final int paddingBottom = getPaddingBottom();
        final int paddingLeft = getPaddingLeft();
        final int paddingRight = getPaddingRight();

        final int paddingStart = (mIsVertical ? paddingTop : paddingLeft);

        final int spaceBefore = paddingStart - firstStart;
        final int end = (mIsVertical ? getHeight() - paddingBottom :
            getWidth() - paddingRight);
        final int spaceAfter = lastEnd - end;

        final int size;
        if (mIsVertical) {
            size = getHeight() - paddingBottom - paddingTop;
        } else {
            size = getWidth() - paddingRight - paddingLeft;
        }

        if (incrementalDelta < 0) {
            incrementalDelta = Math.max(-(size - 1), incrementalDelta);
        } else {
            incrementalDelta = Math.min(size - 1, incrementalDelta);
        }

        final int firstPosition = mFirstPosition;

        final boolean cannotScrollDown = (firstPosition == 0 &&
                firstStart >= paddingStart && incrementalDelta >= 0);
        final boolean cannotScrollUp = (firstPosition + childCount == mItemCount &&
                lastEnd <= end && incrementalDelta <= 0);

        if (cannotScrollDown || cannotScrollUp) {
            return incrementalDelta != 0;
        }

        final boolean inTouchMode = isInTouchMode();
        if (inTouchMode) {
            hideSelector();
        }

        int start = 0;
        int count = 0;

        final boolean down = (incrementalDelta < 0);
        if (down) {
            int childrenStart = -incrementalDelta + paddingStart;

            for (int i = 0; i < childCount; i++) {
                final View child = getChildAt(i);
                final int childEnd = (mIsVertical ? child.getBottom() : child.getRight());

                if (childEnd >= childrenStart) {
                    break;
                }

                count++;
                mRecycler.addScrapView(child, firstPosition + i);
            }
        } else {
            int childrenEnd = end - incrementalDelta;

            for (int i = childCount - 1; i >= 0; i--) {
                final View child = getChildAt(i);
                final int childStart = (mIsVertical ? child.getTop() : child.getLeft());

                if (childStart <= childrenEnd) {
                    break;
                }

                start = i;
                count++;
                mRecycler.addScrapView(child, firstPosition + i);
            }
        }

        mBlockLayoutRequests = true;

        if (count > 0) {
            detachViewsFromParent(start, count);
        }

        
        
        if (!awakenScrollbarsInternal()) {
           invalidate();
        }

        offsetChildren(incrementalDelta);

        if (down) {
            mFirstPosition += count;
        }

        final int absIncrementalDelta = Math.abs(incrementalDelta);
        if (spaceBefore < absIncrementalDelta || spaceAfter < absIncrementalDelta) {
            fillGap(down);
        }

        if (!inTouchMode && mSelectedPosition != INVALID_POSITION) {
            final int childIndex = mSelectedPosition - mFirstPosition;
            if (childIndex >= 0 && childIndex < getChildCount()) {
                positionSelector(mSelectedPosition, getChildAt(childIndex));
            }
        } else if (mSelectorPosition != INVALID_POSITION) {
            final int childIndex = mSelectorPosition - mFirstPosition;
            if (childIndex >= 0 && childIndex < getChildCount()) {
                positionSelector(INVALID_POSITION, getChildAt(childIndex));
            }
        } else {
            mSelectorRect.setEmpty();
        }

        mBlockLayoutRequests = false;

        invokeOnItemScrollListener();

        return false;
    }

    @TargetApi(14)
    private final float getCurrVelocity() {
        if (Build.VERSION.SDK_INT >= 14) {
            return mScroller.getCurrVelocity();
        }

        return 0;
    }

    @TargetApi(5)
    private boolean awakenScrollbarsInternal() {
        if (Build.VERSION.SDK_INT >= 5) {
            return super.awakenScrollBars();
        } else {
            return false;
        }
    }

    @Override
    public void computeScroll() {
        if (!mScroller.computeScrollOffset()) {
            return;
        }

        final int pos;
        if (mIsVertical) {
            pos = mScroller.getCurrY();
        } else {
            pos = mScroller.getCurrX();
        }

        final int diff = (int) (pos - mLastTouchPos);
        mLastTouchPos = pos;

        final boolean stopped = trackMotionScroll(diff);

        if (!stopped && !mScroller.isFinished()) {
            ViewCompat.postInvalidateOnAnimation(this);
        } else {
            if (stopped) {
                final int overScrollMode = ViewCompat.getOverScrollMode(this);
                if (overScrollMode != ViewCompat.OVER_SCROLL_NEVER) {
                    final EdgeEffectCompat edge =
                            (diff > 0 ? mStartEdge : mEndEdge);

                    boolean needsInvalidate =
                            edge.onAbsorb(Math.abs((int) getCurrVelocity()));

                    if (needsInvalidate) {
                        ViewCompat.postInvalidateOnAnimation(this);
                    }
                }

                mScroller.abortAnimation();
            }

            mTouchMode = TOUCH_MODE_REST;
            reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
        }
    }

    private void finishEdgeGlows() {
        if (mStartEdge != null) {
            mStartEdge.finish();
        }

        if (mEndEdge != null) {
            mEndEdge.finish();
        }
    }

    private boolean drawStartEdge(Canvas canvas) {
        if (mStartEdge.isFinished()) {
            return false;
        }

        if (mIsVertical) {
            return mStartEdge.draw(canvas);
        }

        final int restoreCount = canvas.save();
        final int height = getHeight() - getPaddingTop() - getPaddingBottom();

        canvas.translate(0, height);
        canvas.rotate(270);

        final boolean needsInvalidate = mStartEdge.draw(canvas);
        canvas.restoreToCount(restoreCount);
        return needsInvalidate;
    }

    private boolean drawEndEdge(Canvas canvas) {
        if (mEndEdge.isFinished()) {
            return false;
        }

        final int restoreCount = canvas.save();
        final int width = getWidth() - getPaddingLeft() - getPaddingRight();
        final int height = getHeight() - getPaddingTop() - getPaddingBottom();

        if (mIsVertical) {
            canvas.translate(-width, height);
            canvas.rotate(180, width, 0);
        } else {
            canvas.translate(width, 0);
            canvas.rotate(90);
        }

        final boolean needsInvalidate = mEndEdge.draw(canvas);
        canvas.restoreToCount(restoreCount);
        return needsInvalidate;
    }

    private void drawSelector(Canvas canvas) {
        if (!mSelectorRect.isEmpty()) {
            final Drawable selector = mSelector;
            selector.setBounds(mSelectorRect);
            selector.draw(canvas);
        }
    }

    private void useDefaultSelector() {
        setSelector(getResources().getDrawable(
                android.R.drawable.list_selector_background));
    }

    private boolean shouldShowSelector() {
        return (hasFocus() && !isInTouchMode()) || touchModeDrawsInPressedState();
    }

    private void positionSelector(int position, View selected) {
        if (position != INVALID_POSITION) {
            mSelectorPosition = position;
        }

        mSelectorRect.set(selected.getLeft(), selected.getTop(), selected.getRight(),
                selected.getBottom());

        final boolean isChildViewEnabled = mIsChildViewEnabled;
        if (selected.isEnabled() != isChildViewEnabled) {
            mIsChildViewEnabled = !isChildViewEnabled;

            if (getSelectedItemPosition() != INVALID_POSITION) {
                refreshDrawableState();
            }
        }
    }

    private void hideSelector() {
        if (mSelectedPosition != INVALID_POSITION) {
            if (mLayoutMode != LAYOUT_SPECIFIC) {
                mResurrectToPosition = mSelectedPosition;
            }

            if (mNextSelectedPosition >= 0 && mNextSelectedPosition != mSelectedPosition) {
                mResurrectToPosition = mNextSelectedPosition;
            }

            setSelectedPositionInt(INVALID_POSITION);
            setNextSelectedPositionInt(INVALID_POSITION);

            mSelectedStart = 0;
        }
    }

    private void setSelectedPositionInt(int position) {
        mSelectedPosition = position;
        mSelectedRowId = getItemIdAtPosition(position);
    }

    private void setSelectionInt(int position) {
        setNextSelectedPositionInt(position);
        boolean awakeScrollbars = false;

        final int selectedPosition = mSelectedPosition;
        if (selectedPosition >= 0) {
            if (position == selectedPosition - 1) {
                awakeScrollbars = true;
            } else if (position == selectedPosition + 1) {
                awakeScrollbars = true;
            }
        }

        layoutChildren();

        if (awakeScrollbars) {
            awakenScrollbarsInternal();
        }
    }

    private void setNextSelectedPositionInt(int position) {
        mNextSelectedPosition = position;
        mNextSelectedRowId = getItemIdAtPosition(position);

        
        if (mNeedSync && mSyncMode == SYNC_SELECTED_POSITION && position >= 0) {
            mSyncPosition = position;
            mSyncRowId = mNextSelectedRowId;
        }
    }

    private boolean touchModeDrawsInPressedState() {
        switch (mTouchMode) {
        case TOUCH_MODE_TAP:
        case TOUCH_MODE_DONE_WAITING:
            return true;
        default:
            return false;
        }
    }

    private void updateSelectorState() {
        if (mSelector != null) {
            if (shouldShowSelector()) {
                mSelector.setState(getDrawableState());
            } else {
                mSelector.setState(STATE_NOTHING);
            }
        }
    }

    private void checkSelectionChanged() {
        if ((mSelectedPosition != mOldSelectedPosition) || (mSelectedRowId != mOldSelectedRowId)) {
            selectionChanged();
            mOldSelectedPosition = mSelectedPosition;
            mOldSelectedRowId = mSelectedRowId;
        }
    }

    private void selectionChanged() {
        OnItemSelectedListener listener = getOnItemSelectedListener();
        if (listener == null) {
            return;
        }

        if (mInLayout || mBlockLayoutRequests) {
            
            
            
            
            if (mSelectionNotifier == null) {
                mSelectionNotifier = new SelectionNotifier();
            }

            post(mSelectionNotifier);
        } else {
            fireOnSelected();
        }
    }

    private void fireOnSelected() {
        OnItemSelectedListener listener = getOnItemSelectedListener();
        if (listener == null) {
            return;
        }

        final int selection = getSelectedItemPosition();
        if (selection >= 0) {
            View v = getSelectedView();
            listener.onItemSelected(this, v, selection,
                    mAdapter.getItemId(selection));
        } else {
            listener.onNothingSelected(this);
        }
    }

    private int lookForSelectablePosition(int position) {
        return lookForSelectablePosition(position, true);
    }

    private int lookForSelectablePosition(int position, boolean lookDown) {
        final ListAdapter adapter = mAdapter;
        if (adapter == null || isInTouchMode()) {
            return INVALID_POSITION;
        }

        final int itemCount = mItemCount;
        if (!mAreAllItemsSelectable) {
            if (lookDown) {
                position = Math.max(0, position);
                while (position < itemCount && !adapter.isEnabled(position)) {
                    position++;
                }
            } else {
                position = Math.min(position, itemCount - 1);
                while (position >= 0 && !adapter.isEnabled(position)) {
                    position--;
                }
            }

            if (position < 0 || position >= itemCount) {
                return INVALID_POSITION;
            }

            return position;
        } else {
            if (position < 0 || position >= itemCount) {
                return INVALID_POSITION;
            }

            return position;
        }
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        updateSelectorState();
    }

    @Override
    protected int[] onCreateDrawableState(int extraSpace) {
        
        if (mIsChildViewEnabled) {
            
            return super.onCreateDrawableState(extraSpace);
        }

        
        
        
        final int enabledState = ENABLED_STATE_SET[0];

        
        
        
        int[] state = super.onCreateDrawableState(extraSpace + 1);
        int enabledPos = -1;
        for (int i = state.length - 1; i >= 0; i--) {
            if (state[i] == enabledState) {
                enabledPos = i;
                break;
            }
        }

        
        if (enabledPos >= 0) {
            System.arraycopy(state, enabledPos + 1, state, enabledPos,
                    state.length - enabledPos - 1);
        }

        return state;
    }

    @Override
    protected boolean canAnimate() {
        return (super.canAnimate() && mItemCount > 0);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        final boolean drawSelectorOnTop = mDrawSelectorOnTop;
        if (!drawSelectorOnTop) {
            drawSelector(canvas);
        }

        super.dispatchDraw(canvas);

        if (drawSelectorOnTop) {
            drawSelector(canvas);
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        boolean needsInvalidate = false;

        if (mStartEdge != null) {
            needsInvalidate |= drawStartEdge(canvas);
        }

        if (mEndEdge != null) {
            needsInvalidate |= drawEndEdge(canvas);
        }

        if (needsInvalidate) {
            ViewCompat.postInvalidateOnAnimation(this);
        }
    }

    @Override
    public void requestLayout() {
        if (!mInLayout && !mBlockLayoutRequests) {
            super.requestLayout();
        }
    }

    @Override
    public View getSelectedView() {
        if (mItemCount > 0 && mSelectedPosition >= 0) {
            return getChildAt(mSelectedPosition - mFirstPosition);
        } else {
            return null;
        }
    }

    @Override
    public void setSelection(int position) {
        setSelectionFromOffset(position, 0);
    }

    public void setSelectionFromOffset(int position, int offset) {
        if (mAdapter == null) {
            return;
        }

        if (!isInTouchMode()) {
            position = lookForSelectablePosition(position);
            if (position >= 0) {
                setNextSelectedPositionInt(position);
            }
        } else {
            mResurrectToPosition = position;
        }

        if (position >= 0) {
            mLayoutMode = LAYOUT_SPECIFIC;

            if (mIsVertical) {
                mSpecificStart = getPaddingTop() + offset;
            } else {
                mSpecificStart = getPaddingLeft() + offset;
            }

            if (mNeedSync) {
                mSyncPosition = position;
                mSyncRowId = mAdapter.getItemId(position);
            }

            requestLayout();
        }
    }

    @Override
    protected void dispatchSetPressed(boolean pressed) {
        
        
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (mSelector == null) {
            useDefaultSelector();
        }

        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int widthSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);

        int childWidth = 0;
        int childHeight = 0;

        mItemCount = (mAdapter == null ? 0 : mAdapter.getCount());
        if (mItemCount > 0 && (widthMode == MeasureSpec.UNSPECIFIED ||
                heightMode == MeasureSpec.UNSPECIFIED)) {
            final View child = obtainView(0, mIsScrap);

            final int secondaryMeasureSpec =
                    (mIsVertical ? widthMeasureSpec : heightMeasureSpec);

            measureScrapChild(child, 0, secondaryMeasureSpec);

            childWidth = child.getMeasuredWidth();
            childHeight = child.getMeasuredHeight();

            if (recycleOnMeasure()) {
                mRecycler.addScrapView(child, -1);
            }
        }

        if (widthMode == MeasureSpec.UNSPECIFIED) {
            widthSize = getPaddingLeft() + getPaddingRight() + childWidth;
            if (mIsVertical) {
                widthSize += getVerticalScrollbarWidth();
            }
        }

        if (heightMode == MeasureSpec.UNSPECIFIED) {
            heightSize = getPaddingTop() + getPaddingBottom() + childHeight;
            if (!mIsVertical) {
                heightSize += getHorizontalScrollbarHeight();
            }
        }

        if (mIsVertical && heightMode == MeasureSpec.AT_MOST) {
            heightSize = measureHeightOfChildren(widthMeasureSpec, 0, NO_POSITION, heightSize, -1);
        }

        if (!mIsVertical && widthMode == MeasureSpec.AT_MOST) {
            widthSize = measureWidthOfChildren(heightMeasureSpec, 0, NO_POSITION, widthSize, -1);
        }

        setMeasuredDimension(widthSize, heightSize);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        mInLayout = true;

        if (changed) {
            final int childCount = getChildCount();
            for (int i = 0; i < childCount; i++) {
                getChildAt(i).forceLayout();
            }

            mRecycler.markChildrenDirty();
        }

        layoutChildren();

        mInLayout = false;

        final int width = r - l - getPaddingLeft() - getPaddingRight();
        final int height = b - t - getPaddingTop() - getPaddingBottom();

        if (mStartEdge != null && mEndEdge != null) {
            if (mIsVertical) {
                mStartEdge.setSize(width, height);
                mEndEdge.setSize(width, height);
            } else {
                mStartEdge.setSize(height, width);
                mEndEdge.setSize(height, width);
            }
        }
    }

    private void layoutChildren() {
        if (getWidth() == 0 || getHeight() == 0) {
            return;
        }

        final boolean blockLayoutRequests = mBlockLayoutRequests;
        if (!blockLayoutRequests) {
            mBlockLayoutRequests = true;
        } else {
            return;
        }

        try {
            invalidate();

            if (mAdapter == null) {
                resetState();
                return;
            }

            final int start = (mIsVertical ? getPaddingTop() : getPaddingLeft());
            final int end =
                    (mIsVertical ? getHeight() - getPaddingBottom() : getWidth() - getPaddingRight());

            int childCount = getChildCount();
            int index = 0;
            int delta = 0;

            View selected = null;
            View oldSelected = null;
            View newSelected = null;
            View oldFirstChild = null;

            switch (mLayoutMode) {
            case LAYOUT_SET_SELECTION:
                index = mNextSelectedPosition - mFirstPosition;
                if (index >= 0 && index < childCount) {
                    newSelected = getChildAt(index);
                }

                break;

            case LAYOUT_FORCE_TOP:
            case LAYOUT_FORCE_BOTTOM:
            case LAYOUT_SPECIFIC:
            case LAYOUT_SYNC:
                break;

            case LAYOUT_MOVE_SELECTION:
            default:
                
                index = mSelectedPosition - mFirstPosition;
                if (index >= 0 && index < childCount) {
                    oldSelected = getChildAt(index);
                }

                
                oldFirstChild = getChildAt(0);

                if (mNextSelectedPosition >= 0) {
                    delta = mNextSelectedPosition - mSelectedPosition;
                }

                
                newSelected = getChildAt(index + delta);
            }

            final boolean dataChanged = mDataChanged;
            if (dataChanged) {
                handleDataChanged();
            }

            
            
            if (mItemCount == 0) {
                resetState();
                return;
            } else if (mItemCount != mAdapter.getCount()) {
                throw new IllegalStateException("The content of the adapter has changed but "
                        + "TwoWayView did not receive a notification. Make sure the content of "
                        + "your adapter is not modified from a background thread, but only "
                        + "from the UI thread. [in TwoWayView(" + getId() + ", " + getClass()
                        + ") with Adapter(" + mAdapter.getClass() + ")]");
            }

            setSelectedPositionInt(mNextSelectedPosition);

            
            
            final int firstPosition = mFirstPosition;
            final RecycleBin recycleBin = mRecycler;

            if (dataChanged) {
                for (int i = 0; i < childCount; i++) {
                    recycleBin.addScrapView(getChildAt(i), firstPosition + i);
                }
            } else {
                recycleBin.fillActiveViews(childCount, firstPosition);
            }

            detachAllViewsFromParent();

            switch (mLayoutMode) {
            case LAYOUT_SET_SELECTION:
                if (newSelected != null) {
                    final int newSelectedStart =
                            (mIsVertical ? newSelected.getTop() : newSelected.getLeft());

                    selected = fillFromSelection(newSelectedStart, start, end);
                } else {
                    selected = fillFromMiddle(start, end);
                }

                break;

            case LAYOUT_SYNC:
                selected = fillSpecific(mSyncPosition, mSpecificStart);
                break;

            case LAYOUT_FORCE_BOTTOM:
                selected = fillBefore(mItemCount - 1, end);
                adjustViewsStartOrEnd();
                break;

            case LAYOUT_FORCE_TOP:
                mFirstPosition = 0;
                selected = fillFromOffset(start);
                adjustViewsStartOrEnd();
                break;

            case LAYOUT_SPECIFIC:
                selected = fillSpecific(reconcileSelectedPosition(), mSpecificStart);
                break;

            case LAYOUT_MOVE_SELECTION:
                selected = moveSelection(oldSelected, newSelected, delta, start, end);
                break;

            default:
                if (childCount == 0) {
                    final int position = lookForSelectablePosition(0);
                    setSelectedPositionInt(position);
                    selected = fillFromOffset(start);
                } else {
                    if (mSelectedPosition >= 0 && mSelectedPosition < mItemCount) {
                        int offset = start;
                        if (oldSelected != null) {
                            offset = (mIsVertical ? oldSelected.getTop() : oldSelected.getLeft());
                        }
                        selected = fillSpecific(mSelectedPosition, offset);
                    } else if (mFirstPosition < mItemCount) {
                        int offset = start;
                        if (oldFirstChild != null) {
                            offset = (mIsVertical ? oldFirstChild.getTop() : oldFirstChild.getLeft());
                        }

                        selected = fillSpecific(mFirstPosition, offset);
                    } else {
                        selected = fillSpecific(0, start);
                    }
                }

                break;

            }

            recycleBin.scrapActiveViews();

            if (selected != null) {
                positionSelector(INVALID_POSITION, selected);
                mSelectedStart = (mIsVertical ? selected.getTop() : selected.getLeft());
            } else {
                if (mTouchMode > TOUCH_MODE_DOWN && mTouchMode < TOUCH_MODE_DRAGGING) {
                    View child = getChildAt(mMotionPosition - mFirstPosition);

                    if (child != null) {
                        positionSelector(mMotionPosition, child);
                    }
                } else {
                    mSelectedStart = 0;
                    mSelectorRect.setEmpty();
                }
            }

            mLayoutMode = LAYOUT_NORMAL;
            mDataChanged = false;
            mNeedSync = false;

            setNextSelectedPositionInt(mSelectedPosition);
            if (mItemCount > 0) {
                checkSelectionChanged();
            }

            invokeOnItemScrollListener();
        } finally {
            if (!blockLayoutRequests) {
                mBlockLayoutRequests = false;
                mDataChanged = false;
            }
        }
    }

    protected boolean recycleOnMeasure() {
        return true;
    }

    private void offsetChildren(int offset) {
        final int childCount = getChildCount();

        for (int i = 0; i < childCount; i++) {
            final View child = getChildAt(i);

            if (mIsVertical) {
                child.offsetTopAndBottom(offset);
            } else {
                child.offsetLeftAndRight(offset);
            }
        }
    }

    private View moveSelection(View oldSelected, View newSelected, int delta, int start,
            int end) {
        final int selectedPosition = mSelectedPosition;

        final int oldSelectedStart = (mIsVertical ? oldSelected.getTop() : oldSelected.getLeft());
        final int oldSelectedEnd = (mIsVertical ? oldSelected.getBottom() : oldSelected.getRight());

        View selected = null;

        if (delta > 0) {
            



            















            
            oldSelected = makeAndAddView(selectedPosition - 1, oldSelectedStart, true, false);

            final int itemMargin = mItemMargin;

            
            selected = makeAndAddView(selectedPosition, oldSelectedEnd + itemMargin, true, true);

            final int selectedStart = (mIsVertical ? selected.getTop() : selected.getLeft());
            final int selectedEnd = (mIsVertical ? selected.getBottom() : selected.getRight());

            
            if (selectedEnd > end) {
                
                final int spaceBefore = selectedStart - start;

                
                final int spaceAfter = selectedEnd - end;

                
                final int halfSpace = (end - start) / 2;
                int offset = Math.min(spaceBefore, spaceAfter);
                offset = Math.min(offset, halfSpace);

                if (mIsVertical) {
                    oldSelected.offsetTopAndBottom(-offset);
                    selected.offsetTopAndBottom(-offset);
                } else {
                    oldSelected.offsetLeftAndRight(-offset);
                    selected.offsetLeftAndRight(-offset);
                }
            }

            
            fillBefore(mSelectedPosition - 2, selectedStart - itemMargin);
            adjustViewsStartOrEnd();
            fillAfter(mSelectedPosition + 1, selectedEnd + itemMargin);
        } else if (delta < 0) {
            



            















            if (newSelected != null) {
                
                final int newSelectedStart = (mIsVertical ? newSelected.getTop() : newSelected.getLeft());
                selected = makeAndAddView(selectedPosition, newSelectedStart, true, true);
            } else {
                
                
                selected = makeAndAddView(selectedPosition, oldSelectedStart, false, true);
            }

            final int selectedStart = (mIsVertical ? selected.getTop() : selected.getLeft());
            final int selectedEnd = (mIsVertical ? selected.getBottom() : selected.getRight());

            
            if (selectedStart < start) {
                
                final int spaceBefore = start - selectedStart;

               
                final int spaceAfter = end - selectedEnd;

                
                final int halfSpace = (end - start) / 2;
                int offset = Math.min(spaceBefore, spaceAfter);
                offset = Math.min(offset, halfSpace);

                if (mIsVertical) {
                    selected.offsetTopAndBottom(offset);
                } else {
                    selected.offsetLeftAndRight(offset);
                }
            }

            
            fillBeforeAndAfter(selected, selectedPosition);
        } else {
            



            selected = makeAndAddView(selectedPosition, oldSelectedStart, true, true);

            final int selectedStart = (mIsVertical ? selected.getTop() : selected.getLeft());
            final int selectedEnd = (mIsVertical ? selected.getBottom() : selected.getRight());

            
            if (oldSelectedStart < start) {
                
                
                int newEnd = selectedEnd;
                if (newEnd < start + 20) {
                    
                    if (mIsVertical) {
                        selected.offsetTopAndBottom(start - selectedStart);
                    } else {
                        selected.offsetLeftAndRight(start - selectedStart);
                    }
                }
            }

            
            fillBeforeAndAfter(selected, selectedPosition);
        }

        return selected;
    }

    void confirmCheckedPositionsById() {
        
        mCheckStates.clear();

        for (int checkedIndex = 0; checkedIndex < mCheckedIdStates.size(); checkedIndex++) {
            final long id = mCheckedIdStates.keyAt(checkedIndex);
            final int lastPos = mCheckedIdStates.valueAt(checkedIndex);

            final long lastPosId = mAdapter.getItemId(lastPos);
            if (id != lastPosId) {
                
                final int start = Math.max(0, lastPos - CHECK_POSITION_SEARCH_DISTANCE);
                final int end = Math.min(lastPos + CHECK_POSITION_SEARCH_DISTANCE, mItemCount);
                boolean found = false;

                for (int searchPos = start; searchPos < end; searchPos++) {
                    final long searchId = mAdapter.getItemId(searchPos);
                    if (id == searchId) {
                        found = true;
                        mCheckStates.put(searchPos, true);
                        mCheckedIdStates.setValueAt(checkedIndex, searchPos);
                        break;
                    }
                }

                if (!found) {
                    mCheckedIdStates.delete(id);
                    checkedIndex--;
                    mCheckedItemCount--;
                }
            } else {
                mCheckStates.put(lastPos, true);
            }
        }
    }

    private void handleDataChanged() {
        if (mChoiceMode.compareTo(ChoiceMode.NONE) != 0 && mAdapter != null && mAdapter.hasStableIds()) {
            confirmCheckedPositionsById();
        }

        mRecycler.clearTransientStateViews();

        final int itemCount = mItemCount;
        if (itemCount > 0) {
            int newPos;
            int selectablePos;

            
            if (mNeedSync) {
                
                mNeedSync = false;
                mPendingSync = null;

                switch (mSyncMode) {
                case SYNC_SELECTED_POSITION:
                    if (isInTouchMode()) {
                        
                        
                        
                        
                        mLayoutMode = LAYOUT_SYNC;
                        mSyncPosition = Math.min(Math.max(0, mSyncPosition), itemCount - 1);

                        return;
                    } else {
                        
                        
                        newPos = findSyncPosition();
                        if (newPos >= 0) {
                            
                            selectablePos = lookForSelectablePosition(newPos, true);
                            if (selectablePos == newPos) {
                                
                                mSyncPosition = newPos;

                                if (mSyncHeight == getHeight()) {
                                    
                                    
                                    mLayoutMode = LAYOUT_SYNC;
                                } else {
                                    
                                    
                                    mLayoutMode = LAYOUT_SET_SELECTION;
                                }

                                
                                setNextSelectedPositionInt(newPos);
                                return;
                            }
                        }
                    }
                    break;

                case SYNC_FIRST_POSITION:
                    
                    mLayoutMode = LAYOUT_SYNC;
                    mSyncPosition = Math.min(Math.max(0, mSyncPosition), itemCount - 1);

                    return;
                }
            }

            if (!isInTouchMode()) {
                
                newPos = getSelectedItemPosition();

                
                if (newPos >= itemCount) {
                    newPos = itemCount - 1;
                }
                if (newPos < 0) {
                    newPos = 0;
                }

                
                selectablePos = lookForSelectablePosition(newPos, true);

                if (selectablePos >= 0) {
                    setNextSelectedPositionInt(selectablePos);
                    return;
                } else {
                    
                    selectablePos = lookForSelectablePosition(newPos, false);
                    if (selectablePos >= 0) {
                        setNextSelectedPositionInt(selectablePos);
                        return;
                    }
                }
            } else {
                
                if (mResurrectToPosition >= 0) {
                    return;
                }
            }
        }

        
        mLayoutMode = LAYOUT_FORCE_TOP;
        mSelectedPosition = INVALID_POSITION;
        mSelectedRowId = INVALID_ROW_ID;
        mNextSelectedPosition = INVALID_POSITION;
        mNextSelectedRowId = INVALID_ROW_ID;
        mNeedSync = false;
        mPendingSync = null;
        mSelectorPosition = INVALID_POSITION;

        checkSelectionChanged();

    }

    private int reconcileSelectedPosition() {
        int position = mSelectedPosition;
        if (position < 0) {
            position = mResurrectToPosition;
        }

        position = Math.max(0, position);
        position = Math.min(position, mItemCount - 1);

        return position;
    }

    boolean resurrectSelection() {
        final int childCount = getChildCount();
        if (childCount <= 0) {
            return false;
        }

        int selectedStart = 0;
        int selectedPosition;

        final int start = (mIsVertical ? getPaddingTop() : getPaddingLeft());
        final int end =
                (mIsVertical ? getHeight() - getPaddingBottom() : getWidth() - getPaddingRight());

        final int firstPosition = mFirstPosition;
        final int toPosition = mResurrectToPosition;
        boolean down = true;

        if (toPosition >= firstPosition && toPosition < firstPosition + childCount) {
            selectedPosition = toPosition;

            final View selected = getChildAt(selectedPosition - mFirstPosition);
            selectedStart = (mIsVertical ? selected.getTop() : selected.getLeft());
        } else if (toPosition < firstPosition) {
            
            selectedPosition = firstPosition;

            for (int i = 0; i < childCount; i++) {
                final View child = getChildAt(i);
                final int childStart = (mIsVertical ? child.getTop() : child.getLeft());

                if (i == 0) {
                    
                    selectedStart = childStart;
                }

                if (childStart >= start) {
                    
                    selectedPosition = firstPosition + i;
                    selectedStart = childStart;
                    break;
                }
            }
        } else {
            selectedPosition = firstPosition + childCount - 1;
            down = false;

            for (int i = childCount - 1; i >= 0; i--) {
                final View child = getChildAt(i);
                final int childStart = (mIsVertical ? child.getTop() : child.getLeft());
                final int childEnd = (mIsVertical ? child.getBottom() : child.getRight());

                if (i == childCount - 1) {
                    selectedStart = childStart;
                }

                if (childEnd <= end) {
                    selectedPosition = firstPosition + i;
                    selectedStart = childStart;
                    break;
                }
            }
        }

        mResurrectToPosition = INVALID_POSITION;
        mTouchMode = TOUCH_MODE_REST;
        reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);

        mSpecificStart = selectedStart;

        selectedPosition = lookForSelectablePosition(selectedPosition, down);
        if (selectedPosition >= firstPosition && selectedPosition <= getLastVisiblePosition()) {
            mLayoutMode = LAYOUT_SPECIFIC;
            updateSelectorState();
            setSelectionInt(selectedPosition);
            invokeOnItemScrollListener();
        } else {
            selectedPosition = INVALID_POSITION;
        }

        return selectedPosition >= 0;
    }

    private int getChildWidthMeasureSpec(LayoutParams lp) {
        if (!mIsVertical && lp.width == LayoutParams.WRAP_CONTENT) {
            return MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        } else if (mIsVertical) {
            final int maxWidth = getWidth() - getPaddingLeft() - getPaddingRight();
            return MeasureSpec.makeMeasureSpec(maxWidth, MeasureSpec.EXACTLY);
        } else {
            return MeasureSpec.makeMeasureSpec(lp.width, MeasureSpec.EXACTLY);
        }
    }

    private int getChildHeightMeasureSpec(LayoutParams lp) {
        if (mIsVertical && lp.height == LayoutParams.WRAP_CONTENT) {
            return MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        } else if (!mIsVertical) {
            final int maxHeight = getHeight() - getPaddingTop() - getPaddingBottom();
            return MeasureSpec.makeMeasureSpec(maxHeight, MeasureSpec.EXACTLY);
        } else {
            return MeasureSpec.makeMeasureSpec(lp.height, MeasureSpec.EXACTLY);
        }
    }

    private void measureChild(View child, LayoutParams lp) {
        final int widthSpec = getChildWidthMeasureSpec(lp);
        final int heightSpec = getChildHeightMeasureSpec(lp);
        child.measure(widthSpec, heightSpec);
    }

    private void measureScrapChild(View scrapChild, int position, int secondaryMeasureSpec) {
        LayoutParams lp = (LayoutParams) scrapChild.getLayoutParams();
        if (lp == null) {
            lp = generateDefaultLayoutParams();
            scrapChild.setLayoutParams(lp);
        }

        lp.viewType = mAdapter.getItemViewType(position);
        lp.forceAdd = true;

        final int widthMeasureSpec;
        final int heightMeasureSpec;
        if (mIsVertical) {
            widthMeasureSpec = secondaryMeasureSpec;
            heightMeasureSpec = getChildHeightMeasureSpec(lp);
        } else {
            widthMeasureSpec = getChildWidthMeasureSpec(lp);
            heightMeasureSpec = secondaryMeasureSpec;
        }

        scrapChild.measure(widthMeasureSpec, heightMeasureSpec);
    }

    

























    private int measureHeightOfChildren(int widthMeasureSpec, int startPosition, int endPosition,
            final int maxHeight, int disallowPartialChildPosition) {

        final int paddingTop = getPaddingTop();
        final int paddingBottom = getPaddingBottom();

        final ListAdapter adapter = mAdapter;
        if (adapter == null) {
            return paddingTop + paddingBottom;
        }

        
        int returnedHeight = paddingTop + paddingBottom;
        final int itemMargin = mItemMargin;

        
        
        int prevHeightWithoutPartialChild = 0;
        int i;
        View child;

        
        endPosition = (endPosition == NO_POSITION) ? adapter.getCount() - 1 : endPosition;
        final RecycleBin recycleBin = mRecycler;
        final boolean shouldRecycle = recycleOnMeasure();
        final boolean[] isScrap = mIsScrap;

        for (i = startPosition; i <= endPosition; ++i) {
            child = obtainView(i, isScrap);

            measureScrapChild(child, i, widthMeasureSpec);

            if (i > 0) {
                
                returnedHeight += itemMargin;
            }

            
            if (shouldRecycle) {
                recycleBin.addScrapView(child, -1);
            }

            returnedHeight += child.getMeasuredHeight();

            if (returnedHeight >= maxHeight) {
                
                
                return (disallowPartialChildPosition >= 0) 
                            && (i > disallowPartialChildPosition) 
                            && (prevHeightWithoutPartialChild > 0) 
                            && (returnedHeight != maxHeight) 
                        ? prevHeightWithoutPartialChild
                        : maxHeight;
            }

            if ((disallowPartialChildPosition >= 0) && (i >= disallowPartialChildPosition)) {
                prevHeightWithoutPartialChild = returnedHeight;
            }
        }

        
        
        return returnedHeight;
    }

    

























    private int measureWidthOfChildren(int heightMeasureSpec, int startPosition, int endPosition,
            final int maxWidth, int disallowPartialChildPosition) {

        final int paddingLeft = getPaddingLeft();
        final int paddingRight = getPaddingRight();

        final ListAdapter adapter = mAdapter;
        if (adapter == null) {
            return paddingLeft + paddingRight;
        }

        
        int returnedWidth = paddingLeft + paddingRight;
        final int itemMargin = mItemMargin;

        
        
        int prevWidthWithoutPartialChild = 0;
        int i;
        View child;

        
        endPosition = (endPosition == NO_POSITION) ? adapter.getCount() - 1 : endPosition;
        final RecycleBin recycleBin = mRecycler;
        final boolean shouldRecycle = recycleOnMeasure();
        final boolean[] isScrap = mIsScrap;

        for (i = startPosition; i <= endPosition; ++i) {
            child = obtainView(i, isScrap);

            measureScrapChild(child, i, heightMeasureSpec);

            if (i > 0) {
                
                returnedWidth += itemMargin;
            }

            
            if (shouldRecycle) {
                recycleBin.addScrapView(child, -1);
            }

            returnedWidth += child.getMeasuredHeight();

            if (returnedWidth >= maxWidth) {
                
                
                return (disallowPartialChildPosition >= 0) 
                            && (i > disallowPartialChildPosition) 
                            && (prevWidthWithoutPartialChild > 0) 
                            && (returnedWidth != maxWidth) 
                        ? prevWidthWithoutPartialChild
                        : maxWidth;
            }

            if ((disallowPartialChildPosition >= 0) && (i >= disallowPartialChildPosition)) {
                prevWidthWithoutPartialChild = returnedWidth;
            }
        }

        
        
        return returnedWidth;
    }

    private View makeAndAddView(int position, int offset, boolean flow, boolean selected) {
        final int top;
        final int left;

        if (mIsVertical) {
            top = offset;
            left = getPaddingLeft();
        } else {
            top = getPaddingTop();
            left = offset;
        }

        if (!mDataChanged) {
            
            final View activeChild = mRecycler.getActiveView(position);
            if (activeChild != null) {
                
                
                setupChild(activeChild, position, top, left, flow, selected, true);

                return activeChild;
            }
        }

        
        final View child = obtainView(position, mIsScrap);

        
        setupChild(child, position, top, left, flow, selected, mIsScrap[0]);

        return child;
    }

    @TargetApi(11)
    private void setupChild(View child, int position, int top, int left,
            boolean flow, boolean selected, boolean recycled) {
        final boolean isSelected = selected && shouldShowSelector();
        final boolean updateChildSelected = isSelected != child.isSelected();
        final int touchMode = mTouchMode;

        final boolean isPressed = touchMode > TOUCH_MODE_DOWN && touchMode < TOUCH_MODE_DRAGGING &&
                mMotionPosition == position;

        final boolean updateChildPressed = isPressed != child.isPressed();
        final boolean needToMeasure = !recycled || updateChildSelected || child.isLayoutRequested();

        
        LayoutParams lp = (LayoutParams) child.getLayoutParams();
        if (lp == null) {
            lp = generateDefaultLayoutParams();
        }

        lp.viewType = mAdapter.getItemViewType(position);

        if (recycled && !lp.forceAdd) {
            attachViewToParent(child, (flow ? -1 : 0), lp);
        } else {
            lp.forceAdd = false;
            addViewInLayout(child, (flow ? -1 : 0), lp, true);
        }

        if (updateChildSelected) {
            child.setSelected(isSelected);
        }

        if (updateChildPressed) {
            child.setPressed(isPressed);
        }

        if (mChoiceMode.compareTo(ChoiceMode.NONE) != 0 && mCheckStates != null) {
            if (child instanceof Checkable) {
                ((Checkable) child).setChecked(mCheckStates.get(position));
            } else if (getContext().getApplicationInfo().targetSdkVersion
                    >= Build.VERSION_CODES.HONEYCOMB) {
                child.setActivated(mCheckStates.get(position));
            }
        }

        if (needToMeasure) {
            measureChild(child, lp);
        } else {
            cleanupLayoutState(child);
        }

        final int w = child.getMeasuredWidth();
        final int h = child.getMeasuredHeight();

        final int childTop = (mIsVertical && !flow ? top - h : top);
        final int childLeft = (!mIsVertical && !flow ? left - w : left);

        if (needToMeasure) {
            final int childRight = childLeft + w;
            final int childBottom = childTop + h;

            child.layout(childLeft, childTop, childRight, childBottom);
        } else {
            child.offsetLeftAndRight(childLeft - child.getLeft());
            child.offsetTopAndBottom(childTop - child.getTop());
        }
    }

    void fillGap(boolean down) {
        final int childCount = getChildCount();

        if (down) {
            final int paddingStart = (mIsVertical ? getPaddingTop() : getPaddingLeft());

            final int lastEnd;
            if (mIsVertical) {
                lastEnd = getChildAt(childCount - 1).getBottom();
            } else {
                lastEnd = getChildAt(childCount - 1).getRight();
            }

            final int offset = (childCount > 0 ? lastEnd + mItemMargin : paddingStart);
            fillAfter(mFirstPosition + childCount, offset);
            correctTooHigh(getChildCount());
        } else {
            final int end;
            final int firstStart;

            if (mIsVertical) {
                end = getHeight() - getPaddingBottom();
                firstStart = getChildAt(0).getTop();
            } else {
                end = getWidth() - getPaddingRight();
                firstStart = getChildAt(0).getLeft();
            }

            final int offset = (childCount > 0 ? firstStart - mItemMargin : end);
            fillBefore(mFirstPosition - 1, offset);
            correctTooLow(getChildCount());
        }
    }

    private View fillBefore(int pos, int nextOffset) {
        View selectedView = null;

        final int start = (mIsVertical ? getPaddingTop() : getPaddingLeft());

        while (nextOffset > start && pos >= 0) {
            boolean isSelected = (pos == mSelectedPosition);
            View child = makeAndAddView(pos, nextOffset, false, isSelected);

            if (mIsVertical) {
                nextOffset = child.getTop() - mItemMargin;
            } else {
                nextOffset = child.getLeft() - mItemMargin;
            }

            if (isSelected) {
                selectedView = child;
            }

            pos--;
        }

        mFirstPosition = pos + 1;

        return selectedView;
    }

    private View fillAfter(int pos, int nextOffset) {
        View selectedView = null;

        final int end =
                (mIsVertical ? getHeight() - getPaddingBottom() : getWidth() - getPaddingRight());

        while (nextOffset < end && pos < mItemCount) {
            boolean selected = (pos == mSelectedPosition);

            View child = makeAndAddView(pos, nextOffset, true, selected);

            if (mIsVertical) {
                nextOffset = child.getBottom() + mItemMargin;
            } else {
                nextOffset = child.getRight() + mItemMargin;
            }

            if (selected) {
                selectedView = child;
            }

            pos++;
        }

        return selectedView;
    }

    private View fillSpecific(int position, int top) {
        final boolean tempIsSelected = (position == mSelectedPosition);
        View temp = makeAndAddView(position, top, true, tempIsSelected);

        
        mFirstPosition = position;

        final int itemMargin = mItemMargin;

        final int offsetBefore;
        if (mIsVertical) {
            offsetBefore = temp.getTop() - itemMargin;
        } else {
            offsetBefore = temp.getLeft() - itemMargin;
        }
        final View before = fillBefore(position - 1, offsetBefore);

        
        adjustViewsStartOrEnd();

        final int offsetAfter;
        if (mIsVertical) {
            offsetAfter = temp.getBottom() + itemMargin;
        } else {
            offsetAfter = temp.getRight() + itemMargin;
        }
        final View after = fillAfter(position + 1, offsetAfter);

        final int childCount = getChildCount();
        if (childCount > 0) {
            correctTooHigh(childCount);
        }

        if (tempIsSelected) {
            return temp;
        } else if (before != null) {
            return before;
        } else {
            return after;
        }
    }

    private View fillFromOffset(int nextOffset) {
        mFirstPosition = Math.min(mFirstPosition, mSelectedPosition);
        mFirstPosition = Math.min(mFirstPosition, mItemCount - 1);

        if (mFirstPosition < 0) {
            mFirstPosition = 0;
        }

        return fillAfter(mFirstPosition, nextOffset);
    }

    private View fillFromMiddle(int start, int end) {
        final int size = end - start;
        int position = reconcileSelectedPosition();

        View selected = makeAndAddView(position, start, true, true);
        mFirstPosition = position;

        if (mIsVertical) {
            int selectedHeight = selected.getMeasuredHeight();
            if (selectedHeight <= size) {
                selected.offsetTopAndBottom((size - selectedHeight) / 2);
            }
        } else {
            int selectedWidth = selected.getMeasuredWidth();
            if (selectedWidth <= size) {
                selected.offsetLeftAndRight((size - selectedWidth) / 2);
            }
        }

        fillBeforeAndAfter(selected, position);
        correctTooHigh(getChildCount());

        return selected;
    }

    private void fillBeforeAndAfter(View selected, int position) {
        final int itemMargin = mItemMargin;

        final int offsetBefore;
        if (mIsVertical) {
            offsetBefore = selected.getTop() - itemMargin;
        } else {
            offsetBefore = selected.getLeft() - itemMargin;
        }

        fillBefore(position - 1, offsetBefore);

        adjustViewsStartOrEnd();

        final int offsetAfter;
        if (mIsVertical) {
            offsetAfter = selected.getBottom() + itemMargin;
        } else {
            offsetAfter = selected.getRight() + itemMargin;
        }

        fillAfter(position + 1, offsetAfter);
    }

    private View fillFromSelection(int selectedTop, int start, int end) {
        final int selectedPosition = mSelectedPosition;
        View selected;

        selected = makeAndAddView(selectedPosition, selectedTop, true, true);

        final int selectedStart = (mIsVertical ? selected.getTop() : selected.getLeft());
        final int selectedEnd = (mIsVertical ? selected.getBottom() : selected.getRight());

        
        if (selectedEnd > end) {
            
            
            final int spaceAbove = selectedStart - start;

            
            
            final int spaceBelow = selectedEnd - end;

            final int offset = Math.min(spaceAbove, spaceBelow);

            
            selected.offsetTopAndBottom(-offset);
        } else if (selectedStart < start) {
            
            
            final int spaceAbove = start - selectedStart;

            
            
            final int spaceBelow = end - selectedEnd;

            final int offset = Math.min(spaceAbove, spaceBelow);

            
            selected.offsetTopAndBottom(offset);
        }

        
        fillBeforeAndAfter(selected, selectedPosition);
        correctTooHigh(getChildCount());

        return selected;
    }

    private void correctTooHigh(int childCount) {
        
        
        int lastPosition = mFirstPosition + childCount - 1;
        if (lastPosition != mItemCount - 1 || childCount == 0) {
            return;
        }

        
        final View lastChild = getChildAt(childCount - 1);

        
        final int lastEnd;
        if (mIsVertical) {
            lastEnd = lastChild.getBottom();
        } else {
            lastEnd = lastChild.getRight();
        }

        
        final int start = (mIsVertical ? getPaddingTop() : getPaddingLeft());
        final int end =
                (mIsVertical ? getHeight() - getPaddingBottom() : getWidth() - getPaddingRight());

        
        
        int endOffset = end - lastEnd;

        View firstChild = getChildAt(0);
        int firstStart = (mIsVertical ? firstChild.getTop() : firstChild.getLeft());

        
        
        if (endOffset > 0 && (mFirstPosition > 0 || firstStart < start))  {
            if (mFirstPosition == 0) {
                
                endOffset = Math.min(endOffset, start - firstStart);
            }

            
            offsetChildren(endOffset);

            if (mFirstPosition > 0) {
                firstStart = (mIsVertical ? firstChild.getTop() : firstChild.getLeft());

                
                
                fillBefore(mFirstPosition - 1, firstStart - mItemMargin);

                
                adjustViewsStartOrEnd();
            }
        }
    }

    private void correctTooLow(int childCount) {
        
        
        if (mFirstPosition != 0 || childCount == 0) {
            return;
        }

        final View first = getChildAt(0);
        final int firstStart = (mIsVertical ? first.getTop() : first.getLeft());

        final int start = (mIsVertical ? getPaddingTop() : getPaddingLeft());

        final int end;
        if (mIsVertical) {
            end = getHeight() - getPaddingBottom();
        } else {
            end = getWidth() - getPaddingRight();
        }

        
        
        int startOffset = firstStart - start;

        View last = getChildAt(childCount - 1);
        int lastEnd = (mIsVertical ? last.getBottom() : last.getRight());

        int lastPosition = mFirstPosition + childCount - 1;

        
        
        
        if (startOffset > 0) {
            if (lastPosition < mItemCount - 1 || lastEnd > end)  {
                if (lastPosition == mItemCount - 1) {
                    
                    startOffset = Math.min(startOffset, lastEnd - end);
                }

                
                offsetChildren(-startOffset);

                if (lastPosition < mItemCount - 1) {
                    lastEnd = (mIsVertical ? last.getBottom() : last.getRight());

                    
                    
                    fillAfter(lastPosition + 1, lastEnd + mItemMargin);

                    
                    adjustViewsStartOrEnd();
                }
            } else if (lastPosition == mItemCount - 1) {
                adjustViewsStartOrEnd();
            }
        }
    }

    private void adjustViewsStartOrEnd() {
        if (getChildCount() == 0) {
            return;
        }

        final View firstChild = getChildAt(0);

        int delta;
        if (mIsVertical) {
            delta = firstChild.getTop() - getPaddingTop() - mItemMargin;
        } else {
            delta = firstChild.getLeft() - getPaddingLeft() - mItemMargin;
        }

        if (delta < 0) {
            
            delta = 0;
        }

        if (delta != 0) {
            offsetChildren(-delta);
        }
    }

    @TargetApi(14)
    private SparseBooleanArray cloneCheckStates() {
        if (mCheckStates == null) {
            return null;
        }

        SparseBooleanArray checkedStates;

        if (Build.VERSION.SDK_INT >= 14) {
            checkedStates = mCheckStates.clone();
        } else {
            checkedStates = new SparseBooleanArray();

            for (int i = 0; i < mCheckStates.size(); i++) {
                checkedStates.put(mCheckStates.keyAt(i), mCheckStates.valueAt(i));
            }
        }

        return checkedStates;
    }

    private int findSyncPosition() {
        int itemCount = mItemCount;

        if (itemCount == 0) {
            return INVALID_POSITION;
        }

        final long idToMatch = mSyncRowId;

        
        if (idToMatch == INVALID_ROW_ID) {
            return INVALID_POSITION;
        }

        
        int seed = mSyncPosition;
        seed = Math.max(0, seed);
        seed = Math.min(itemCount - 1, seed);

        long endTime = SystemClock.uptimeMillis() + SYNC_MAX_DURATION_MILLIS;

        long rowId;

        
        int first = seed;

        
        int last = seed;

        
        boolean next = false;

        
        boolean hitFirst;

        
        boolean hitLast;

        
        
        final ListAdapter adapter = mAdapter;
        if (adapter == null) {
            return INVALID_POSITION;
        }

        while (SystemClock.uptimeMillis() <= endTime) {
            rowId = adapter.getItemId(seed);
            if (rowId == idToMatch) {
                
                return seed;
            }

            hitLast = (last == itemCount - 1);
            hitFirst = (first == 0);

            if (hitLast && hitFirst) {
                
                break;
            }

            if (hitFirst || (next && !hitLast)) {
                
                last++;
                seed = last;

                
                next = false;
            } else if (hitLast || (!next && !hitFirst)) {
                
                first--;
                seed = first;

                
                next = true;
            }
        }

        return INVALID_POSITION;
    }

    View obtainView(int position, boolean[] isScrap) {
        isScrap[0] = false;
        View scrapView;

        scrapView = mRecycler.getTransientStateView(position);
        if (scrapView != null) {
            return scrapView;
        }

        scrapView = mRecycler.getScrapView(position);

        final View child;
        if (scrapView != null) {
            child = mAdapter.getView(position, scrapView, this);

            if (child != scrapView) {
                mRecycler.addScrapView(scrapView, position);
            } else {
                isScrap[0] = true;
            }
        } else {
            child = mAdapter.getView(position, null, this);
        }

        if (mHasStableIds) {
            LayoutParams lp = (LayoutParams) child.getLayoutParams();

            if (lp == null) {
                lp = generateDefaultLayoutParams();
            } else if (!checkLayoutParams(lp)) {
                lp = generateLayoutParams(lp);
            }

            lp.id = mAdapter.getItemId(position);

            child.setLayoutParams(lp);
        }

        return child;
    }

    void resetState() {
        removeAllViewsInLayout();

        mSelectedStart = 0;
        mFirstPosition = 0;
        mDataChanged = false;
        mNeedSync = false;
        mPendingSync = null;
        mOldSelectedPosition = INVALID_POSITION;
        mOldSelectedRowId = INVALID_ROW_ID;

        mOverScroll = 0;

        setSelectedPositionInt(INVALID_POSITION);
        setNextSelectedPositionInt(INVALID_POSITION);

        mSelectorPosition = INVALID_POSITION;
        mSelectorRect.setEmpty();

        invalidate();
    }

    private void rememberSyncState() {
        if (getChildCount() == 0) {
            return;
        }

        mNeedSync = true;

        if (mSelectedPosition >= 0) {
            View child = getChildAt(mSelectedPosition - mFirstPosition);

            mSyncRowId = mNextSelectedRowId;
            mSyncPosition = mNextSelectedPosition;

            if (child != null) {
                mSpecificStart = (mIsVertical ? child.getTop() : child.getLeft());
            }

            mSyncMode = SYNC_SELECTED_POSITION;
        } else {
            
            View child = getChildAt(0);
            ListAdapter adapter = getAdapter();

            if (mFirstPosition >= 0 && mFirstPosition < adapter.getCount()) {
                mSyncRowId = adapter.getItemId(mFirstPosition);
            } else {
                mSyncRowId = NO_ID;
            }

            mSyncPosition = mFirstPosition;

            if (child != null) {
                mSpecificStart = child.getTop();
            }

            mSyncMode = SYNC_FIRST_POSITION;
        }
    }

    private ContextMenuInfo createContextMenuInfo(View view, int position, long id) {
        return new AdapterContextMenuInfo(view, position, id);
    }

    @TargetApi(11)
    private void updateOnScreenCheckedViews() {
        final int firstPos = mFirstPosition;
        final int count = getChildCount();

        final boolean useActivated = getContext().getApplicationInfo().targetSdkVersion
                >= Build.VERSION_CODES.HONEYCOMB;

        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            final int position = firstPos + i;

            if (child instanceof Checkable) {
                ((Checkable) child).setChecked(mCheckStates.get(position));
            } else if (useActivated) {
                child.setActivated(mCheckStates.get(position));
            }
        }
    }

    @Override
    public boolean performItemClick(View view, int position, long id) {
        boolean checkedStateChanged = false;

        if (mChoiceMode.compareTo(ChoiceMode.MULTIPLE) == 0) {
            boolean checked = !mCheckStates.get(position, false);
            mCheckStates.put(position, checked);

            if (mCheckedIdStates != null && mAdapter.hasStableIds()) {
                if (checked) {
                    mCheckedIdStates.put(mAdapter.getItemId(position), position);
                } else {
                    mCheckedIdStates.delete(mAdapter.getItemId(position));
                }
            }

            if (checked) {
                mCheckedItemCount++;
            } else {
                mCheckedItemCount--;
            }

            checkedStateChanged = true;
        } else if (mChoiceMode.compareTo(ChoiceMode.SINGLE) == 0) {
            boolean checked = !mCheckStates.get(position, false);
            if (checked) {
                mCheckStates.clear();
                mCheckStates.put(position, true);

                if (mCheckedIdStates != null && mAdapter.hasStableIds()) {
                    mCheckedIdStates.clear();
                    mCheckedIdStates.put(mAdapter.getItemId(position), position);
                }

                mCheckedItemCount = 1;
            } else if (mCheckStates.size() == 0 || !mCheckStates.valueAt(0)) {
                mCheckedItemCount = 0;
            }

            checkedStateChanged = true;
        }

        if (checkedStateChanged) {
            updateOnScreenCheckedViews();
        }

        return super.performItemClick(view, position, id);
    }

    private boolean performLongPress(final View child,
            final int longPressPosition, final long longPressId) {
        
        boolean handled = false;

        OnItemLongClickListener listener = getOnItemLongClickListener();
        if (listener != null) {
            handled = listener.onItemLongClick(TwoWayView.this, child,
                    longPressPosition, longPressId);
        }

        if (!handled) {
            mContextMenuInfo = createContextMenuInfo(child, longPressPosition, longPressId);
            handled = super.showContextMenuForChild(TwoWayView.this);
        }

        if (handled) {
            performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        }

        return handled;
    }

    @Override
    protected LayoutParams generateDefaultLayoutParams() {
        if (mIsVertical) {
            return new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        } else {
            return new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.MATCH_PARENT);
        }
    }

    @Override
    protected LayoutParams generateLayoutParams(ViewGroup.LayoutParams lp) {
        return new LayoutParams(lp);
    }

    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams lp) {
        return lp instanceof LayoutParams;
    }

    @Override
    public ViewGroup.LayoutParams generateLayoutParams(AttributeSet attrs) {
        return new LayoutParams(getContext(), attrs);
    }

    @Override
    protected ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    @Override
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);

        if (mPendingSync != null) {
            ss.selectedId = mPendingSync.selectedId;
            ss.firstId = mPendingSync.firstId;
            ss.viewStart = mPendingSync.viewStart;
            ss.position = mPendingSync.position;
            ss.height = mPendingSync.height;

            return ss;
        }

        boolean haveChildren = (getChildCount() > 0 && mItemCount > 0);
        long selectedId = getSelectedItemId();
        ss.selectedId = selectedId;
        ss.height = getHeight();

        if (selectedId >= 0) {
            ss.viewStart = mSelectedStart;
            ss.position = getSelectedItemPosition();
            ss.firstId = INVALID_POSITION;
        } else if (haveChildren && mFirstPosition > 0) {
            
            
            
            
            
            
            
            
            
            
            

            View child = getChildAt(0);
            ss.viewStart = (mIsVertical ? child.getTop() : child.getLeft());

            int firstPos = mFirstPosition;
            if (firstPos >= mItemCount) {
                firstPos = mItemCount - 1;
            }

            ss.position = firstPos;
            ss.firstId = mAdapter.getItemId(firstPos);
        } else {
            ss.viewStart = 0;
            ss.firstId = INVALID_POSITION;
            ss.position = 0;
        }

        if (mCheckStates != null) {
            ss.checkState = cloneCheckStates();
        }

        if (mCheckedIdStates != null) {
            final LongSparseArray<Integer> idState = new LongSparseArray<Integer>();

            final int count = mCheckedIdStates.size();
            for (int i = 0; i < count; i++) {
                idState.put(mCheckedIdStates.keyAt(i), mCheckedIdStates.valueAt(i));
            }

            ss.checkIdState = idState;
        }

        ss.checkedItemCount = mCheckedItemCount;

        return ss;
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());

        mDataChanged = true;
        mSyncHeight = ss.height;

        if (ss.selectedId >= 0) {
            mNeedSync = true;
            mPendingSync = ss;
            mSyncRowId = ss.selectedId;
            mSyncPosition = ss.position;
            mSpecificStart = ss.viewStart;
            mSyncMode = SYNC_SELECTED_POSITION;
        } else if (ss.firstId >= 0) {
            setSelectedPositionInt(INVALID_POSITION);

            
            setNextSelectedPositionInt(INVALID_POSITION);

            mSelectorPosition = INVALID_POSITION;
            mNeedSync = true;
            mPendingSync = ss;
            mSyncRowId = ss.firstId;
            mSyncPosition = ss.position;
            mSpecificStart = ss.viewStart;
            mSyncMode = SYNC_FIRST_POSITION;
        }

        if (ss.checkState != null) {
            mCheckStates = ss.checkState;
        }

        if (ss.checkIdState != null) {
            mCheckedIdStates = ss.checkIdState;
        }

        mCheckedItemCount = ss.checkedItemCount;

        requestLayout();
    }

    public static class LayoutParams extends ViewGroup.LayoutParams {
        


        int viewType;

        


        long id = -1;

        




        int scrappedFromPosition;

        







        boolean forceAdd;

        public LayoutParams(int width, int height) {
            super(width, height);

            if (this.width == MATCH_PARENT) {
                Log.w(LOGTAG, "Constructing LayoutParams with width FILL_PARENT " +
                        "does not make much sense as the view might change orientation. " +
                        "Falling back to WRAP_CONTENT");
                this.width = WRAP_CONTENT;
            }

            if (this.height == MATCH_PARENT) {
                Log.w(LOGTAG, "Constructing LayoutParams with height FILL_PARENT " +
                        "does not make much sense as the view might change orientation. " +
                        "Falling back to WRAP_CONTENT");
                this.height = WRAP_CONTENT;
            }
        }

        public LayoutParams(Context c, AttributeSet attrs) {
            super(c, attrs);

            if (this.width == MATCH_PARENT) {
                Log.w(LOGTAG, "Inflation setting LayoutParams width to MATCH_PARENT - " +
                        "does not make much sense as the view might change orientation. " +
                        "Falling back to WRAP_CONTENT");
                this.width = MATCH_PARENT;
            }

            if (this.height == MATCH_PARENT) {
                Log.w(LOGTAG, "Inflation setting LayoutParams height to MATCH_PARENT - " +
                        "does not make much sense as the view might change orientation. " +
                        "Falling back to WRAP_CONTENT");
                this.height = WRAP_CONTENT;
            }
        }

        public LayoutParams(ViewGroup.LayoutParams other) {
            super(other);

            if (this.width == MATCH_PARENT) {
                Log.w(LOGTAG, "Constructing LayoutParams with height MATCH_PARENT - " +
                        "does not make much sense as the view might change orientation. " +
                        "Falling back to WRAP_CONTENT");
                this.width = WRAP_CONTENT;
            }

            if (this.height == MATCH_PARENT) {
                Log.w(LOGTAG, "Constructing LayoutParams with height MATCH_PARENT - " +
                        "does not make much sense as the view might change orientation. " +
                        "Falling back to WRAP_CONTENT");
                this.height = WRAP_CONTENT;
            }
        }
    }

    class RecycleBin {
        private RecyclerListener mRecyclerListener;
        private int mFirstActivePosition;
        private View[] mActiveViews = new View[0];
        private ArrayList<View>[] mScrapViews;
        private int mViewTypeCount;
        private ArrayList<View> mCurrentScrap;
        private SparseArrayCompat<View> mTransientStateViews;

        public void setViewTypeCount(int viewTypeCount) {
            if (viewTypeCount < 1) {
                throw new IllegalArgumentException("Can't have a viewTypeCount < 1");
            }

            @SuppressWarnings({"unchecked", "rawtypes"})
            ArrayList<View>[] scrapViews = new ArrayList[viewTypeCount];
            for (int i = 0; i < viewTypeCount; i++) {
                scrapViews[i] = new ArrayList<View>();
            }

            mViewTypeCount = viewTypeCount;
            mCurrentScrap = scrapViews[0];
            mScrapViews = scrapViews;
        }

        public void markChildrenDirty() {
            if (mViewTypeCount == 1) {
                final ArrayList<View> scrap = mCurrentScrap;
                final int scrapCount = scrap.size();

                for (int i = 0; i < scrapCount; i++) {
                    scrap.get(i).forceLayout();
                }
            } else {
                final int typeCount = mViewTypeCount;
                for (int i = 0; i < typeCount; i++) {
                    final ArrayList<View> scrap = mScrapViews[i];
                    final int scrapCount = scrap.size();

                    for (int j = 0; j < scrapCount; j++) {
                        scrap.get(j).forceLayout();
                    }
                }
            }

            if (mTransientStateViews != null) {
                final int count = mTransientStateViews.size();
                for (int i = 0; i < count; i++) {
                    mTransientStateViews.valueAt(i).forceLayout();
                }
            }
        }

        public boolean shouldRecycleViewType(int viewType) {
            return viewType >= 0;
        }

        void clear() {
            if (mViewTypeCount == 1) {
                final ArrayList<View> scrap = mCurrentScrap;
                final int scrapCount = scrap.size();

                for (int i = 0; i < scrapCount; i++) {
                    removeDetachedView(scrap.remove(scrapCount - 1 - i), false);
                }
            } else {
                final int typeCount = mViewTypeCount;
                for (int i = 0; i < typeCount; i++) {
                    final ArrayList<View> scrap = mScrapViews[i];
                    final int scrapCount = scrap.size();

                    for (int j = 0; j < scrapCount; j++) {
                        removeDetachedView(scrap.remove(scrapCount - 1 - j), false);
                    }
                }
            }

            if (mTransientStateViews != null) {
                mTransientStateViews.clear();
            }
        }

        void fillActiveViews(int childCount, int firstActivePosition) {
            if (mActiveViews.length < childCount) {
                mActiveViews = new View[childCount];
            }

            mFirstActivePosition = firstActivePosition;

            final View[] activeViews = mActiveViews;
            for (int i = 0; i < childCount; i++) {
                View child = getChildAt(i);

                
                
                activeViews[i] = child;
            }
        }

        View getActiveView(int position) {
            final int index = position - mFirstActivePosition;
            final View[] activeViews = mActiveViews;

            if (index >= 0 && index < activeViews.length) {
                final View match = activeViews[index];
                activeViews[index] = null;

                return match;
            }

            return null;
        }

        View getTransientStateView(int position) {
            if (mTransientStateViews == null) {
                return null;
            }

            final int index = mTransientStateViews.indexOfKey(position);
            if (index < 0) {
                return null;
            }

            final View result = mTransientStateViews.valueAt(index);
            mTransientStateViews.removeAt(index);

            return result;
        }

        void clearTransientStateViews() {
            if (mTransientStateViews != null) {
                mTransientStateViews.clear();
            }
        }

        View getScrapView(int position) {
            if (mViewTypeCount == 1) {
                return retrieveFromScrap(mCurrentScrap, position);
            } else {
                int whichScrap = mAdapter.getItemViewType(position);
                if (whichScrap >= 0 && whichScrap < mScrapViews.length) {
                    return retrieveFromScrap(mScrapViews[whichScrap], position);
                }
            }

            return null;
        }

        void addScrapView(View scrap, int position) {
            LayoutParams lp = (LayoutParams) scrap.getLayoutParams();
            if (lp == null) {
                return;
            }

            lp.scrappedFromPosition = position;

            final int viewType = lp.viewType;
            final boolean scrapHasTransientState = ViewCompat.hasTransientState(scrap);

            
            if (!shouldRecycleViewType(viewType) || scrapHasTransientState) {
                if (scrapHasTransientState) {
                    if (mTransientStateViews == null) {
                        mTransientStateViews = new SparseArrayCompat<View>();
                    }

                    mTransientStateViews.put(position, scrap);
                }

                return;
            }

            if (mViewTypeCount == 1) {
                mCurrentScrap.add(scrap);
            } else {
                mScrapViews[viewType].add(scrap);
            }

            if (mRecyclerListener != null) {
                mRecyclerListener.onMovedToScrapHeap(scrap);
            }
        }

        void scrapActiveViews() {
            final View[] activeViews = mActiveViews;
            final boolean multipleScraps = (mViewTypeCount > 1);

            ArrayList<View> scrapViews = mCurrentScrap;
            final int count = activeViews.length;

            for (int i = count - 1; i >= 0; i--) {
                final View victim = activeViews[i];
                if (victim != null) {
                    final LayoutParams lp = (LayoutParams) victim.getLayoutParams();
                    int whichScrap = lp.viewType;

                    activeViews[i] = null;

                    final boolean scrapHasTransientState = ViewCompat.hasTransientState(victim);
                    if (!shouldRecycleViewType(whichScrap) || scrapHasTransientState) {
                        if (scrapHasTransientState) {
                            removeDetachedView(victim, false);

                            if (mTransientStateViews == null) {
                                mTransientStateViews = new SparseArrayCompat<View>();
                            }

                            mTransientStateViews.put(mFirstActivePosition + i, victim);
                        }

                        continue;
                    }

                    if (multipleScraps) {
                        scrapViews = mScrapViews[whichScrap];
                    }

                    lp.scrappedFromPosition = mFirstActivePosition + i;
                    scrapViews.add(victim);

                    if (mRecyclerListener != null) {
                        mRecyclerListener.onMovedToScrapHeap(victim);
                    }
                }
            }

            pruneScrapViews();
        }

        private void pruneScrapViews() {
            final int maxViews = mActiveViews.length;
            final int viewTypeCount = mViewTypeCount;
            final ArrayList<View>[] scrapViews = mScrapViews;

            for (int i = 0; i < viewTypeCount; ++i) {
                final ArrayList<View> scrapPile = scrapViews[i];
                int size = scrapPile.size();
                final int extras = size - maxViews;

                size--;

                for (int j = 0; j < extras; j++) {
                    removeDetachedView(scrapPile.remove(size--), false);
                }
            }

            if (mTransientStateViews != null) {
                for (int i = 0; i < mTransientStateViews.size(); i++) {
                    final View v = mTransientStateViews.valueAt(i);
                    if (!ViewCompat.hasTransientState(v)) {
                        mTransientStateViews.removeAt(i);
                        i--;
                    }
                }
            }
        }

        void reclaimScrapViews(List<View> views) {
            if (mViewTypeCount == 1) {
                views.addAll(mCurrentScrap);
            } else {
                final int viewTypeCount = mViewTypeCount;
                final ArrayList<View>[] scrapViews = mScrapViews;

                for (int i = 0; i < viewTypeCount; ++i) {
                    final ArrayList<View> scrapPile = scrapViews[i];
                    views.addAll(scrapPile);
                }
            }
        }

        View retrieveFromScrap(ArrayList<View> scrapViews, int position) {
            int size = scrapViews.size();
            if (size <= 0) {
                return null;
            }

            for (int i = 0; i < size; i++) {
                final View scrapView = scrapViews.get(i);
                final LayoutParams lp = (LayoutParams) scrapView.getLayoutParams();

                if (lp.scrappedFromPosition == position) {
                    scrapViews.remove(i);
                    return scrapView;
                }
            }

            return scrapViews.remove(size - 1);
        }
    }

    @Override
    public void setEmptyView(View emptyView) {
        super.setEmptyView(emptyView);
        mEmptyView = emptyView;
        updateEmptyStatus();
    }

    private void updateEmptyStatus() {
        boolean isEmpty = mAdapter == null || mAdapter.isEmpty();
        if (isEmpty) {
            if (mEmptyView != null) {
                mEmptyView.setVisibility(View.VISIBLE);
                setVisibility(View.GONE);
            } else {
                
                
                setVisibility(View.VISIBLE);
            }

            
            
            
            if (mDataChanged) {
                this.onLayout(false, getLeft(), getTop(), getRight(), getBottom());
            }
        } else {
            if (mEmptyView != null)
                mEmptyView.setVisibility(View.GONE);
            setVisibility(View.VISIBLE);
        }
    }

    private class AdapterDataSetObserver extends DataSetObserver {
        private Parcelable mInstanceState = null;

        @Override
        public void onChanged() {
            mDataChanged = true;
            mOldItemCount = mItemCount;
            mItemCount = getAdapter().getCount();

            
            
            if (TwoWayView.this.mHasStableIds && mInstanceState != null
                    && mOldItemCount == 0 && mItemCount > 0) {
                TwoWayView.this.onRestoreInstanceState(mInstanceState);
                mInstanceState = null;
            } else {
                rememberSyncState();
            }
            updateEmptyStatus();
            requestLayout();
        }

        @Override
        public void onInvalidated() {
            mDataChanged = true;

            if (TwoWayView.this.mHasStableIds) {
                
                
                mInstanceState = TwoWayView.this.onSaveInstanceState();
            }

            
            mOldItemCount = mItemCount;
            mItemCount = 0;

            mSelectedPosition = INVALID_POSITION;
            mSelectedRowId = INVALID_ROW_ID;

            mNextSelectedPosition = INVALID_POSITION;
            mNextSelectedRowId = INVALID_ROW_ID;

            mNeedSync = false;

            updateEmptyStatus();
            requestLayout();
        }
    }

    static class SavedState extends BaseSavedState {
        long selectedId;
        long firstId;
        int viewStart;
        int position;
        int height;
        int checkedItemCount;
        SparseBooleanArray checkState;
        LongSparseArray<Integer> checkIdState;

        


        SavedState(Parcelable superState) {
            super(superState);
        }

        


        private SavedState(Parcel in) {
            super(in);

            selectedId = in.readLong();
            firstId = in.readLong();
            viewStart = in.readInt();
            position = in.readInt();
            height = in.readInt();

            checkedItemCount = in.readInt();
            checkState = in.readSparseBooleanArray();

            final int N = in.readInt();
            if (N > 0) {
                checkIdState = new LongSparseArray<Integer>();
                for (int i = 0; i < N; i++) {
                    final long key = in.readLong();
                    final int value = in.readInt();
                    checkIdState.put(key, value);
                }
            }
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);

            out.writeLong(selectedId);
            out.writeLong(firstId);
            out.writeInt(viewStart);
            out.writeInt(position);
            out.writeInt(height);

            out.writeInt(checkedItemCount);
            out.writeSparseBooleanArray(checkState);

            final int N = checkIdState != null ? checkIdState.size() : 0;
            out.writeInt(N);

            for (int i = 0; i < N; i++) {
                out.writeLong(checkIdState.keyAt(i));
                out.writeInt(checkIdState.valueAt(i));
            }
        }

        @Override
        public String toString() {
            return "TwoWayView.SavedState{"
                    + Integer.toHexString(System.identityHashCode(this))
                    + " selectedId=" + selectedId
                    + " firstId=" + firstId
                    + " viewStart=" + viewStart
                    + " height=" + height
                    + " position=" + position
                    + " checkState=" + checkState + "}";
        }

        public static final Parcelable.Creator<SavedState> CREATOR
                = new Parcelable.Creator<SavedState>() {
            @Override
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            @Override
            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }

    private class SelectionNotifier implements Runnable {
        @Override
        public void run() {
            if (mDataChanged) {
                
                
                
                if (mAdapter != null) {
                    post(this);
                }
            } else {
                fireOnSelected();
            }
        }
    }

    private class WindowRunnnable {
        private int mOriginalAttachCount;

        public void rememberWindowAttachCount() {
            mOriginalAttachCount = getWindowAttachCount();
        }

        public boolean sameWindow() {
            return hasWindowFocus() && getWindowAttachCount() == mOriginalAttachCount;
        }
    }

    private class PerformClick extends WindowRunnnable implements Runnable {
        int mClickMotionPosition;

        @Override
        public void run() {
            if (mDataChanged) {
                return;
            }

            final ListAdapter adapter = mAdapter;
            final int motionPosition = mClickMotionPosition;

            if (adapter != null && mItemCount > 0 &&
                motionPosition != INVALID_POSITION &&
                motionPosition < adapter.getCount() && sameWindow()) {

                final View child = getChildAt(motionPosition - mFirstPosition);
                if (child != null) {
                    performItemClick(child, motionPosition, adapter.getItemId(motionPosition));
                }
            }
        }
    }

    private final class CheckForTap implements Runnable {
        @Override
        public void run() {
            if (mTouchMode != TOUCH_MODE_DOWN) {
                return;
            }

            mTouchMode = TOUCH_MODE_TAP;

            final View child = getChildAt(mMotionPosition - mFirstPosition);
            if (child != null && !child.hasFocusable()) {
                mLayoutMode = LAYOUT_NORMAL;

                if (!mDataChanged) {
                    setPressed(true);
                    child.setPressed(true);

                    layoutChildren();
                    positionSelector(mMotionPosition, child);
                    refreshDrawableState();

                    positionSelector(mMotionPosition, child);
                    refreshDrawableState();

                    final boolean longClickable = isLongClickable();

                    if (mSelector != null) {
                        Drawable d = mSelector.getCurrent();

                        if (d != null && d instanceof TransitionDrawable) {
                            if (longClickable) {
                                final int longPressTimeout = ViewConfiguration.getLongPressTimeout();
                                ((TransitionDrawable) d).startTransition(longPressTimeout);
                            } else {
                                ((TransitionDrawable) d).resetTransition();
                            }
                        }
                    }

                    if (longClickable) {
                        triggerCheckForLongPress();
                    } else {
                        mTouchMode = TOUCH_MODE_DONE_WAITING;
                    }
                } else {
                    mTouchMode = TOUCH_MODE_DONE_WAITING;
                }
            }
        }
    }

    private class CheckForLongPress extends WindowRunnnable implements Runnable {
        @Override
        public void run() {
            final int motionPosition = mMotionPosition;
            final View child = getChildAt(motionPosition - mFirstPosition);

            if (child != null) {
                final long longPressId = mAdapter.getItemId(mMotionPosition);

                boolean handled = false;
                if (sameWindow() && !mDataChanged) {
                    handled = performLongPress(child, motionPosition, longPressId);
                }

                if (handled) {
                    mTouchMode = TOUCH_MODE_REST;
                    setPressed(false);
                    child.setPressed(false);
                } else {
                    mTouchMode = TOUCH_MODE_DONE_WAITING;
                }
            }
        }
    }
}
