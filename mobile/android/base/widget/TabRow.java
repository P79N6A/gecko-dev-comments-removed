




package org.mozilla.gecko.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Checkable;
import android.widget.LinearLayout;

public class TabRow extends LinearLayout
                    implements Checkable {
    private static final String LOGTAG = "GeckoTabRow";
    private static final int[] STATE_CHECKED = { android.R.attr.state_checked };
    private boolean mChecked;

    public TabRow(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        if (mChecked)
            mergeDrawableStates(drawableState, STATE_CHECKED);

        return drawableState;
    }

    @Override
    public boolean isChecked() {
        return mChecked;
    }

    @Override
    public void setChecked(boolean checked) {
        if (mChecked != checked) {
            mChecked = checked;
            refreshDrawableState();

            int count = getChildCount();
            for (int i=0; i < count; i++) {
                final View child = getChildAt(i);
                if (child instanceof Checkable)
                    ((Checkable) child).setChecked(checked);
            }
        }
    }

    @Override
    public void toggle() {
        mChecked = !mChecked;
    }
}
