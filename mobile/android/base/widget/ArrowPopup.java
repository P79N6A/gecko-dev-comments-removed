




package org.mozilla.gecko.widget;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.HardwareUtils;

import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;

public class ArrowPopup extends PopupWindow {
    protected final GeckoApp mActivity;

    private View mAnchor;
    private ImageView mArrow;

    private int mArrowWidth;
    private int mYOffset;

    protected LinearLayout mContent;
    protected boolean mInflated;

    public ArrowPopup(GeckoApp aActivity, View aAnchor) {
        super(aActivity);
        mActivity = aActivity;
        mAnchor = aAnchor;

        mInflated = false;

        mArrowWidth = aActivity.getResources().getDimensionPixelSize(R.dimen.menu_popup_arrow_width);
        mYOffset = aActivity.getResources().getDimensionPixelSize(R.dimen.menu_popup_offset);

        setAnimationStyle(R.style.PopupAnimation);
    }

    public void setAnchor(View aAnchor) {
        mAnchor = aAnchor;
    }

    protected void init() {
        setBackgroundDrawable(new BitmapDrawable());
        setOutsideTouchable(true);

        setWindowLayoutMode(HardwareUtils.isTablet() ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.FILL_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.arrow_popup, null);
        setContentView(layout);

        mArrow = (ImageView) layout.findViewById(R.id.arrow);
        mContent = (LinearLayout) layout.findViewById(R.id.content);

        mInflated = true;
    }

    



    public void show() {
        int[] anchorLocation = new int[2];
        if (mAnchor != null)
            mAnchor.getLocationInWindow(anchorLocation);

        
        
        if (mAnchor == null || anchorLocation[1] < 0) {
            final View view = mActivity.getView();

            
            
            if (Build.VERSION.SDK_INT < 11) {
                setWidth(view.getWidth());
                setHeight(view.getHeight());
            }

            showAtLocation(view, Gravity.TOP, 0, 0);
            return;
        }

        
        int anchorWidth = mAnchor.getWidth() - mAnchor.getPaddingLeft() - mAnchor.getPaddingRight();
        
        
        int arrowOffset = (anchorWidth - mArrowWidth)/2 + mAnchor.getPaddingLeft();

        
        int offset = 0;

        RelativeLayout.LayoutParams arrowLayoutParams = (RelativeLayout.LayoutParams) mArrow.getLayoutParams();

        if (HardwareUtils.isTablet()) {
            
            
            
            offset = arrowOffset - arrowLayoutParams.leftMargin;
        } else {
            
            
            int leftMargin = anchorLocation[0] + arrowOffset;
            arrowLayoutParams.setMargins(leftMargin, 0, 0, 0);
        }

        if (isShowing()) {
            update(mAnchor, offset, -mYOffset, -1, -1);
        } else {
            showAsDropDown(mAnchor, offset, -mYOffset);
        }
    }
}
