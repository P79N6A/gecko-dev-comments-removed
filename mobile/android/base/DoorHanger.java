




package org.mozilla.gecko;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.text.style.URLSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;

public class DoorHanger extends LinearLayout implements Button.OnClickListener {
    private static final String LOGTAG = "GeckoDoorHanger";

    private GeckoApp mActivity;
    
    private DoorHangerPopup mPopup;
    private LinearLayout mChoicesLayout;
    private TextView mTextView;

    
    static private LayoutParams mLayoutParams;
    private final int mTabId;
    
    private final String mValue;

    
    private CheckBox mCheckBox;

    private int mPersistence = 0;
    private boolean mPersistWhileVisible = false;
    private long mTimeout = 0;

    DoorHanger(GeckoApp aActivity, DoorHangerPopup aPopup, int aTabId, String aValue) {
        super(aActivity);

        mActivity = aActivity;
        mPopup = aPopup;
        mTabId = aTabId;
        mValue = aValue;
    }
 
    int getTabId() {
        return mTabId;
    }

    String getValue() {
        return mValue;
    }

    
    void init(String message, JSONArray buttons, JSONObject options) {
        setOrientation(VERTICAL);
        setBackgroundResource(R.drawable.doorhanger_shadow_bg);

        LayoutInflater.from(mActivity).inflate(R.layout.doorhanger, this);
        setVisibility(View.GONE);

        mTextView = (TextView) findViewById(R.id.doorhanger_title);
        mTextView.setText(message);

        mChoicesLayout = (LinearLayout) findViewById(R.id.doorhanger_choices);

        
        for (int i = 0; i < buttons.length(); i++) {
            try {
                JSONObject buttonObject = buttons.getJSONObject(i);
                String label = buttonObject.getString("label");
                int callBackId = buttonObject.getInt("callback");
                addButton(label, callBackId);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error creating doorhanger button", e);
            }
         }

        
        if (buttons.length() > 0) {
            mChoicesLayout.setVisibility(LinearLayout.VISIBLE);
        }

        setOptions(options);
    }

    private void addButton(String aText, int aCallback) {
        if (mLayoutParams == null)
            mLayoutParams = new LayoutParams(LayoutParams.FILL_PARENT,
                                             LayoutParams.FILL_PARENT,
                                             1.0f);

        Button mButton = new Button(mActivity);
        mButton.setText(aText);
        mButton.setTag(Integer.toString(aCallback));
        mButton.setOnClickListener(this);
        mChoicesLayout.addView(mButton, mLayoutParams);
    }

    public void onClick(View v) {
        JSONObject response = new JSONObject();
        try {
            response.put("callback", v.getTag().toString());

            
            if (mCheckBox != null)
                response.put("checked", mCheckBox.isChecked());
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error creating onClick response", e);
        }

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Doorhanger:Reply", response.toString());
        GeckoAppShell.sendEventToGecko(e);
        mPopup.removeDoorHanger(this);

        
        
        mPopup.updatePopup();
    }

    private void setOptions(JSONObject options) {
        try {
            mPersistence = options.getInt("persistence");
        } catch (JSONException e) { }

        try {
            mPersistWhileVisible = options.getBoolean("persistWhileVisible");
        } catch (JSONException e) { }

        try {
            mTimeout = options.getLong("timeout");
        } catch (JSONException e) { }

        try {
            JSONObject link = options.getJSONObject("link");
            String title = mTextView.getText().toString();
            String linkLabel = link.getString("label");
            String linkUrl = link.getString("url");
            SpannableString titleWithLink = new SpannableString(title + " " + linkLabel);
            URLSpan linkSpan = new URLSpan(linkUrl) {
                @Override
                public void onClick(View view) {
                    mActivity.loadUrlInTab(this.getURL());
                }
            };

            
            ForegroundColorSpan colorSpan = new ForegroundColorSpan(mTextView.getCurrentTextColor());
            titleWithLink.setSpan(colorSpan, 0, title.length(), 0);

            titleWithLink.setSpan(linkSpan, title.length() + 1, titleWithLink.length(), 0);
            mTextView.setText(titleWithLink);
            mTextView.setMovementMethod(LinkMovementMethod.getInstance());
        } catch (JSONException e) { }

        try {
            String checkBoxText = options.getString("checkbox");
            mCheckBox = (CheckBox) findViewById(R.id.doorhanger_checkbox);
            mCheckBox.setText(checkBoxText);
            mCheckBox.setVisibility(VISIBLE);
        } catch (JSONException e) { }
    }

    
    
    boolean shouldRemove() {
        if (mPersistWhileVisible && mPopup.isShowing()) {
            
            if (mPersistence != 0)
                mPersistence--;
            return false;
        }

        
        
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
