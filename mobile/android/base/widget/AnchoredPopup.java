




package org.mozilla.gecko.widget;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.R;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import org.mozilla.gecko.util.HardwareUtils;




public abstract class AnchoredPopup extends PopupWindow {
    public interface OnVisibilityChangeListener {
        public void onDoorHangerShow();
        public void onDoorHangerHide();
    }

    private View mAnchor;
    private OnVisibilityChangeListener onVisibilityChangeListener;

    protected LinearLayout mContent;
    protected boolean mInflated;

    protected final Context mContext;

    public AnchoredPopup(Context context) {
        super(context);

        mContext = context;

        setAnimationStyle(R.style.PopupAnimation);
    }

    protected void init() {
        
        
        setBackgroundDrawable(new BitmapDrawable(mContext.getResources()));

        
        setOutsideTouchable(true);

        
        int width = (int) mContext.getResources().getDimension(R.dimen.doorhanger_width);
        setWindowLayoutMode(0, ViewGroup.LayoutParams.WRAP_CONTENT);
        setWidth(width);

        final LayoutInflater inflater = LayoutInflater.from(mContext);
        final View layout = inflater.inflate(R.layout.anchored_popup, null);
        setContentView(layout);

        mContent = (LinearLayout) layout.findViewById(R.id.content);

        mInflated = true;
    }

    




    public void setAnchor(View anchor) {
        mAnchor = anchor;
    }

    public void setOnVisibilityChangeListener(OnVisibilityChangeListener listener) {
        onVisibilityChangeListener = listener;
    }

    



    public void show() {
        if (!mInflated) {
            throw new IllegalStateException("ArrowPopup#init() must be called before ArrowPopup#show()");
        }

        if (onVisibilityChangeListener != null) {
            onVisibilityChangeListener.onDoorHangerShow();
        }

        final int[] anchorLocation = new int[2];
        if (mAnchor != null) {
            mAnchor.getLocationInWindow(anchorLocation);
        }

        
        int offsetY = mContext.getResources().getDimensionPixelOffset(R.dimen.doorhanger_offsetY);
        final View decorView = ((Activity) mContext).getWindow().getDecorView();

        
        
        
        if (Versions.preHC) {
            setWidth(decorView.getWidth());
            offsetY = mContext.getResources().getDimensionPixelOffset(R.dimen.doorhanger_GB_offsetY);
            if (mAnchor == null) {
              mAnchor = decorView;
            }
            showAsDropDown(mAnchor, 0, -offsetY);
            return;
        }

        final boolean validAnchor = (mAnchor != null) && (anchorLocation[1] > 0);
        if (HardwareUtils.isTablet()) {
            if (validAnchor) {
                showAsDropDown(mAnchor, 0, 0);
            } else {
                
                
                final int offsetX = mContext.getResources().getDimensionPixelOffset(R.dimen.doorhanger_offsetX);
                showAtLocation(decorView, Gravity.TOP | Gravity.LEFT, offsetX, offsetY);
            }
        } else {
            
            
            final View anchor = validAnchor ? mAnchor : decorView;

            showAtLocation(anchor, Gravity.TOP | Gravity.CENTER_HORIZONTAL, 0, offsetY);
        }
    }

    @Override
    public void dismiss() {
        super.dismiss();
        if (onVisibilityChangeListener != null) {
            onVisibilityChangeListener.onDoorHangerHide();
        }
    }
}
