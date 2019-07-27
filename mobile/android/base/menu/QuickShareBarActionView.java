




package org.mozilla.gecko.menu;

import android.annotation.TargetApi;
import android.content.Context;
import android.util.AttributeSet;

import org.mozilla.gecko.R;









public class QuickShareBarActionView extends MenuItemActionView {

    public QuickShareBarActionView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.menuItemActionViewStyle);
    }

    @TargetApi(14)
    public QuickShareBarActionView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        
        
        removeAllViews();
    }
}
