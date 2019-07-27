




package org.mozilla.gecko.tabs;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.tabs.TabsPanel.TabsLayout;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.TwoWayView;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;

class TabsListLayout extends TwoWayView
                     implements TabsLayout,
                                Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "Gecko" + TabsListLayout.class.getSimpleName();

    private Context mContext;
    private TabsPanel mTabsPanel;

    final private boolean mIsPrivate;

    private TabsAdapter mTabsAdapter;

    private List<View> mPendingClosedTabs;
    private int mCloseAnimationCount;
    private int mCloseAllAnimationCount;

    private TabSwipeGestureListener mSwipeListener;

    
    private static final int ANIMATION_DURATION = 250;

    
    private static final int ANIMATION_CASCADE_DELAY = 75;

    private int mOriginalSize;

    public TabsListLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mPendingClosedTabs = new ArrayList<View>();

        setItemsCanFocus(true);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabsTray);
        mIsPrivate = (a.getInt(R.styleable.TabsTray_tabs, 0x0) == 1);
        a.recycle();

        mTabsAdapter = new TabsAdapter(mContext);
        setAdapter(mTabsAdapter);

        mSwipeListener = new TabSwipeGestureListener();
        setOnTouchListener(mSwipeListener);
        setOnScrollListener(mSwipeListener.makeScrollListener());

        setRecyclerListener(new RecyclerListener() {
            @Override
            public void onMovedToScrapHeap(View view) {
                TabsLayoutItemView item = (TabsLayoutItemView) view.getTag();
                item.thumbnail.setImageDrawable(null);
                item.close.setVisibility(View.VISIBLE);
            }
        });
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        mTabsPanel = panel;
    }

    @Override
    public void show() {
        setVisibility(View.VISIBLE);
        Tabs.getInstance().refreshThumbnails();
        Tabs.registerOnTabsChangedListener(this);
        refreshTabsData();
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
        Tabs.unregisterOnTabsChangedListener(this);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Screenshot:Cancel",""));
        mTabsAdapter.clear();
    }

    @Override
    public boolean shouldExpand() {
        return isVertical();
    }

    private void autoHidePanel() {
        mTabsPanel.autoHidePanel();
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch (msg) {
            case ADDED:
                
                refreshTabsData();
                break;

            case CLOSED:
               if (tab.isPrivate() == mIsPrivate && mTabsAdapter.getCount() > 0) {
                   if (mTabsAdapter.removeTab(tab)) {
                       int selected = mTabsAdapter.getPositionForTab(Tabs.getInstance().getSelectedTab());
                       updateSelectedStyle(selected);
                   }
               }
               break;

            case SELECTED:
                
                updateSelectedPosition();
            case UNSELECTED:
                
            case THUMBNAIL:
            case TITLE:
            case RECORDING_CHANGE:
                View view = getChildAt(mTabsAdapter.getPositionForTab(tab) - getFirstVisiblePosition());
                if (view == null)
                    return;

                TabsLayoutItemView item = (TabsLayoutItemView) view.getTag();
                item.assignValues(tab);
                break;
        }
    }

    
    private void updateSelectedPosition() {
        int selected = mTabsAdapter.getPositionForTab(Tabs.getInstance().getSelectedTab());
        updateSelectedStyle(selected);

        if (selected != -1) {
            setSelection(selected);
        }
    }

    




    private void updateSelectedStyle(int selected) {
        for (int i = 0; i < mTabsAdapter.getCount(); i++) {
            setItemChecked(i, (i == selected));
        }
    }

    private void refreshTabsData() {
        
        
        ArrayList<Tab> tabData = new ArrayList<Tab>();

        Iterable<Tab> allTabs = Tabs.getInstance().getTabsInOrder();
        for (Tab tab : allTabs) {
            if (tab.isPrivate() == mIsPrivate)
                tabData.add(tab);
        }

        mTabsAdapter.setTabs(tabData);
        updateSelectedPosition();
    }

    
    private class TabsAdapter extends BaseAdapter {
        private Context mContext;
        private ArrayList<Tab> mTabs;
        private LayoutInflater mInflater;
        private Button.OnClickListener mOnCloseClickListener;

        public TabsAdapter(Context context) {
            mContext = context;
            mInflater = LayoutInflater.from(mContext);

            mOnCloseClickListener = new Button.OnClickListener() {
                @Override
                public void onClick(View v) {
                    TabsLayoutItemView tab = (TabsLayoutItemView) v.getTag();
                    final int pos = (isVertical() ? tab.info.getWidth() : 0 - tab.info.getHeight());
                    animateClose(tab.info, pos);
                }
            };
        }

        public void setTabs (ArrayList<Tab> tabs) {
            mTabs = tabs;
            notifyDataSetChanged(); 
        }

        public boolean removeTab (Tab tab) {
            boolean tabRemoved = mTabs.remove(tab);
            if (tabRemoved) {
                notifyDataSetChanged(); 
            }
            return tabRemoved;
        }

        public void clear() {
            mTabs = null;
            notifyDataSetChanged(); 
        }

        @Override
        public int getCount() {
            return (mTabs == null ? 0 : mTabs.size());
        }

        @Override
        public Tab getItem(int position) {
            return mTabs.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        private int getPositionForTab(Tab tab) {
            if (mTabs == null || tab == null)
                return -1;

            return mTabs.indexOf(tab);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            TabsLayoutItemView item;

            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.tabs_row, null);
                item = new TabsLayoutItemView(convertView);
                item.close.setOnClickListener(mOnCloseClickListener);
                convertView.setTag(item);
            } else {
                item = (TabsLayoutItemView) convertView.getTag();
                
                
                resetTransforms(convertView);
            }

            Tab tab = mTabs.get(position);
            item.assignValues(tab);

            return convertView;
        }
    }

    private void resetTransforms(View view) {
        ViewHelper.setAlpha(view, 1);

        if (isVertical()) {
            ViewHelper.setTranslationX(view, 0);
        } else {
            ViewHelper.setTranslationY(view, 0);
        }

        
        if (mOriginalSize != 0) {
            if (isVertical()) {
                ViewHelper.setHeight(view, mOriginalSize);
            } else {
                ViewHelper.setWidth(view, mOriginalSize);
            }
        }
    }

    private boolean isVertical() {
        return (getOrientation().compareTo(TwoWayView.Orientation.VERTICAL) == 0);
    }

    @Override
    public void closeAll() {
        final int childCount = getChildCount();

        
        if (childCount == 0) {
            autoHidePanel();
            return;
        }

        
        setEnabled(false);

        
        int cascadeDelay = 0;

        for (int i = childCount - 1; i >= 0; i--) {
            final View view = getChildAt(i);
            final PropertyAnimator animator = new PropertyAnimator(ANIMATION_DURATION);
            animator.attach(view, Property.ALPHA, 0);

            if (isVertical()) {
                animator.attach(view, Property.TRANSLATION_X, view.getWidth());
            } else {
                animator.attach(view, Property.TRANSLATION_Y, view.getHeight());
            }

            mCloseAllAnimationCount++;

            animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
                @Override
                public void onPropertyAnimationStart() { }

                @Override
                public void onPropertyAnimationEnd() {
                    mCloseAllAnimationCount--;
                    if (mCloseAllAnimationCount > 0) {
                        return;
                    }

                    
                    autoHidePanel();

                    
                    TabsListLayout.this.setEnabled(true);

                    
                    final Iterable<Tab> tabs = Tabs.getInstance().getTabsInOrder();
                    for (Tab tab : tabs) {
                        
                        
                        if (!mIsPrivate || tab.isPrivate()) {
                            Tabs.getInstance().closeTab(tab, false);
                        }
                    }
                }
            });

            ThreadUtils.getUiHandler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    animator.start();
                }
            }, cascadeDelay);

            cascadeDelay += ANIMATION_CASCADE_DELAY;
        }
    }

    private void animateClose(final View view, int pos) {
        PropertyAnimator animator = new PropertyAnimator(ANIMATION_DURATION);
        animator.attach(view, Property.ALPHA, 0);

        if (isVertical())
            animator.attach(view, Property.TRANSLATION_X, pos);
        else
            animator.attach(view, Property.TRANSLATION_Y, pos);

        mCloseAnimationCount++;
        mPendingClosedTabs.add(view);

        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() { }
            @Override
            public void onPropertyAnimationEnd() {
                mCloseAnimationCount--;
                if (mCloseAnimationCount > 0)
                    return;

                for (View pendingView : mPendingClosedTabs) {
                    animateFinishClose(pendingView);
                }

                mPendingClosedTabs.clear();
            }
        });

        if (mTabsAdapter.getCount() == 1)
            autoHidePanel();

        animator.start();
    }

    private void animateFinishClose(final View view) {
        PropertyAnimator animator = new PropertyAnimator(ANIMATION_DURATION);

        final boolean isVertical = isVertical();
        if (isVertical)
            animator.attach(view, Property.HEIGHT, 1);
        else
            animator.attach(view, Property.WIDTH, 1);

        TabsLayoutItemView tab = (TabsLayoutItemView)view.getTag();
        final int tabId = tab.id;

        
        if (mOriginalSize == 0) {
            mOriginalSize = (isVertical ? view.getHeight() : view.getWidth());
        }

        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() { }
            @Override
            public void onPropertyAnimationEnd() {
                Tabs tabs = Tabs.getInstance();
                Tab tab = tabs.getTab(tabId);
                tabs.closeTab(tab, true);
            }
        });

        animator.start();
    }

    private void animateCancel(final View view) {
        PropertyAnimator animator = new PropertyAnimator(ANIMATION_DURATION);
        animator.attach(view, Property.ALPHA, 1);

        if (isVertical())
            animator.attach(view, Property.TRANSLATION_X, 0);
        else
            animator.attach(view, Property.TRANSLATION_Y, 0);


        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() { }
            @Override
            public void onPropertyAnimationEnd() {
                TabsLayoutItemView tab = (TabsLayoutItemView) view.getTag();
                tab.close.setVisibility(View.VISIBLE);
            }
        });

        animator.start();
    }

    private class TabSwipeGestureListener implements View.OnTouchListener {
        
        
        private static final float MIN_VELOCITY = 750;

        private int mSwipeThreshold;
        private int mMinFlingVelocity;

        private int mMaxFlingVelocity;
        private VelocityTracker mVelocityTracker;

        private int mListWidth = 1;
        private int mListHeight = 1;

        private View mSwipeView;
        private Runnable mPendingCheckForTap;

        private float mSwipeStartX;
        private float mSwipeStartY;
        private boolean mSwiping;
        private boolean mEnabled;

        public TabSwipeGestureListener() {
            mEnabled = true;

            ViewConfiguration vc = ViewConfiguration.get(TabsListLayout.this.getContext());
            mSwipeThreshold = vc.getScaledTouchSlop();
            mMinFlingVelocity = (int) (getContext().getResources().getDisplayMetrics().density * MIN_VELOCITY);
            mMaxFlingVelocity = vc.getScaledMaximumFlingVelocity();
        }

        public void setEnabled(boolean enabled) {
            mEnabled = enabled;
        }

        public TwoWayView.OnScrollListener makeScrollListener() {
            return new TwoWayView.OnScrollListener() {
                @Override
                public void onScrollStateChanged(TwoWayView twoWayView, int scrollState) {
                    setEnabled(scrollState != TwoWayView.OnScrollListener.SCROLL_STATE_TOUCH_SCROLL);
                }

                @Override
                public void onScroll(TwoWayView twoWayView, int i, int i1, int i2) {
                }
            };
        }

        @Override
        public boolean onTouch(View view, MotionEvent e) {
            if (!mEnabled)
                return false;

            if (mListWidth < 2 || mListHeight < 2) {
                mListWidth = TabsListLayout.this.getWidth();
                mListHeight = TabsListLayout.this.getHeight();
            }

            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN: {
                    
                    
                    triggerCheckForTap();

                    final float x = e.getRawX();
                    final float y = e.getRawY();

                    
                    mSwipeView = findViewAt(x, y);

                    if (mSwipeView != null) {
                        mSwipeStartX = e.getRawX();
                        mSwipeStartY = e.getRawY();

                        mVelocityTracker = VelocityTracker.obtain();
                        mVelocityTracker.addMovement(e);
                    }

                    view.onTouchEvent(e);
                    return true;
                }

                case MotionEvent.ACTION_UP: {
                    if (mSwipeView == null)
                        break;

                    cancelCheckForTap();
                    mSwipeView.setPressed(false);

                    if (!mSwiping) {
                        TabsLayoutItemView tab = (TabsLayoutItemView) mSwipeView.getTag();
                        Tabs.getInstance().selectTab(tab.id);
                        autoHidePanel();

                        mVelocityTracker.recycle();
                        mVelocityTracker = null;
                        break;
                    }

                    mVelocityTracker.addMovement(e);
                    mVelocityTracker.computeCurrentVelocity(1000, mMaxFlingVelocity);

                    float velocityX = Math.abs(mVelocityTracker.getXVelocity());
                    float velocityY = Math.abs(mVelocityTracker.getYVelocity());

                    boolean dismiss = false;
                    boolean dismissDirection = false;
                    int dismissTranslation = 0;

                    if (isVertical()) {
                        float deltaX = ViewHelper.getTranslationX(mSwipeView);

                        if (Math.abs(deltaX) > mListWidth / 2) {
                            dismiss = true;
                            dismissDirection = (deltaX > 0);
                        } else if (mMinFlingVelocity <= velocityX && velocityX <= mMaxFlingVelocity
                                && velocityY < velocityX) {
                            dismiss = mSwiping && (deltaX * mVelocityTracker.getXVelocity() > 0);
                            dismissDirection = (mVelocityTracker.getXVelocity() > 0);
                        }

                        dismissTranslation = (dismissDirection ? mListWidth : -mListWidth);
                    } else {
                        float deltaY = ViewHelper.getTranslationY(mSwipeView);

                        if (Math.abs(deltaY) > mListHeight / 2) {
                            dismiss = true;
                            dismissDirection = (deltaY > 0);
                        } else if (mMinFlingVelocity <= velocityY && velocityY <= mMaxFlingVelocity
                                && velocityX < velocityY) {
                            dismiss = mSwiping && (deltaY * mVelocityTracker.getYVelocity() > 0);
                            dismissDirection = (mVelocityTracker.getYVelocity() > 0);
                        }

                        dismissTranslation = (dismissDirection ? mListHeight : -mListHeight);
                     }

                    if (dismiss)
                        animateClose(mSwipeView, dismissTranslation);
                    else
                        animateCancel(mSwipeView);

                    mVelocityTracker.recycle();
                    mVelocityTracker = null;
                    mSwipeView = null;

                    mSwipeStartX = 0;
                    mSwipeStartY = 0;
                    mSwiping = false;

                    break;
                }

                case MotionEvent.ACTION_MOVE: {
                    if (mSwipeView == null || mVelocityTracker == null)
                        break;

                    mVelocityTracker.addMovement(e);

                    final boolean isVertical = isVertical();

                    float deltaX = e.getRawX() - mSwipeStartX;
                    float deltaY = e.getRawY() - mSwipeStartY;
                    float delta = (isVertical ? deltaX : deltaY);

                    boolean isScrollingX = Math.abs(deltaX) > mSwipeThreshold;
                    boolean isScrollingY = Math.abs(deltaY) > mSwipeThreshold;
                    boolean isSwipingToClose = (isVertical ? isScrollingX : isScrollingY);

                    
                    
                    if (isScrollingX || isScrollingY)
                        cancelCheckForTap();

                    if (isSwipingToClose) {
                        mSwiping = true;
                        TabsListLayout.this.requestDisallowInterceptTouchEvent(true);

                        TabsLayoutItemView tab = (TabsLayoutItemView) mSwipeView.getTag();
                        tab.close.setVisibility(View.INVISIBLE);

                        
                        
                        MotionEvent cancelEvent = MotionEvent.obtain(e);
                        cancelEvent.setAction(MotionEvent.ACTION_CANCEL |
                                (e.getActionIndex() << MotionEvent.ACTION_POINTER_INDEX_SHIFT));
                        TabsListLayout.this.onTouchEvent(cancelEvent);
                        cancelEvent.recycle();
                    }

                    if (mSwiping) {
                        if (isVertical)
                            ViewHelper.setTranslationX(mSwipeView, delta);
                        else
                            ViewHelper.setTranslationY(mSwipeView, delta);

                        ViewHelper.setAlpha(mSwipeView, Math.max(0.1f, Math.min(1f,
                                1f - 2f * Math.abs(delta) / (isVertical ? mListWidth : mListHeight))));

                        return true;
                    }

                    break;
                }
            }

            return false;
        }

        private View findViewAt(float rawX, float rawY) {
            Rect rect = new Rect();

            int[] listViewCoords = new int[2];
            TabsListLayout.this.getLocationOnScreen(listViewCoords);

            int x = (int) rawX - listViewCoords[0];
            int y = (int) rawY - listViewCoords[1];

            for (int i = 0; i < TabsListLayout.this.getChildCount(); i++) {
                View child = TabsListLayout.this.getChildAt(i);
                child.getHitRect(rect);

                if (rect.contains(x, y))
                    return child;
            }

            return null;
        }

        private void triggerCheckForTap() {
            if (mPendingCheckForTap == null)
                mPendingCheckForTap = new CheckForTap();

            TabsListLayout.this.postDelayed(mPendingCheckForTap, ViewConfiguration.getTapTimeout());
        }

        private void cancelCheckForTap() {
            if (mPendingCheckForTap == null)
                return;

            TabsListLayout.this.removeCallbacks(mPendingCheckForTap);
        }

        private class CheckForTap implements Runnable {
            @Override
            public void run() {
                if (!mSwiping && mSwipeView != null && mEnabled)
                    mSwipeView.setPressed(true);
            }
        }
    }
}
