




package org.mozilla.gecko.home;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.Rect;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.accessibility.AccessibilityEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewParent;
import android.view.ViewTreeObserver;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.util.Log;

import org.mozilla.gecko.home.HomePager;
import org.mozilla.gecko.R;

public class TabMenuStrip extends LinearLayout
                          implements HomePager.Decor,
                                     View.OnFocusChangeListener {
    private static final String LOGTAG = "GeckoTabMenuStrip";

    private HomePager.OnTitleClickListener mOnTitleClickListener;
    private Drawable mStrip;
    private View mSelectedView;

    
    private View toTab;
    private View fromTab;
    private float progress;

    
    private float mPrevProgress;

    public TabMenuStrip(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabMenuStrip);
        final int stripResId = a.getResourceId(R.styleable.TabMenuStrip_strip, -1);
        a.recycle();

        if (stripResId != -1) {
            mStrip = getResources().getDrawable(stripResId);
        }

        setWillNotDraw(false);
    }

    @Override
    public void onAddPagerView(String title) {
        final TextView button = (TextView) LayoutInflater.from(getContext()).inflate(R.layout.tab_menu_strip, this, false);
        button.setText(title.toUpperCase());

        addView(button);
        button.setOnClickListener(new ViewClickListener(getChildCount() - 1));
        button.setOnFocusChangeListener(this);
    }

    @Override
    public void removeAllPagerViews() {
        removeAllViews();
    }

    @Override
    public void onPageSelected(final int position) {
        mSelectedView = getChildAt(position);

        
        ViewTreeObserver vto = mSelectedView.getViewTreeObserver();
        if (vto.isAlive()) {
            vto.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    mSelectedView.getViewTreeObserver().removeGlobalOnLayoutListener(this);

                    if (mStrip != null) {
                        mStrip.setBounds(mSelectedView.getLeft(),
                                         mSelectedView.getTop(),
                                         mSelectedView.getRight(),
                                         mSelectedView.getBottom());
                    }

                    mPrevProgress = position;
                }
            });
        }
    }

    
    @Override
    public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
        if (mStrip == null) {
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

        mStrip.setBounds((int) (fromTabLeft + ((toTabLeft - fromTabLeft) * progress)),
                         0,
                         (int) (fromTabRight + ((toTabRight - fromTabRight) * progress)),
                         getHeight());
        invalidate();
    }

    




    private void setScrollingData(int position, float positionOffset) {
        if (position >= getChildCount() - 1) {
            return;
        }

        final float currProgress = position + positionOffset;

        if (mPrevProgress > currProgress) {
            toTab = getChildAt(position);
            fromTab = getChildAt(position + 1);
            progress = 1 - positionOffset;
        } else {
            toTab = getChildAt(position + 1);
            fromTab = getChildAt(position);
            progress = positionOffset;
        }

        mPrevProgress = currProgress;
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (mStrip != null) {
            mStrip.draw(canvas);
        }
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (v == this && hasFocus && getChildCount() > 0) {
            mSelectedView.requestFocus();
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

    @Override
    public void setOnTitleClickListener(HomePager.OnTitleClickListener onTitleClickListener) {
        mOnTitleClickListener = onTitleClickListener;
    }

    private class ViewClickListener implements OnClickListener {
        private final int mIndex;

        public ViewClickListener(int index) {
            mIndex = index;
        }

        @Override
        public void onClick(View view) {
            if (mOnTitleClickListener != null) {
                mOnTitleClickListener.onTitleClicked(mIndex);
            }
        }
    }
}
