




package org.mozilla.gecko;

import org.mozilla.gecko.widget.Divider;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.text.style.URLSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class DoorHanger extends LinearLayout implements Button.OnClickListener {
    private static final String LOGTAG = "GeckoDoorHanger";

    
    private DoorHangerPopup mPopup;
    private LinearLayout mChoicesLayout;
    private TextView mTextView;
    private List<PromptInput> mInputs;

    private static int sInputPadding = -1;
    private static int sSpinnerTextColor = -1;
    private static int sSpinnerTextSize = -1;

    
    static private LayoutParams mLayoutParams;
    private final int mTabId;
    
    private final String mValue;

    
    private CheckBox mCheckBox;

    
    private View mDivider;

    private int mPersistence = 0;
    private boolean mPersistWhileVisible = false;
    private long mTimeout = 0;

    DoorHanger(Context context, DoorHangerPopup popup, int tabId, String value) {
        super(context);

        mPopup = popup;
        mTabId = tabId;
        mValue = value;

        if (sInputPadding == -1) {
            sInputPadding = getResources().getDimensionPixelSize(R.dimen.doorhanger_padding_spinners);
        }
        if (sSpinnerTextColor == -1) {
            sSpinnerTextColor = getResources().getColor(R.color.text_color_primary_disable_only);
        }
        if (sSpinnerTextSize == -1) {
            sSpinnerTextSize = getResources().getDimensionPixelSize(R.dimen.doorhanger_spinner_textsize);
        }
    }
 
    int getTabId() {
        return mTabId;
    }

    String getValue() {
        return mValue;
    }

    public void showDivider() {
        mDivider.setVisibility(View.VISIBLE);
    }

    public void hideDivider() {
        mDivider.setVisibility(View.GONE);
    }

    
    void init(String message, JSONArray buttons, JSONObject options) {
        setOrientation(VERTICAL);

        LayoutInflater.from(getContext()).inflate(R.layout.doorhanger, this);
        setVisibility(View.GONE);

        mTextView = (TextView) findViewById(R.id.doorhanger_title);
        mTextView.setText(message);

        mChoicesLayout = (LinearLayout) findViewById(R.id.doorhanger_choices);

        mDivider = findViewById(R.id.divider_doorhanger);

        
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
            findViewById(R.id.divider_choices).setVisibility(View.VISIBLE);
            mChoicesLayout.setVisibility(View.VISIBLE);
        }

        setOptions(options);
    }

    private void addButton(String aText, int aCallback) {
        if (mLayoutParams == null)
            mLayoutParams = new LayoutParams(LayoutParams.FILL_PARENT,
                                             LayoutParams.FILL_PARENT,
                                             1.0f);

        Button button = (Button) LayoutInflater.from(getContext()).inflate(R.layout.doorhanger_button, null);
        button.setText(aText);
        button.setTag(Integer.toString(aCallback));
        button.setOnClickListener(this);

        if (mChoicesLayout.getChildCount() > 0) {
            Divider divider = new Divider(getContext(), null);
            divider.setOrientation(Divider.Orientation.VERTICAL);
            divider.setBackgroundColor(0xFFD1D5DA);
            mChoicesLayout.addView(divider);
        }

        mChoicesLayout.addView(button, mLayoutParams);
    }

    @Override
    public void onClick(View v) {
        JSONObject response = new JSONObject();
        try {
            response.put("callback", v.getTag().toString());

            
            if (mCheckBox != null)
                response.put("checked", mCheckBox.isChecked());

            if (mInputs != null) {
                JSONObject inputs = new JSONObject();
                for (PromptInput input : mInputs) {
                    inputs.put(input.getId(), input.getValue());
                }
                response.put("inputs", inputs);
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error creating onClick response", e);
        }

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Doorhanger:Reply", response.toString());
        GeckoAppShell.sendEventToGecko(e);
        mPopup.removeDoorHanger(this);

        
        
        mPopup.updatePopup();
    }

    private void setOptions(final JSONObject options) {
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
                    Tabs.getInstance().loadUrlInTab(this.getURL());
                }
            };

            
            ForegroundColorSpan colorSpan = new ForegroundColorSpan(mTextView.getCurrentTextColor());
            titleWithLink.setSpan(colorSpan, 0, title.length(), 0);

            titleWithLink.setSpan(linkSpan, title.length() + 1, titleWithLink.length(), 0);
            mTextView.setText(titleWithLink);
            mTextView.setMovementMethod(LinkMovementMethod.getInstance());
        } catch (JSONException e) { }

        final JSONArray inputs = options.optJSONArray("inputs");
        if (inputs != null) {
            mInputs = new ArrayList<PromptInput>();

            final ViewGroup group = (ViewGroup) findViewById(R.id.doorhanger_inputs);
            group.setVisibility(VISIBLE);

            for (int i = 0; i < inputs.length(); i++) {
                try {
                    PromptInput input = PromptInput.getInput(inputs.getJSONObject(i));
                    mInputs.add(input);

                    View v = input.getView(getContext());
                    styleInput(input, v);
                    group.addView(v);
                } catch(JSONException ex) { }
            }
        }

        try {
            String checkBoxText = options.getString("checkbox");
            mCheckBox = (CheckBox) findViewById(R.id.doorhanger_checkbox);
            mCheckBox.setText(checkBoxText);
            mCheckBox.setVisibility(VISIBLE);
        } catch (JSONException e) { }
    }

    private void styleInput(PromptInput input, View view) {
        if (input instanceof PromptInput.MenulistInput) {
            styleSpinner(input, view);
        } else {
            
            view.setPadding(0, sInputPadding,
                            0, sInputPadding);
        }
    }

    private void styleSpinner(PromptInput input, View view) {
        PromptInput.MenulistInput spinInput = (PromptInput.MenulistInput) input;

        












        
        
        Spinner spinner = spinInput.spinner;
        SpinnerAdapter adapter = spinner.getAdapter();
        View dropView = adapter.getView(0, null, spinner);
        int textPadding = 0;
        if (dropView != null) {
            textPadding = dropView.getPaddingLeft();
        }

        
        Rect rect = new Rect();
        spinner.getBackground().getPadding(rect);

        
        view.setPadding(sInputPadding - rect.left - textPadding, 0, rect.right, sInputPadding);

        if (spinInput.textView != null) {
            spinInput.textView.setTextColor(sSpinnerTextColor);
            spinInput.textView.setTextSize(sSpinnerTextSize);

            
            spinInput.textView.setPadding(rect.left + textPadding, 0, 0, 0);
        }
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
