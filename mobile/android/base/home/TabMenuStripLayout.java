




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
import android.widget.LinearLayout;
import android.widget.TextView;





class TabMenuStripLayout extends LinearLayout
                         implements View.OnFocusChangeListener {

    private HomePager.OnTitleClickListener onTitleClickListener;
    private Drawable strip;
    private View selectedView;

    
    private View toTab;
    private View fromTab;
    private float progress;

    
    private float prevProgress;

    TabMenuStripLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabMenuStrip);
        final int stripResId = a.getResourceId(R.styleable.TabMenuStrip_strip, -1);
        a.recycle();

        if (stripResId != -1) {
            strip = getResources().getDrawable(stripResId);
        }

        setWillNotDraw(false);
    }

    void onAddPagerView(String title) {
        final TextView button = (TextView) LayoutInflater.from(getContext()).inflate(R.layout.tab_menu_strip, this, false);
        button.setText(title.toUpperCase());

        addView(button);
        button.setOnClickListener(new ViewClickListener(getChildCount() - 1));
        button.setOnFocusChangeListener(this);
    }

    void onPageSelected(final int position) {
        selectedView = getChildAt(position);

        
        ViewTreeObserver vto = selectedView.getViewTreeObserver();
        if (vto.isAlive()) {
            vto.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    selectedView.getViewTreeObserver().removeGlobalOnLayoutListener(this);

                    if (strip != null) {
                        strip.setBounds(selectedView.getLeft(),
                                        selectedView.getTop(),
                                        selectedView.getRight(),
                                        selectedView.getBottom());
                    }

                    prevProgress = position;
                }
            });
        }
    }

    
    void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
        if (strip == null) {
            return;
        }

        setScrollingData(position, positionOffset);

        if (fromTab == null || toTab == null) {
            return;
        }

        final int fromTabLeft =  fromTab.getLeft();
        final int fromTabRight = fromTab.getRight();

        final int toTabLeft =  toTab.getLeft();
        final int toTabRight = toTab.getRight();

        strip.setBounds((int) (fromTabLeft + ((toTabLeft - fromTabLeft) * progress)),
                         0,
                         (int) (fromTabRight + ((toTabRight - fromTabRight) * progress)),
                         getHeight());
        invalidate();
    }

    




    void setScrollingData(int position, float positionOffset) {
        if (position >= getChildCount() - 1) {
            return;
        }

        final float currProgress = position + positionOffset;

        if (prevProgress > currProgress) {
            toTab = getChildAt(position);
            fromTab = getChildAt(position + 1);
            progress = 1 - positionOffset;
        } else {
            toTab = getChildAt(position + 1);
            fromTab = getChildAt(position);
            progress = positionOffset;
        }

        prevProgress = currProgress;
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (strip != null) {
            strip.draw(canvas);
        }
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (v == this && hasFocus && getChildCount() > 0) {
            selectedView.requestFocus();
            return;
        }

        if (!hasFocus) {
            return;
        }

        int i = 0;
        final int numTabs = getChildCount();

        while (i < numTabs) {
            View view = getChildAt(i);
            if (view == v) {
                view.requestFocus();
                if (isShown()) {
                    
                    sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
                }
                break;
            }

            i++;
        }
    }

    void setOnTitleClickListener(HomePager.OnTitleClickListener onTitleClickListener) {
        this.onTitleClickListener = onTitleClickListener;
    }

    private class ViewClickListener implements OnClickListener {
        private final int mIndex;

        public ViewClickListener(int index) {
            mIndex = index;
        }

        @Override
        public void onClick(View view) {
            if (onTitleClickListener != null) {
                onTitleClickListener.onTitleClicked(mIndex);
            }
        }
    }
}
