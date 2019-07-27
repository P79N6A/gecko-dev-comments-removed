




package org.mozilla.gecko.widget;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.HardwareUtils;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;

public abstract class ArrowPopup extends PopupWindow {
    View mAnchor;
    ImageView mArrow;

    int mArrowWidth;
    private int mYOffset;

    protected LinearLayout mContent;
    protected boolean mInflated;

    protected final Context mContext;

    public ArrowPopup(Context context) {
        super(context);

        mContext = context;

        final Resources res = context.getResources();
        mArrowWidth = res.getDimensionPixelSize(R.dimen.arrow_popup_arrow_width);
        mYOffset = res.getDimensionPixelSize(R.dimen.arrow_popup_arrow_offset);

        setAnimationStyle(R.style.PopupAnimation);
    }

    protected void init() {
        
        
        setBackgroundDrawable(new BitmapDrawable(mContext.getResources()));

        
        setOutsideTouchable(true);

        final int widthSpec = HardwareUtils.isTablet() ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.MATCH_PARENT;
        setWindowLayoutMode(widthSpec, ViewGroup.LayoutParams.WRAP_CONTENT);

        final LayoutInflater inflater = LayoutInflater.from(mContext);
        final ArrowPopupLayout layout = (ArrowPopupLayout) inflater.inflate(R.layout.arrow_popup, null);
        setContentView(layout);

        layout.mListener = new ArrowPopupLayout.OnSizeChangedListener() {
            @Override
            public void onSizeChanged() {
                if (mAnchor == null) {
                    return;
                }

                
                final int anchorWidth = mAnchor.getWidth() - mAnchor.getPaddingLeft() - mAnchor.getPaddingRight();

                
                
                final int arrowOffset = (anchorWidth - mArrowWidth) / 2 + mAnchor.getPaddingLeft();

                
                final int[] location = new int[2];
                mAnchor.getLocationOnScreen(location);
                final int anchorX = location[0];
                layout.getLocationOnScreen(location);
                final int popupX = location[0];
                final int leftMargin = anchorX - popupX + arrowOffset;

                
                
                
                
                final RelativeLayout.LayoutParams arrowLayoutParams = (RelativeLayout.LayoutParams) mArrow.getLayoutParams();
                arrowLayoutParams.setMargins(leftMargin, 0, 0, 0);
            }
        };

        mArrow = (ImageView) layout.findViewById(R.id.arrow);
        mContent = (LinearLayout) layout.findViewById(R.id.content);

        mInflated = true;
    }

    




    public void setAnchor(View anchor) {
        mAnchor = anchor;
    }

    



    public void show() {
        if (!mInflated) {
            throw new IllegalStateException("ArrowPopup#init() must be called before ArrowPopup#show()");
        }

        final int[] anchorLocation = new int[2];
        if (mAnchor != null) {
            mAnchor.getLocationInWindow(anchorLocation);
        }

        
        
        if (mAnchor == null || anchorLocation[1] < 0) {
            final View decorView = ((Activity) mContext).getWindow().getDecorView();

            
            
            if (Versions.preHC) {
                setWidth(decorView.getWidth());
                setHeight(decorView.getHeight());
            }

            showAtLocation(decorView, Gravity.NO_GRAVITY, anchorLocation[0] - mArrowWidth, 0);
            return;
        }

        
        
        
        
        if (isShowing()) {
            update(mAnchor, -mArrowWidth, -mYOffset, -1, -1);
        } else {
            showAsDropDown(mAnchor, -mArrowWidth, -mYOffset);
        }
    }

    private static class ArrowPopupLayout extends RelativeLayout {
        public interface OnSizeChangedListener {
            public void onSizeChanged();
        }

        OnSizeChangedListener mListener;

        public ArrowPopupLayout(Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, defStyle);
        }

        public ArrowPopupLayout(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public ArrowPopupLayout(Context context) {
            super(context);
        }

        @Override
        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            super.onSizeChanged(w, h, oldw, oldh);
            mListener.onSizeChanged();
        }
    }
}
