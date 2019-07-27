



package org.mozilla.gecko.menu;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.ThreadUtils.AssertBehavior;
import org.mozilla.gecko.widget.GeckoActionProvider;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class GeckoMenu extends ListView 
                       implements Menu,
                                  AdapterView.OnItemClickListener,
                                  GeckoMenuItem.OnShowAsActionChangedListener {
    private static final String LOGTAG = "GeckoMenu";

    



    private static final AssertBehavior THREAD_ASSERT_BEHAVIOR = AppConstants.RELEASE_BUILD ? AssertBehavior.NONE : AssertBehavior.THROW;

    


    public static interface Callback {
        
        public boolean onMenuItemClick(MenuItem item);

        
        public boolean onMenuItemLongClick(MenuItem item);
    }

    




    public static interface MenuPresenter {
        
        public void openMenu();

        
        public void showMenu(View menu);

        
        public void closeMenu();
    }

    





    public static interface ActionItemBarPresenter {
        
        public boolean addActionItem(View actionItem);

        
        public void removeActionItem(View actionItem);
    }

    protected static final int NO_ID = 0;

    
    private List<GeckoMenuItem> mItems;

    
    private Map<GeckoMenuItem, View> mPrimaryActionItems;

    
    private Map<GeckoMenuItem, View> mSecondaryActionItems;

    
    private Callback mCallback;

    
    private MenuPresenter mMenuPresenter;

    
    private ActionItemBarPresenter mPrimaryActionItemBar;

    
    private final ActionItemBarPresenter mSecondaryActionItemBar;

    
    private MenuItemsAdapter mAdapter;

    
    boolean mShowIcons;

    public GeckoMenu(Context context) {
        this(context, null);
    }

    public GeckoMenu(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.geckoMenuListViewStyle);
    }

    public GeckoMenu(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT,
                                         LayoutParams.MATCH_PARENT));

        
        mAdapter = new MenuItemsAdapter();
        setAdapter(mAdapter);
        setOnItemClickListener(this);

        mItems = new ArrayList<GeckoMenuItem>();
        mPrimaryActionItems = new HashMap<GeckoMenuItem, View>();
        mSecondaryActionItems = new HashMap<GeckoMenuItem, View>();

        mPrimaryActionItemBar = (DefaultActionItemBar) LayoutInflater.from(context).inflate(R.layout.menu_action_bar, null);
        mSecondaryActionItemBar = (DefaultActionItemBar) LayoutInflater.from(context).inflate(R.layout.menu_secondary_action_bar, null);
    }

    private static void assertOnUiThread() {
        ThreadUtils.assertOnUiThread(THREAD_ASSERT_BEHAVIOR);
    }

    @Override
    public MenuItem add(CharSequence title) {
        GeckoMenuItem menuItem = new GeckoMenuItem(this, NO_ID, 0, title);
        addItem(menuItem);
        return menuItem;
    }

    @Override
    public MenuItem add(int groupId, int itemId, int order, int titleRes) {
        GeckoMenuItem menuItem = new GeckoMenuItem(this, itemId, order, titleRes);
        addItem(menuItem);
        return menuItem;
    }

    @Override
    public MenuItem add(int titleRes) {
        GeckoMenuItem menuItem = new GeckoMenuItem(this, NO_ID, 0, titleRes);
        addItem(menuItem);
        return menuItem;
    }

    @Override
    public MenuItem add(int groupId, int itemId, int order, CharSequence title) {
        GeckoMenuItem menuItem = new GeckoMenuItem(this, itemId, order, title);
        addItem(menuItem);
        return menuItem;
    }

    private void addItem(GeckoMenuItem menuItem) {
        assertOnUiThread();
        menuItem.setOnShowAsActionChangedListener(this);
        mAdapter.addMenuItem(menuItem);
        mItems.add(menuItem);
    }

    private boolean addActionItem(final GeckoMenuItem menuItem) {
        assertOnUiThread();
        menuItem.setOnShowAsActionChangedListener(this);

        final View actionView = menuItem.getActionView();
        final int actionEnum = menuItem.getActionEnum();
        boolean added = false;

        if (actionEnum == GeckoMenuItem.SHOW_AS_ACTION_ALWAYS) {
            if (mPrimaryActionItems.size() == 0 &&
                mPrimaryActionItemBar instanceof DefaultActionItemBar) {
                
                setAdapter(null);
                addHeaderView((DefaultActionItemBar) mPrimaryActionItemBar);
                setAdapter(mAdapter);
            }

            if (added = mPrimaryActionItemBar.addActionItem(actionView)) {
                mPrimaryActionItems.put(menuItem, actionView);
                mItems.add(menuItem);
            }
        } else if (actionEnum == GeckoMenuItem.SHOW_AS_ACTION_IF_ROOM) {
            if (mSecondaryActionItems.size() == 0) {
                
                setAdapter(null);
                addHeaderView((DefaultActionItemBar) mSecondaryActionItemBar);
                setAdapter(mAdapter);
            }

            if (added = mSecondaryActionItemBar.addActionItem(actionView)) {
                mSecondaryActionItems.put(menuItem, actionView);
                mItems.add(menuItem);
            }
        }

        
        if (actionView instanceof MenuItemActionBar) {
            ((MenuItemActionBar) actionView).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    handleMenuItemClick(menuItem);
                }
            });
            ((MenuItemActionBar) actionView).setOnLongClickListener(new View.OnLongClickListener() {
                @Override
                public boolean onLongClick(View v) {
                    handleMenuItemLongClick(menuItem);
                    return true;
                }
            });
        } else if (actionView instanceof MenuItemActionView) {
            ((MenuItemActionView) actionView).setMenuItemClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    handleMenuItemClick(menuItem);
                }
            });
            ((MenuItemActionView) actionView).setMenuItemLongClickListener(new View.OnLongClickListener() {
                @Override
                public boolean onLongClick(View view) {
                    handleMenuItemLongClick(menuItem);
                    return true;
                }
            });
        }

        return added;
    }

    @Override
    public int addIntentOptions(int groupId, int itemId, int order, ComponentName caller, Intent[] specifics, Intent intent, int flags, MenuItem[] outSpecificItems) {
        return 0;
    }

    @Override
    public SubMenu addSubMenu(int groupId, int itemId, int order, CharSequence title) {
        MenuItem menuItem = add(groupId, itemId, order, title);
        return addSubMenu(menuItem);
    }

    @Override
    public SubMenu addSubMenu(int groupId, int itemId, int order, int titleRes) {
        MenuItem menuItem = add(groupId, itemId, order, titleRes);
        return addSubMenu(menuItem);
    }

    @Override
    public SubMenu addSubMenu(CharSequence title) {
        MenuItem menuItem = add(title);
        return addSubMenu(menuItem);
    }

    @Override
    public SubMenu addSubMenu(int titleRes) {
        MenuItem menuItem = add(titleRes);
        return addSubMenu(menuItem);
    }

    private SubMenu addSubMenu(MenuItem menuItem) {
        GeckoSubMenu subMenu = new GeckoSubMenu(getContext());
        subMenu.setMenuItem(menuItem);
        subMenu.setCallback(mCallback);
        subMenu.setMenuPresenter(mMenuPresenter);
        ((GeckoMenuItem) menuItem).setSubMenu(subMenu);
        return subMenu;
    }

    private void removePrimaryActionBarView() {
        
        setAdapter(null);
        removeHeaderView((DefaultActionItemBar) mPrimaryActionItemBar);
        setAdapter(mAdapter);
    }

    private void removeSecondaryActionBarView() {
        
        setAdapter(null);
        removeHeaderView((DefaultActionItemBar) mSecondaryActionItemBar);
        setAdapter(mAdapter);
    }

    @Override
    public void clear() {
        assertOnUiThread();
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.hasSubMenu()) {
                SubMenu sub = menuItem.getSubMenu();
                if (sub == null) {
                    continue;
                }
                try {
                    sub.clear();
                } catch (Exception ex) {
                    Log.e(LOGTAG, "Couldn't clear submenu.", ex);
                }
            }
        }

        mAdapter.clear();
        mItems.clear();

        




        if (mPrimaryActionItemBar != null) {
            for (View item : mPrimaryActionItems.values()) {
                mPrimaryActionItemBar.removeActionItem(item);
            }
        }
        mPrimaryActionItems.clear();

        if (mSecondaryActionItemBar != null) {
            for (View item : mSecondaryActionItems.values()) {
                mSecondaryActionItemBar.removeActionItem(item);
            }
        }
        mSecondaryActionItems.clear();

        
        
        if (mPrimaryActionItemBar instanceof DefaultActionItemBar) {
            removePrimaryActionBarView();
        }

        removeSecondaryActionBarView();
    }

    @Override
    public void close() {
        if (mMenuPresenter != null)
            mMenuPresenter.closeMenu();
    }

    private void showMenu(View viewForMenu) {
        if (mMenuPresenter != null)
            mMenuPresenter.showMenu(viewForMenu);
    }

    @Override
    public MenuItem findItem(int id) {
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.getItemId() == id) {
                return menuItem;
            } else if (menuItem.hasSubMenu()) {
                if (!menuItem.hasActionProvider()) {
                    SubMenu subMenu = menuItem.getSubMenu();
                    MenuItem item = subMenu.findItem(id);
                    if (item != null)
                        return item;
                }
            }
        }
        return null;
    }

    @Override
    public MenuItem getItem(int index) {
        if (index < mItems.size())
            return mItems.get(index);

        return null;
    }

    @Override
    public boolean hasVisibleItems() {
        assertOnUiThread();
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.isVisible() &&
                !mPrimaryActionItems.containsKey(menuItem) &&
                !mSecondaryActionItems.containsKey(menuItem))
                return true;
        }

        return false;
    }

    @Override
    public boolean isShortcutKey(int keyCode, KeyEvent event) {
        return true;
    }

    @Override
    public boolean performIdentifierAction(int id, int flags) {
        return false;
    }

    @Override
    public boolean performShortcut(int keyCode, KeyEvent event, int flags) {
        return false;
    }

    @Override
    public void removeGroup(int groupId) {
    }

    @Override
    public void removeItem(int id) {
        assertOnUiThread();
        GeckoMenuItem item = (GeckoMenuItem) findItem(id);
        if (item == null)
            return;

        
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.hasSubMenu()) {
                SubMenu subMenu = menuItem.getSubMenu();
                if (subMenu != null && subMenu.findItem(id) != null) {
                    subMenu.removeItem(id);
                    return;
                }
            }
        }

        
        if (mPrimaryActionItems.containsKey(item)) {
            if (mPrimaryActionItemBar != null)
                mPrimaryActionItemBar.removeActionItem(mPrimaryActionItems.get(item));

            mPrimaryActionItems.remove(item);
            mItems.remove(item);

            if (mPrimaryActionItems.size() == 0 && 
                mPrimaryActionItemBar instanceof DefaultActionItemBar) {
                removePrimaryActionBarView();
            }

            return;
        }

        if (mSecondaryActionItems.containsKey(item)) {
            if (mSecondaryActionItemBar != null)
                mSecondaryActionItemBar.removeActionItem(mSecondaryActionItems.get(item));

            mSecondaryActionItems.remove(item);
            mItems.remove(item);

            if (mSecondaryActionItems.size() == 0) {
                removeSecondaryActionBarView();
            }

            return;
        }

        mAdapter.removeMenuItem(item);
        mItems.remove(item);
    }

    @Override
    public void setGroupCheckable(int group, boolean checkable, boolean exclusive) {
    }

    @Override
    public void setGroupEnabled(int group, boolean enabled) {
    }

    @Override
    public void setGroupVisible(int group, boolean visible) {
    }

    @Override
    public void setQwertyMode(boolean isQwerty) {
    }

    @Override
    public int size() {
        return mItems.size();
    }

    @Override
    public boolean hasActionItemBar() {
         return (mPrimaryActionItemBar != null) && (mSecondaryActionItemBar != null);
    }

    @Override
    public void onShowAsActionChanged(GeckoMenuItem item) {
        removeItem(item.getItemId());

        if (item.isActionItem() && addActionItem(item)) {
            return;
        }

        addItem(item);
    }

    public void onItemChanged(GeckoMenuItem item) {
        assertOnUiThread();
        if (item.isActionItem()) {
            final View actionView;
            if (item.getActionEnum() == GeckoMenuItem.SHOW_AS_ACTION_ALWAYS) {
                actionView = mPrimaryActionItems.get(item);
            } else {
                actionView = mSecondaryActionItems.get(item);
            }

            if (actionView != null) {
                
                
                final GeckoMenuItem menuItem = item;
                actionView.post(new Runnable() {
                    @Override
                    public void run() {
                        if (menuItem.isVisible()) {
                            actionView.setVisibility(View.VISIBLE);
                            if (actionView instanceof MenuItemActionBar) {
                                ((MenuItemActionBar) actionView).initialize(menuItem);
                            } else {
                                ((MenuItemActionView) actionView).initialize(menuItem);
                            }
                        } else {
                            actionView.setVisibility(View.GONE);
                        }
                    }
                });
            }
        } else {
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        
        position -= getHeaderViewsCount();

        GeckoMenuItem item = mAdapter.getItem(position);
        handleMenuItemClick(item);
    }

    void handleMenuItemClick(GeckoMenuItem item) {
        if (!item.isEnabled())
            return;

        if (item.invoke()) {
            close();
        } else if (item.hasSubMenu()) {
            
            GeckoActionProvider provider = item.getGeckoActionProvider();
            if (provider != null) {
                GeckoSubMenu subMenu = new GeckoSubMenu(getContext());
                subMenu.setShowIcons(true);
                provider.onPrepareSubMenu(subMenu);
                item.setSubMenu(subMenu);
            }

            
            GeckoSubMenu subMenu = (GeckoSubMenu) item.getSubMenu();
            showMenu(subMenu);
        } else {
            close();
            mCallback.onMenuItemClick(item);
        }
    }

    void handleMenuItemLongClick(GeckoMenuItem item) {
        if(!item.isEnabled()) {
            return;
        }

        if(mCallback != null) {
            mCallback.onMenuItemLongClick(item);
        }
    }

    public Callback getCallback() {
        return mCallback;
    }

    public MenuPresenter getMenuPresenter() {
        return mMenuPresenter;
    }

    public void setCallback(Callback callback) {
        mCallback = callback;

        
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.hasSubMenu()) {
                GeckoSubMenu subMenu = (GeckoSubMenu) menuItem.getSubMenu();
                subMenu.setCallback(mCallback);
            }
        }
    }

    public void setMenuPresenter(MenuPresenter presenter) {
        mMenuPresenter = presenter;

        
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.hasSubMenu()) {
                GeckoSubMenu subMenu = (GeckoSubMenu) menuItem.getSubMenu();
                subMenu.setMenuPresenter(mMenuPresenter);
            }
        }
    }

    public void setActionItemBarPresenter(ActionItemBarPresenter presenter) {
        mPrimaryActionItemBar = presenter;
    }

    public void setShowIcons(boolean show) {
        if (mShowIcons != show) {
            mShowIcons = show;
            mAdapter.notifyDataSetChanged();
        }
    }

    
    
    public static class DefaultActionItemBar extends LinearLayout
                                             implements ActionItemBarPresenter {
        private final int mRowHeight;
        private float mWeightSum;

        public DefaultActionItemBar(Context context) {
            this(context, null);
        }

        public DefaultActionItemBar(Context context, AttributeSet attrs) {
            super(context, attrs);

            mRowHeight = getResources().getDimensionPixelSize(R.dimen.menu_item_row_height);
        }

        @Override
        public boolean addActionItem(View actionItem) {
            ViewGroup.LayoutParams actualParams = actionItem.getLayoutParams();
            LinearLayout.LayoutParams params;

            if (actualParams != null) {
                params = new LinearLayout.LayoutParams(actionItem.getLayoutParams());
                params.width = 0;
            } else {
                params = new LinearLayout.LayoutParams(0, mRowHeight);
            }

            if (actionItem instanceof MenuItemActionView) {
                params.weight = ((MenuItemActionView) actionItem).getChildCount();
            } else {
                params.weight = 1.0f;
            }

            mWeightSum += params.weight;

            actionItem.setLayoutParams(params);
            addView(actionItem);
            setWeightSum(mWeightSum);
            return true;
        }

        @Override
        public void removeActionItem(View actionItem) {
            if (indexOfChild(actionItem) != -1) {
                LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) actionItem.getLayoutParams();
                mWeightSum -= params.weight;
                removeView(actionItem);
            }
        }
    }

    
    private class MenuItemsAdapter extends BaseAdapter {
        private static final int VIEW_TYPE_DEFAULT = 0;
        private static final int VIEW_TYPE_ACTION_MODE = 1;

        private List<GeckoMenuItem> mItems;

        public MenuItemsAdapter() {
            mItems = new ArrayList<GeckoMenuItem>();
        }

        @Override
        public int getCount() {
            if (mItems == null)
                return 0;

            int visibleCount = 0;
            for (GeckoMenuItem item : mItems) {
                if (item.isVisible())
                    visibleCount++;
            }

            return visibleCount;
        }

        @Override
        public GeckoMenuItem getItem(int position) {
            for (GeckoMenuItem item : mItems) {
                if (item.isVisible()) {
                    position--;

                    if (position < 0)
                        return item;
                }
            }

            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            GeckoMenuItem item = getItem(position);
            GeckoMenuItem.Layout view = null;

            
            if (convertView == null && getItemViewType(position) == VIEW_TYPE_DEFAULT) {
                view = new MenuItemDefault(parent.getContext(), null);
            } else {
                view = (GeckoMenuItem.Layout) convertView;
            }

            if (view == null || view instanceof MenuItemActionView) {
                
                
                view = (MenuItemActionView) item.getActionView();

                
                
                final View actionView = (View) view;
                final int pos = position;
                final long id = getItemId(position);
                ((MenuItemActionView) view).setMenuItemClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        GeckoMenu listView = GeckoMenu.this;
                        listView.performItemClick(actionView, pos + listView.getHeaderViewsCount(), id);
                    }
                });
            }

            
            view.setShowIcon(mShowIcons);
            view.initialize(item);
            return (View) view; 
        }

        @Override
        public int getItemViewType(int position) {
            return getItem(position).getGeckoActionProvider() == null ? VIEW_TYPE_DEFAULT : VIEW_TYPE_ACTION_MODE;
        }

        @Override
        public int getViewTypeCount() {
            return 2;
        }

        @Override
        public boolean hasStableIds() {
            return false;
        }

        @Override
        public boolean areAllItemsEnabled() {
            
            
            return true;
        }

        @Override
        public boolean isEnabled(int position) {
            
            
            return true;
        }

        public void addMenuItem(GeckoMenuItem menuItem) {
            if (mItems.contains(menuItem))
                return;

            
            int index = 0;
            for (GeckoMenuItem item : mItems) {
                if (item.getOrder() > menuItem.getOrder()) {
                    mItems.add(index, menuItem);
                    notifyDataSetChanged();
                    return;
                } else {
                  index++;
                }
            }

            
            mItems.add(menuItem);
            notifyDataSetChanged();
        }

        public void removeMenuItem(GeckoMenuItem menuItem) {
            
            mItems.remove(menuItem);
            notifyDataSetChanged();
        }

        public void clear() {
            mItems.clear();
            notifyDataSetChanged();
        }

        public GeckoMenuItem getMenuItem(int id) {
            for (GeckoMenuItem item : mItems) {
                if (item.getItemId() == id)
                    return item;
            }

            return null;
        }
    }
}
