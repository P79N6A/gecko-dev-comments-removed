




package org.mozilla.gecko.menu;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.R;

import android.content.Context;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.LinearLayout;






public class MenuPanel extends LinearLayout {
    public MenuPanel(Context context, AttributeSet attrs) {
        super(context, attrs);

        int width = (int) context.getResources().getDimension(R.dimen.menu_item_row_width);
        setLayoutParams(new ViewGroup.LayoutParams(width, ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        
        DisplayMetrics metrics = getContext().getResources().getDisplayMetrics();
        int restrictedHeightSpec = MeasureSpec.makeMeasureSpec((int) (0.75 * metrics.heightPixels), MeasureSpec.AT_MOST);

        super.onMeasure(widthMeasureSpec, restrictedHeightSpec);
    }

    @Override
    public boolean dispatchPopulateAccessibilityEvent (AccessibilityEvent event) {
        if (Versions.feature14Plus) {
            onPopulateAccessibilityEvent(event);
        }

        return true;
    }
}
