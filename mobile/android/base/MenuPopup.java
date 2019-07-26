




package org.mozilla.gecko;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;




public class MenuPopup extends PopupWindow {
    private Resources mResources;

    private ImageView mArrow;
    private RelativeLayout mPanel;

    private int mYOffset;
    private int mArrowMargin;
    private int mPopupWidth;

    public MenuPopup(Context context) {
        super(context);
        mResources = context.getResources();

        setFocusable(true);

        mYOffset = mResources.getDimensionPixelSize(R.dimen.menu_popup_offset);
        mArrowMargin = mResources.getDimensionPixelSize(R.dimen.menu_popup_arrow_margin);
        mPopupWidth = mResources.getDimensionPixelSize(R.dimen.menu_popup_width);

        
        setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        setWindowLayoutMode(View.MeasureSpec.makeMeasureSpec(mPopupWidth, View.MeasureSpec.AT_MOST),
                            ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = LayoutInflater.from(context);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.menu_popup, null);
        setContentView(layout);

        mArrow = (ImageView) layout.findViewById(R.id.menu_arrow);
        mPanel = (RelativeLayout) layout.findViewById(R.id.menu_panel);
    }

    




    public void setPanelView(View view) {
        mPanel.removeAllViews();
        mPanel.addView(view);
    }

    


    @Override
    public void showAsDropDown(View anchor) {
        int[] anchorLocation = new int[2];
        anchor.getLocationOnScreen(anchorLocation);

        int screenWidth = mResources.getDisplayMetrics().widthPixels;
        int arrowWidth = mResources.getDimensionPixelSize(R.dimen.menu_popup_arrow_width);
        LayoutParams params = (LayoutParams) mArrow.getLayoutParams();
        int arrowOffset = (anchor.getWidth() - arrowWidth)/2;
       
        if (anchorLocation[0] + mPopupWidth <= screenWidth) {
            
            params.rightMargin = mPopupWidth - anchor.getWidth() + arrowOffset;
        } else {
            
            params.rightMargin = mArrowMargin;
        }

        showAsDropDown(anchor, 0, -mYOffset);
    }
}
