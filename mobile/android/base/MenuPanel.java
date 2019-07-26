




package org.mozilla.gecko;

import android.content.Context;
import android.os.Build;
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

        
        DisplayMetrics metrics = GeckoApp.mAppContext.getResources().getDisplayMetrics();
        int restrictedHeightSpec = MeasureSpec.makeMeasureSpec((int) (0.75 * metrics.heightPixels), MeasureSpec.AT_MOST);

        super.onMeasure(widthMeasureSpec, restrictedHeightSpec);
    }

    @Override
    public boolean dispatchPopulateAccessibilityEvent (AccessibilityEvent event) {
        if (Build.VERSION.SDK_INT >= 14) 
            onPopulateAccessibilityEvent(event);
        return true;
    }
}
