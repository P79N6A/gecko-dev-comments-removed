





































package org.mozilla.gecko;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONObject;
import org.json.JSONException;

public class DoorHanger extends LinearLayout implements Button.OnClickListener {
    private Context mContext;
    private LinearLayout mChoicesLayout;
    private TextView mTextView;
    static private LayoutParams mLayoutParams;
    public Tab mTab;
    
    private String mValue;

    static private LayoutInflater mInflater;

    private int mPersistence = 0;
    private long mTimeout = 0;

    public DoorHanger(Context aContext, String aValue) {
        super(aContext);

        mContext = aContext;
        mValue = aValue;

        setOrientation(VERTICAL);
        setBackgroundResource(R.drawable.doorhanger_shadow_bg);

        if (mInflater == null)
            mInflater = LayoutInflater.from(mContext);

        mInflater.inflate(R.layout.doorhanger, this);
        hide();

        mTextView = (TextView) findViewById(R.id.doorhanger_title);
        mChoicesLayout = (LinearLayout) findViewById(R.id.doorhanger_choices);

        if (mLayoutParams == null)
            mLayoutParams = new LayoutParams(LayoutParams.FILL_PARENT,
                                             LayoutParams.FILL_PARENT,
                                             1.0f);
    }

    public void addButton(String aText, int aCallback) {
        Button mButton = new Button(mContext);
        mButton.setText(aText);
        mButton.setTag(Integer.toString(aCallback));
        mButton.setOnClickListener(this);
        mChoicesLayout.addView(mButton, mLayoutParams);
    }

    public void onClick(View v) {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Doorhanger:Reply", v.getTag().toString());
        GeckoAppShell.sendEventToGecko(e);
        mTab.removeDoorHanger(mValue);

        
        
        GeckoApp.mDoorHangerPopup.updatePopup();
    }

    public void show() {
        setVisibility(View.VISIBLE);
    }

    public void hide() {
        setVisibility(View.GONE);
    }

    public boolean isVisible() {
        return getVisibility() == View.VISIBLE;
    }

    public String getValue() {
        return mValue;
    }

    public void setText(String aText) {
        mTextView.setText(aText);
    }

    public Tab getTab() {
        return mTab;
    }

    public void setTab(Tab tab) {
        mTab = tab;
    }

    public void setOptions(JSONObject options) {
        try {
            mPersistence = options.getInt("persistence");
        } catch (JSONException e) { }

        try {
            mTimeout = options.getLong("timeout");
        } catch (JSONException e) { }
    }

    
    
    public boolean shouldRemove() {
        
        
        if (mPersistence != 0) {
            mPersistence--;
            return false;
        }

        if (System.currentTimeMillis() <= mTimeout) {
            return false;
        }

        return true;
    }
}
