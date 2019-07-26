




package org.mozilla.gecko;

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

    private View mAnchor;

    private MenuPopup mMenuPopup;
    private MenuPanel mMenuPanel;

    private GeckoMenu mMenu;
    private GeckoMenuInflater mMenuInflater;

    private OnDismissListener mDismissListener;
    private OnMenuItemClickListener mClickListener;

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

        setAnchor(anchor);
    }

    




    public Menu getMenu() {
        return mMenu;
    }

    




    public MenuInflater getMenuInflater() {
        return mMenuInflater;
    }

    




    public void inflate(int menuRes) {
        mMenuInflater.inflate(menuRes, mMenu);

        mMenuPanel.addView(mMenu);
        mMenuPopup.setPanelView(mMenuPanel);
    }

    




    public void setAnchor(View anchor) {
        mAnchor = anchor;
    }

    public void setOnDismissListener(OnDismissListener listener) {
        mDismissListener = listener;
    }

    public void setOnMenuItemClickListener(OnMenuItemClickListener listener) {
        mClickListener = listener;
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
    public boolean onMenuItemSelected(MenuItem item) {
        if (mClickListener != null)
            return mClickListener.onMenuItemClick(item);

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
