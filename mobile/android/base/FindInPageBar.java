



package org.mozilla.gecko;

import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.content.Context;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.LinearLayout;

public class FindInPageBar extends LinearLayout implements TextWatcher, View.OnClickListener, GeckoEventListener  {
    private static final String REQUEST_ID = "FindInPageBar";

    private final Context mContext;
    private CustomEditText mFindText;
    private boolean mInflated = false;

    public FindInPageBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        setFocusable(true);
    }

    public void inflateContent() {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View content = inflater.inflate(R.layout.find_in_page_content, this);

        content.findViewById(R.id.find_prev).setOnClickListener(this);
        content.findViewById(R.id.find_next).setOnClickListener(this);
        content.findViewById(R.id.find_close).setOnClickListener(this);

        
        
        content.setOnClickListener(this);

        mFindText = (CustomEditText) content.findViewById(R.id.find_text);
        mFindText.addTextChangedListener(this);
        mFindText.setOnKeyPreImeListener(new CustomEditText.OnKeyPreImeListener() {
            @Override
            public boolean onKeyPreIme(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_BACK) {
                    hide();
                    return true;
                }
                return false;
            }
        });

        mInflated = true;
        GeckoAppShell.getEventDispatcher().registerEventListener("TextSelection:Data", this);
    }

    public void show() {
        if (!mInflated)
            inflateContent();

        setVisibility(VISIBLE);
        mFindText.requestFocus();

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("TextSelection:Get", REQUEST_ID));
    }

    public void hide() {
        setVisibility(GONE);
        getInputMethodManager(mFindText).hideSoftInputFromWindow(mFindText.getWindowToken(), 0);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FindInPage:Closed", null));
    }

    private InputMethodManager getInputMethodManager(View view) {
        Context context = view.getContext();
        return (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
     }

    public void onDestroy() {
        GeckoAppShell.getEventDispatcher().unregisterEventListener("TextSelection:Data", this);
    }

    

    @Override
    public void afterTextChanged(Editable s) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FindInPage:Find", s.toString()));
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        
    }

    

    @Override
    public void onClick(View v) {
        final int viewId = v.getId();

        if (viewId == R.id.find_prev) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FindInPage:Prev", mFindText.getText().toString()));
            getInputMethodManager(mFindText).hideSoftInputFromWindow(mFindText.getWindowToken(), 0);
            return;
        }

        if (viewId == R.id.find_next) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FindInPage:Next", mFindText.getText().toString()));
            getInputMethodManager(mFindText).hideSoftInputFromWindow(mFindText.getWindowToken(), 0);
            return;
        }

        if (viewId == R.id.find_close) {
            hide();
        }
    }

    

    @Override
    public void handleMessage(String event, JSONObject message) {
        if (!event.equals("TextSelection:Data") || !REQUEST_ID.equals(message.optString("requestId"))) {
            return;
        }

        final String text = message.optString("text");

        
        if (!TextUtils.isEmpty(text)) {
            
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    mFindText.setText(text);
                }
            });
            return;
        }

        
        if (mFindText.hasWindowFocus()) {
            getInputMethodManager(mFindText).showSoftInput(mFindText, 0);
        } else {
            
            mFindText.setOnWindowFocusChangeListener(new CustomEditText.OnWindowFocusChangeListener() {
                @Override
                public void onWindowFocusChanged(boolean hasFocus) {
                    if (!hasFocus)
                        return;

                    mFindText.setOnWindowFocusChangeListener(null);
                    getInputMethodManager(mFindText).showSoftInput(mFindText, 0);
               }
            });
        }
    }
}
