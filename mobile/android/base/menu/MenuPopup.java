




package org.mozilla.gecko.menu;

import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.PopupWindow;




public class MenuPopup extends PopupWindow {
    private final LinearLayout mPanel;

    private final int mYOffset;
    private final int mPopupWidth;
    private final int mPopupMinHeight;

    public MenuPopup(Context context) {
        super(context);

        setFocusable(true);

        mYOffset = context.getResources().getDimensionPixelSize(R.dimen.menu_popup_offset);
        mPopupWidth = context.getResources().getDimensionPixelSize(R.dimen.menu_popup_width);
        mPopupMinHeight = context.getResources().getDimensionPixelSize(R.dimen.menu_item_row_height);

        
        setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        setWindowLayoutMode(ViewGroup.LayoutParams.WRAP_CONTENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = LayoutInflater.from(context);
        mPanel = (LinearLayout) inflater.inflate(R.layout.menu_popup, null);
        setContentView(mPanel);

        setAnimationStyle(R.style.PopupAnimation);
    }

    




    public void setPanelView(View view) {
        view.setLayoutParams(new LinearLayout.LayoutParams(mPopupWidth,
                                                           LinearLayout.LayoutParams.WRAP_CONTENT));

        mPanel.removeAllViews();
        mPanel.addView(view);
    }

    


    @Override
    public void showAsDropDown(View anchor) {
        
        setHeight(mPopupMinHeight);

        
        
        final int xOffset = anchor.getWidth()/2 - mPopupWidth/2;
        showAsDropDown(anchor, xOffset, -mYOffset);
    }
}
