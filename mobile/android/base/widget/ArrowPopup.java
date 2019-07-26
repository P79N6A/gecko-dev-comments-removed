




package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;

public class ArrowPopup extends PopupWindow {
    protected LayoutInflater mInflater;
    protected boolean mInflated;

    protected LinearLayout mContent;
    protected ImageView mArrow;

    protected int mArrowWidth;
    protected int mYOffset;

    public ArrowPopup(Context aContext) {
        super(aContext);
        mInflater = LayoutInflater.from(aContext);

        mInflated = false;
        mArrowWidth = aContext.getResources().getDimensionPixelSize(R.dimen.menu_popup_arrow_width);
        mYOffset = aContext.getResources().getDimensionPixelSize(R.dimen.menu_popup_offset);

        setAnimationStyle(R.style.PopupAnimation);
    }

    protected void init() {
        setBackgroundDrawable(new BitmapDrawable());
        setOutsideTouchable(true);

        setWindowLayoutMode(HardwareUtils.isTablet() ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.FILL_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);

        RelativeLayout layout = (RelativeLayout) mInflater.inflate(R.layout.arrow_popup, null);
        setContentView(layout);

        mArrow = (ImageView) layout.findViewById(R.id.arrow);
        mContent = (LinearLayout) layout.findViewById(R.id.content);

        mInflated = true;
    }
}
