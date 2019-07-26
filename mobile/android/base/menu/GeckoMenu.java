



package org.mozilla.gecko.menu;

import org.mozilla.gecko.R;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.AttributeSet;
import android.view.ActionProvider;
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

    


    public static interface Callback {
        
        public boolean onMenuItemSelected(MenuItem item);
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

    
    private Map<GeckoMenuItem, View> mActionItems;

    
    private Callback mCallback;

    
    private MenuPresenter mMenuPresenter;

    
    private ActionItemBarPresenter mActionItemBarPresenter;

    
    private MenuItemsAdapter mAdapter;

    public GeckoMenu(Context context) {
        this(context, null);
    }

    public GeckoMenu(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.listViewStyle);
    }

    public GeckoMenu(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
                                         LayoutParams.FILL_PARENT));

        
        mAdapter = new MenuItemsAdapter();
        setAdapter(mAdapter);
        setOnItemClickListener(this);

        mItems = new ArrayList<GeckoMenuItem>();
        mActionItems = new HashMap<GeckoMenuItem, View>();

        mActionItemBarPresenter =  (DefaultActionItemBar) LayoutInflater.from(context).inflate(R.layout.menu_action_bar, null);
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
        menuItem.setOnShowAsActionChangedListener(this);
        mAdapter.addMenuItem(menuItem);
        mItems.add(menuItem);
    }

    private boolean addActionItem(final GeckoMenuItem menuItem) {
        menuItem.setOnShowAsActionChangedListener(this);

        if (mActionItems.size() == 0 && 
            mActionItemBarPresenter instanceof DefaultActionItemBar) {
            
            setAdapter(null);
            addHeaderView((DefaultActionItemBar) mActionItemBarPresenter);
            setAdapter(mAdapter);
        }

        View actionView = menuItem.getActionView();
        actionView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleMenuItemClick(menuItem);
            }
        });

        mActionItems.put(menuItem, actionView);
        mActionItemBarPresenter.addActionItem(actionView);
        mItems.add(menuItem);

        return true;
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

    @Override
    public void clear() {
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.hasSubMenu()) {
                menuItem.getSubMenu().clear();
            }
        }

        mAdapter.clear();

        mItems.clear();

        if (mActionItemBarPresenter != null) {
            for (View item : mActionItems.values()) {
                mActionItemBarPresenter.removeActionItem(item);
            }
        }
        mActionItems.clear();
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
        for (GeckoMenuItem menuItem : mItems) {
            if (menuItem.isVisible() && !mActionItems.containsKey(menuItem))
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

        
        if (mActionItems.containsKey(item)) {
            if (mActionItemBarPresenter != null)
                mActionItemBarPresenter.removeActionItem(mActionItems.get(item));

            mActionItems.remove(item);
            mItems.remove(item);

            if (mActionItems.size() == 0 && 
                mActionItemBarPresenter instanceof DefaultActionItemBar) {
                
                setAdapter(null);
                removeHeaderView((DefaultActionItemBar) mActionItemBarPresenter);
                setAdapter(mAdapter);
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
         return (mActionItemBarPresenter != null);
    }

    @Override
    public void onShowAsActionChanged(GeckoMenuItem item, boolean isActionItem) {
        removeItem(item.getItemId());

        if (isActionItem && addActionItem(item)) {
            return;
        }

        addItem(item);
    }

    public void onItemChanged(GeckoMenuItem item) {
        if (item.isActionItem()) {
           final MenuItemActionBar actionView = (MenuItemActionBar) mActionItems.get(item);
           if (actionView != null) {
               
               
               final GeckoMenuItem menuItem = item;
               actionView.post(new Runnable() {
                   @Override
                   public void run() {
                       if (menuItem.isVisible()) {
                           actionView.setVisibility(View.VISIBLE);
                           actionView.initialize(menuItem);
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

    private void handleMenuItemClick(GeckoMenuItem item) {
        if (!item.isEnabled())
            return;

        if (item.invoke()) {
            close();
        } else if (item.hasSubMenu()) {
            
            ActionProvider provider = item.getActionProvider();
            if (provider != null) {
                GeckoSubMenu subMenu = new GeckoSubMenu(getContext());
                provider.onPrepareSubMenu(subMenu);
                item.setSubMenu(subMenu);
            }

            
            GeckoSubMenu subMenu = (GeckoSubMenu) item.getSubMenu();
            showMenu(subMenu);
        } else {
            close();
            mCallback.onMenuItemSelected(item);
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
        mActionItemBarPresenter = presenter;
    }

    
    
    public static class DefaultActionItemBar extends LinearLayout
                                             implements ActionItemBarPresenter {
        public DefaultActionItemBar(Context context) {
            this(context, null);
        }

        public DefaultActionItemBar(Context context, AttributeSet attrs) {
            super(context, attrs);

            setWeightSum(3.0f);
        }

        @Override
        public boolean addActionItem(View actionItem) {
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(actionItem.getLayoutParams());
            params.weight = 1.0f;
            actionItem.setLayoutParams(params);
            addView(actionItem);
            return true;
        }

        @Override
        public void removeActionItem(View actionItem) {
            removeView(actionItem);
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

            
            view.initialize(item);
            return (View) view; 
        }

        @Override
        public int getItemViewType(int position) {
            return getItem(position).getActionProvider() == null ? VIEW_TYPE_DEFAULT : VIEW_TYPE_ACTION_MODE;
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
            for (GeckoMenuItem item : mItems) {
                 if (!item.isEnabled())
                     return false;
            }

            return true;
        }

        @Override
        public boolean isEnabled(int position) {
            return getItem(position).isEnabled();
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
