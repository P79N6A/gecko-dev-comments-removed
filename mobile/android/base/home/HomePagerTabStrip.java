




package org.mozilla.gecko.home;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.v4.view.PagerTabStrip;
import android.util.AttributeSet;

import org.mozilla.gecko.R;






class HomePagerTabStrip extends PagerTabStrip {

    public HomePagerTabStrip(Context context) {
        super(context);
    }

    public HomePagerTabStrip(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.HomePagerTabStrip);
        int color = a.getColor(R.styleable.HomePagerTabStrip_tabIndicatorColor, 0x00);
        a.recycle();

        setTabIndicatorColor(color);
    }
}
