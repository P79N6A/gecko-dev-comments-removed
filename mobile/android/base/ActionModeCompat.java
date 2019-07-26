



package org.mozilla.gecko;

import org.mozilla.gecko.widget.GeckoPopupMenu;

import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

class ActionModeCompat implements GeckoPopupMenu.OnMenuItemClickListener,
                                  GeckoPopupMenu.OnMenuItemLongClickListener,
                                  View.OnClickListener {
    private final String LOGTAG = "GeckoActionModeCompat";

    private Callback mCallback;
    private ActionModeCompatView mView;
    private Presenter mPresenter;

    

    public static interface Callback {
        
        public boolean onCreateActionMode(ActionModeCompat mode, Menu menu);

        

        public boolean onPrepareActionMode(ActionModeCompat mode, Menu menu);

        
        public boolean onActionItemClicked(ActionModeCompat mode, MenuItem item);

        
        public void onDestroyActionMode(ActionModeCompat mode);
    }

    

    public static interface Presenter {
        
        public void startActionModeCompat(final Callback callback);

        
        public void endActionModeCompat();
    }

    public ActionModeCompat(Presenter presenter, Callback callback, ActionModeCompatView view) {
        mPresenter = presenter;
        mCallback = callback;

        mView = view;
        mView.initForMode(this);
    }

    public void finish() {
        
        mView.getMenu().clear();
        if (mCallback != null) {
            mCallback.onDestroyActionMode(this);
        }
    }

    public CharSequence getTitle() {
        return mView.getTitle();
    }

    public void setTitle(CharSequence title) {
        mView.setTitle(title);
    }

    public void setTitle(int resId) {
        mView.setTitle(resId);
    }

    public Menu getMenu() {
        return mView.getMenu();
    }

    public void invalidate() {
        if (mCallback != null) {
            mCallback.onPrepareActionMode(this, mView.getMenu());
        }
        mView.invalidate();
    }

    
    @Override
    public boolean onMenuItemClick(MenuItem item) {
        if (mCallback != null) {
            return mCallback.onActionItemClicked(this, item);
        }
        return false;
    }

    
    @Override
    public boolean onMenuItemLongClick(MenuItem item) {
        showTooltip(item);
        return true;
    }

    
    @Override
    public void onClick(View v) {
        mPresenter.endActionModeCompat();
    }

    private void showTooltip(MenuItem item) {
        
        
        int[] location = new int[2];
        final View view = item.getActionView();
        view.getLocationOnScreen(location);

        int xOffset = location[0] - view.getWidth();
        int yOffset = location[1] + view.getHeight() / 2;

        Toast toast = Toast.makeText(view.getContext(), item.getTitle(), Toast.LENGTH_SHORT);
        toast.setGravity(Gravity.TOP|Gravity.LEFT, xOffset, yOffset);
        toast.show();
    }
}
