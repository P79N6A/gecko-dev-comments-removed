




































package org.mozilla.gecko;

import android.content.Context;
import android.widget.TextView;
import android.widget.Button;
import android.widget.PopupWindow;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;

public class DoorHangerPopup extends PopupWindow {
    private Context mContext;
    private LinearLayout mChoicesLayout;
    private TextView mTextView;
    private Button mButton;
    private LayoutParams mLayoutParams;
    private View popupView;
    public int mTabId;
    
    private String mValue;
    private final int POPUP_VERTICAL_SIZE = 100;

    public DoorHangerPopup(Context aContext, String aValue) {
        super(aContext);
        mContext = aContext;
        mValue = aValue;

        LayoutInflater inflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        popupView = (View) inflater.inflate(R.layout.doorhangerpopup, null);
        setContentView(popupView);

        mChoicesLayout = (LinearLayout) popupView.findViewById(R.id.doorhanger_choices);
        mTextView = (TextView) popupView.findViewById(R.id.doorhanger_title);

        mLayoutParams = new LayoutParams(LayoutParams.WRAP_CONTENT,
                                         LayoutParams.WRAP_CONTENT);
    }

    public void addButton(String aText, int aCallback) {
        final String sCallback = Integer.toString(aCallback);

        Button mButton = new Button(mContext);
        mButton.setText(aText);
        mButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                GeckoEvent e = new GeckoEvent("Doorhanger:Reply", sCallback);
                GeckoAppShell.sendEventToGecko(e);
                dismiss();
            }
        });
        mChoicesLayout.addView(mButton, mLayoutParams);
    }

    public String getValue() {
        return mValue;
    }

    public void setText(String aText) {
        mTextView.setText(aText);
    }

    public void setTab(int tabId) {
        mTabId = tabId;
    }

    public void showAtHeight(int y) {
        Display display = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();

        int width = display.getWidth();
        int height = display.getHeight();
        showAsDropDown(popupView);
        update(0, height - POPUP_VERTICAL_SIZE - y,
               width, POPUP_VERTICAL_SIZE);
    }
}
