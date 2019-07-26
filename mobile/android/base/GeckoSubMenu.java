



package org.mozilla.gecko;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;

public class GeckoSubMenu extends GeckoMenu 
                          implements SubMenu {
    private static final String LOGTAG = "GeckoSubMenu";

    private Context mContext;

    
    private MenuItem mMenuItem;

    public GeckoSubMenu(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    @Override
    public void clearHeader() {
    }

    public SubMenu setMenuItem(MenuItem item) {
        mMenuItem = item;
        return this;
    }

    @Override
    public MenuItem getItem() {
        return mMenuItem;
    }

    @Override
    public SubMenu setHeaderIcon(Drawable icon) {
        return this;
    }

    @Override
    public SubMenu setHeaderIcon(int iconRes) {
        return this;
    }

    @Override
    public SubMenu setHeaderTitle(CharSequence title) {
        return this;
    }

    @Override
    public SubMenu setHeaderTitle(int titleRes) {
        return this;
    }

    @Override
    public SubMenu setHeaderView(View view) { 
        return this;
    }

    @Override
    public SubMenu setIcon(Drawable icon) {
        return this;
    }

    @Override
    public SubMenu setIcon(int iconRes) {
        return this;
    }
}
