




package org.mozilla.gecko.tabs;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.tabs.TabsPanel.TabsLayout;
import org.mozilla.gecko.Tabs;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.PointF;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.animation.DecelerateInterpolator;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.GridView;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.animation.PropertyValuesHolder;
import com.nineoldandroids.animation.ValueAnimator;







class TabsGridLayout extends GridView
                     implements TabsLayout,
                                Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "Gecko" + TabsGridLayout.class.getSimpleName();

    private static final int ANIM_TIME_MS = 200;
    public static final int ANIM_DELAY_MULTIPLE_MS = 20;
    private static final DecelerateInterpolator ANIM_INTERPOLATOR = new DecelerateInterpolator();

    private final Context mContext;
    private TabsPanel mTabsPanel;
    private final SparseArray<PointF> mTabLocations = new SparseArray<PointF>();

    final private boolean mIsPrivate;

    private final TabsLayoutAdapter mTabsAdapter;
    private final int mColumnWidth;

    public TabsGridLayout(Context context, AttributeSet attrs) {
        super(context, attrs, R.attr.tabGridLayoutViewStyle);
        mContext = context;

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabsLayout);
        mIsPrivate = (a.getInt(R.styleable.TabsLayout_tabs, 0x0) == 1);
        a.recycle();

        mTabsAdapter = new TabsGridLayoutAdapter(mContext);
        setAdapter(mTabsAdapter);

        setRecyclerListener(new RecyclerListener() {
            @Override
            public void onMovedToScrapHeap(View view) {
                TabsLayoutItemView item = (TabsLayoutItemView) view;
                item.setThumbnail(null);
            }
        });

        
        
        setClipToPadding(false);

        final Resources resources = getResources();
        mColumnWidth = resources.getDimensionPixelSize(R.dimen.new_tablet_tab_panel_column_width);

        final int padding = resources.getDimensionPixelSize(R.dimen.new_tablet_tab_panel_grid_padding);
        final int paddingTop = resources.getDimensionPixelSize(R.dimen.new_tablet_tab_panel_grid_padding_top);

        
        
        final int paddingBottom = paddingTop * 2;

        setPadding(padding, paddingTop, padding, paddingBottom);

        setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TabsLayoutItemView tab = (TabsLayoutItemView) view;
                Tabs.getInstance().selectTab(tab.getTabId());
                autoHidePanel();
            }
        });
    }

    private class TabsGridLayoutAdapter extends TabsLayoutAdapter {

        final private Button.OnClickListener mCloseClickListener;

        public TabsGridLayoutAdapter (Context context) {
            super(context, R.layout.new_tablet_tabs_item_cell);

            mCloseClickListener = new Button.OnClickListener() {
                @Override
                public void onClick(View v) {
                    closeTab(v);
                }
            };
        }

        @Override
        TabsLayoutItemView newView(int position, ViewGroup parent) {
            final TabsLayoutItemView item = super.newView(position, parent);

            item.setCloseOnClickListener(mCloseClickListener);

            return item;
        }

        @Override
        public void bindView(TabsLayoutItemView view, Tab tab) {
            super.bindView(view, tab);

            
            
            resetTransforms(view);
        }
    }

    private void populateTabLocations(final Tab removedTab) {
        mTabLocations.clear();

        final int firstPosition = getFirstVisiblePosition();
        final int lastPosition = getLastVisiblePosition();
        final int numberOfColumns = getNumColumns();
        final int childCount = getChildCount();
        final int removedPosition = mTabsAdapter.getPositionForTab(removedTab);

        for (int x = 1, i = (removedPosition - firstPosition) + 1; i < childCount; i++, x++) {
            final View child = getChildAt(i);
            if (child != null) {
                mTabLocations.append(x, new PointF(child.getX(), child.getY()));
            }
        }

        final boolean firstChildOffScreen = ((firstPosition > 0) || getChildAt(0).getY() < 0);
        final boolean lastChildVisible = (lastPosition - childCount == firstPosition - 1);
        final boolean oneItemOnLastRow = (lastPosition % numberOfColumns == 0);
        if (firstChildOffScreen && lastChildVisible && oneItemOnLastRow) {
            
            
            

            final int removedHeight = getChildAt(0).getMeasuredHeight();
            final int verticalSpacing =
                    getResources().getDimensionPixelOffset(R.dimen.new_tablet_tab_panel_grid_vspacing);

            ValueAnimator paddingAnimator = ValueAnimator.ofInt(getPaddingBottom() + removedHeight + verticalSpacing, getPaddingBottom());
            paddingAnimator.setDuration(ANIM_TIME_MS * 2);

            paddingAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {

                @Override
                public void onAnimationUpdate(ValueAnimator animation) {
                    setPadding(getPaddingLeft(), getPaddingTop(), getPaddingRight(), (Integer) animation.getAnimatedValue());
                }
            });
            paddingAnimator.start();
        }
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
        return true;
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
                if (mTabsAdapter.getCount() > 0) {
                    animateRemoveTab(tab);
                }

                final Tabs tabsInstance = Tabs.getInstance();

                if (mTabsAdapter.removeTab(tab)) {
                    if (tab.isPrivate() == mIsPrivate && mTabsAdapter.getCount() > 0) {
                        int selected = mTabsAdapter.getPositionForTab(tabsInstance.getSelectedTab());
                        updateSelectedStyle(selected);
                    }
                    if(!tab.isPrivate()) {
                        
                        final Iterable<Tab> tabs = tabsInstance.getTabsInOrder();
                        boolean removedTabIsLastNormalTab = true;
                        for (Tab singleTab : tabs) {
                            if (!singleTab.isPrivate()) {
                                removedTabIsLastNormalTab = false;
                                break;
                            }
                        }
                        if (removedTabIsLastNormalTab) {
                            tabsInstance.addTab();
                        }
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

                ((TabsLayoutItemView) view).assignValues(tab);
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
        
        
        ArrayList<Tab> tabData = new ArrayList<>();

        Iterable<Tab> allTabs = Tabs.getInstance().getTabsInOrder();
        for (Tab tab : allTabs) {
            if (tab.isPrivate() == mIsPrivate)
                tabData.add(tab);
        }

        mTabsAdapter.setTabs(tabData);
        updateSelectedPosition();
    }

    private void resetTransforms(View view) {
        ViewHelper.setAlpha(view, 1);
        ViewHelper.setTranslationX(view, 0);
    }

    @Override
    public void closeAll() {

        autoHidePanel();

        if (getChildCount() == 0) {
            return;
        }

        final Iterable<Tab> tabs = Tabs.getInstance().getTabsInOrder();
        for (Tab tab : tabs) {
            
            
            if (!mIsPrivate || tab.isPrivate()) {
                Tabs.getInstance().closeTab(tab, false);
            }
        }
    }

    private View getViewForTab(Tab tab) {
        final int position = mTabsAdapter.getPositionForTab(tab);
        return getChildAt(position - getFirstVisiblePosition());
    }

    void closeTab(View v) {
        TabsLayoutItemView itemView = (TabsLayoutItemView) v.getTag();
        Tab tab = Tabs.getInstance().getTab(itemView.getTabId());

        Tabs.getInstance().closeTab(tab);
        updateSelectedPosition();
    }

    private void animateRemoveTab(final Tab removedTab) {
        final int removedPosition = mTabsAdapter.getPositionForTab(removedTab);

        final View removedView = getViewForTab(removedTab);

        
        
        if (removedView == null) {
            return;
        }
        final int removedHeight = removedView.getMeasuredHeight();

        populateTabLocations(removedTab);

        getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                getViewTreeObserver().removeOnPreDrawListener(this);
                
                
                
                final int childCount = getChildCount();
                final int firstPosition = getFirstVisiblePosition();
                final int numberOfColumns = getNumColumns();

                final List<Animator> childAnimators = new ArrayList<>();

                PropertyValuesHolder translateX, translateY;
                for (int x = 0, i = removedPosition - firstPosition ; i < childCount; i++, x++) {
                    final View child = getChildAt(i);
                    ObjectAnimator animator;

                    if (i % numberOfColumns == numberOfColumns - 1) {
                        
                        translateX = PropertyValuesHolder.ofFloat("translationX", -(mColumnWidth * numberOfColumns), 0);
                        translateY = PropertyValuesHolder.ofFloat("translationY", removedHeight, 0);
                        animator = ObjectAnimator.ofPropertyValuesHolder(child, translateX, translateY);
                    } else {
                        
                        translateX = PropertyValuesHolder.ofFloat("translationX", mColumnWidth, 0);
                        animator = ObjectAnimator.ofPropertyValuesHolder(child, translateX);
                    }
                    animator.setStartDelay(x * ANIM_DELAY_MULTIPLE_MS);
                    childAnimators.add(animator);
                }

                final AnimatorSet animatorSet = new AnimatorSet();
                animatorSet.playTogether(childAnimators);
                animatorSet.setDuration(ANIM_TIME_MS);
                animatorSet.setInterpolator(ANIM_INTERPOLATOR);
                animatorSet.start();

                
                
                
                for (int x = 1, i = (removedPosition - firstPosition) + 1; i < childCount; i++, x++) {
                    final View child = getChildAt(i);

                    final PointF targetLocation = mTabLocations.get(x+1);
                    if (targetLocation == null) {
                        continue;
                    }

                    child.setX(targetLocation.x);
                    child.setY(targetLocation.y);
                }

                return true;
            }
        });
    }
}
