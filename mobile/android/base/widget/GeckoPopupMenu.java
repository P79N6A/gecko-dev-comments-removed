




package org.mozilla.gecko.widget;

import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.GeckoMenuInflater;
import org.mozilla.gecko.menu.MenuPanel;
import org.mozilla.gecko.menu.MenuPopup;

import android.content.Context;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;





public class GeckoPopupMenu implements GeckoMenu.Callback,
                                       GeckoMenu.MenuPresenter {

    
    public static interface OnDismissListener {
        public boolean onDismiss(GeckoMenu menu);
    }

    
    public static interface OnMenuItemClickListener {
        public boolean onMenuItemClick(MenuItem item);
    }

    
    public static interface OnMenuItemLongClickListener {
        public boolean onMenuItemLongClick(MenuItem item);
    }

    private View mAnchor;

    private MenuPopup mMenuPopup;
    private MenuPanel mMenuPanel;

    private GeckoMenu mMenu;
    private GeckoMenuInflater mMenuInflater;

    private OnDismissListener mDismissListener;
    private OnMenuItemClickListener mClickListener;
    private OnMenuItemLongClickListener mLongClickListener;

    public GeckoPopupMenu(Context context) {
        initialize(context, null);
    }

    public GeckoPopupMenu(Context context, View anchor) {
        initialize(context, anchor);
    }

    



    private void initialize(Context context, View anchor) {
        mMenu = new GeckoMenu(context, null);
        mMenu.setCallback(this);
        mMenu.setMenuPresenter(this);
        mMenuInflater = new GeckoMenuInflater(context);

        mMenuPopup = new MenuPopup(context);
        mMenuPanel = new MenuPanel(context, null);

        mMenuPanel.addView(mMenu);
        mMenuPopup.setPanelView(mMenuPanel);

        setAnchor(anchor);
    }

    




    public Menu getMenu() {
        return mMenu;
    }

    




    public MenuInflater getMenuInflater() {
        return mMenuInflater;
    }

    




    public void inflate(int menuRes) {
        if (menuRes > 0) {
            mMenuInflater.inflate(menuRes, mMenu);
        }
    }

    




    public void setAnchor(View anchor) {
        mAnchor = anchor;

        
        if (mMenuPopup.isShowing()) {
            mMenuPopup.dismiss();
            mMenuPopup.showAsDropDown(mAnchor);
        }
    }

    public void setOnDismissListener(OnDismissListener listener) {
        mDismissListener = listener;
    }

    public void setOnMenuItemClickListener(OnMenuItemClickListener listener) {
        mClickListener = listener;
    }

    public void setOnMenuItemLongClickListener(OnMenuItemLongClickListener listener) {
        mLongClickListener = listener;
    }

    


    public void show() {
        if (!mMenuPopup.isShowing())
            mMenuPopup.showAsDropDown(mAnchor);
    }

    


    public void dismiss() {
        if (mMenuPopup.isShowing()) {
            mMenuPopup.dismiss();

            if (mDismissListener != null)
                mDismissListener.onDismiss(mMenu);
        }
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        if (mClickListener != null) {
            return mClickListener.onMenuItemClick(item);
        }
        return false;
    }

    @Override
    public boolean onMenuItemLongClick(MenuItem item) {
        if (mLongClickListener != null) {
            return mLongClickListener.onMenuItemLongClick(item);
        }
        return false;
    }

    @Override
    public void openMenu() {
        show();
    }

    @Override
    public void showMenu(View menu) {
        mMenuPanel.removeAllViews();
        mMenuPanel.addView(menu);

        openMenu();
    }

    @Override
    public void closeMenu() {
        dismiss();
    }
}
