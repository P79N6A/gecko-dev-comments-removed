




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.accessibility.AccessibilityEvent;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.TextView;






public class TabMenuStrip extends HorizontalScrollView
                          implements HomePager.Decor {

    
    
    private static final int TITLE_OFFSET_DIPS = 24;

    private final int titleOffset;
    private final TabMenuStripLayout layout;

    public TabMenuStrip(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        setHorizontalScrollBarEnabled(false);

        titleOffset = (int) (TITLE_OFFSET_DIPS * getResources().getDisplayMetrics().density);

        layout = new TabMenuStripLayout(context, attrs);
        addView(layout, LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
    }

    @Override
    public void onAddPagerView(String title) {
        layout.onAddPagerView(title);
    }

    @Override
    public void removeAllPagerViews() {
        layout.removeAllViews();
    }

    @Override
    public void onPageSelected(final int position) {
        layout.onPageSelected(position);
    }

    @Override
    public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
        layout.onPageScrolled(position, positionOffset, positionOffsetPixels);

        final View selectedTitle = layout.getChildAt(position);
        if (selectedTitle == null) {
            return;
        }

        final int selectedTitleOffset = (int) (positionOffset * selectedTitle.getWidth());

        int titleLeft = selectedTitle.getLeft() + selectedTitleOffset;
        if (position > 0) {
            titleLeft -= titleOffset;
        }

        int titleRight = selectedTitle.getRight() + selectedTitleOffset;
        if (position < layout.getChildCount() - 1) {
            titleRight += titleOffset;
        }

        final int scrollX = getScrollX();
        if (titleLeft < scrollX) {
            
            scrollTo(titleLeft, 0);
        } else if (titleRight > scrollX + getWidth()) {
            
            scrollTo(titleRight - getWidth(), 0);
        }
    }

    @Override
    public void setOnTitleClickListener(HomePager.OnTitleClickListener onTitleClickListener) {
        layout.setOnTitleClickListener(onTitleClickListener);
    }
}
