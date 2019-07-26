



package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TabWidget;

public class IconTabWidget extends TabWidget {
    private OnTabChangedListener mListener;
    private final int mButtonLayoutId;

    public static interface OnTabChangedListener {
        public void onTabChanged(int tabIndex);
    }

    public IconTabWidget(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.IconTabWidget);
        mButtonLayoutId = a.getResourceId(R.styleable.IconTabWidget_android_layout, 0);
        a.recycle();

        if (mButtonLayoutId == 0) {
            throw new RuntimeException("You must supply layout attribute");
        }
    }

    public ImageButton addTab(int resId) {
        ImageButton button = (ImageButton) LayoutInflater.from(getContext()).inflate(mButtonLayoutId, null);
        button.setImageResource(resId);

        addView(button);
        button.setOnClickListener(new TabClickListener(getTabCount() - 1));
        button.setOnFocusChangeListener(this);
        return button;
    }

    public void setTabSelectionListener(OnTabChangedListener listener) {
        mListener = listener;
    }

    @Override
    public void onFocusChange(View view, boolean hasFocus) {
    }

    private class TabClickListener implements OnClickListener {
        private final int mIndex;

        public TabClickListener(int index) {
            mIndex = index;
        }

        @Override
        public void onClick(View view) {
            if (mListener != null)
                mListener.onTabChanged(mIndex);
        }
    }
}
