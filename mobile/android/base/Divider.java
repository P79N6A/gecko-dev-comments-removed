



package org.mozilla.gecko;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout.LayoutParams;

public class Divider extends View {
    public static enum Orientation { HORIZONTAL, VERTICAL };

    
    private Orientation mOrientation;

    
    private int mDensity;

    public Divider(Context context, AttributeSet attrs) {
        super(context, attrs);

        mDensity = (int) context.getResources().getDisplayMetrics().density;

        setOrientation(Orientation.HORIZONTAL);
    }

    public void setOrientation(Orientation orientation) {
        if (mOrientation != orientation) {
            mOrientation = orientation;

            if (mOrientation == Orientation.HORIZONTAL)
                setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, mDensity));
            else
                setLayoutParams(new LayoutParams(mDensity, LayoutParams.FILL_PARENT));
        }
    }
}
