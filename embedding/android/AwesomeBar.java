






































package org.mozilla.gecko;

import java.io.File;

import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.Configuration;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

public class AwesomeBar extends Activity {
    private static final String LOGTAG = "GeckoAwesomeBar";

    static final String URL_KEY = "url";
    static final String TITLE_KEY = "title";
    static final String CURRENT_URL_KEY = "currenturl";
    static final String TYPE_KEY = "type";
    static enum Type { ADD, EDIT };

    private String mType;
    private AwesomeBarTabs mAwesomeTabs;
    private EditText mText;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(LOGTAG, "creating awesomebar");

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.awesomebar_search);

        mAwesomeTabs = (AwesomeBarTabs) findViewById(R.id.awesomebar_tabs);
        mAwesomeTabs.setOnUrlOpenListener(new AwesomeBarTabs.OnUrlOpenListener() {
            public void onUrlOpen(AwesomeBarTabs tabs, String url) {
                openUrlAndFinish(url);
            }
        });

        mText = (EditText)findViewById(R.id.awesomebar_text);

        Resources resources = getResources();
        
        int padding[] = { mText.getPaddingLeft(),
                          mText.getPaddingTop(),
                          mText.getPaddingRight(),
                          mText.getPaddingBottom() };

        GeckoStateListDrawable states = new GeckoStateListDrawable();
        states.initializeFilter(GeckoApp.mBrowserToolbar.getHighlightColor());
        states.addState(new int[] { android.R.attr.state_focused }, resources.getDrawable(R.drawable.address_bar_url_pressed));
        states.addState(new int[] { android.R.attr.state_pressed }, resources.getDrawable(R.drawable.address_bar_url_pressed));
        states.addState(new int[] { }, resources.getDrawable(R.drawable.address_bar_url_default));
        mText.setBackgroundDrawable(states);

        mText.setPadding(padding[0], padding[1], padding[2], padding[3]);

        Intent intent = getIntent();
        String currentUrl = intent.getStringExtra(CURRENT_URL_KEY);
        mType = intent.getStringExtra(TYPE_KEY);
        if (currentUrl != null) {
            mText.setText(currentUrl);
            mText.selectAll();
        }

        mText.addTextChangedListener(new TextWatcher() {
            public void afterTextChanged(Editable s) {
                
            }

            public void beforeTextChanged(CharSequence s, int start, int count,
                                          int after) {
                
            }

            public void onTextChanged(CharSequence s, int start, int before,
                                      int count) {
                mAwesomeTabs.filter(s.toString());
            }
        });

        mText.setOnKeyListener(new View.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    if (event.getAction() != KeyEvent.ACTION_DOWN)
                        return true;

                    openUrlAndFinish(mText.getText().toString());
                    return true;
                } else {
                    return false;
                }
            }
        });

        mText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                }
            }
        });
    }

    @Override
    public void onConfigurationChanged(Configuration newConfiguration) {
        super.onConfigurationChanged(newConfiguration);
    }

    @Override
    public boolean onSearchRequested() {
        cancelAndFinish();
        return true;
    }

    private void cancelAndFinish() {
        setResult(Activity.RESULT_CANCELED);
        finish();
    }

    private void openUrlAndFinish(String url) {
        Intent resultIntent = new Intent();
        resultIntent.putExtra(URL_KEY, url);
        resultIntent.putExtra(TYPE_KEY, mType);

        setResult(Activity.RESULT_OK, resultIntent);
        finish();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        
        
        if (keyCode == KeyEvent.KEYCODE_BACK ||
            keyCode == KeyEvent.KEYCODE_MENU ||
            keyCode == KeyEvent.KEYCODE_SEARCH ||
            keyCode == KeyEvent.KEYCODE_DPAD_UP ||
            keyCode == KeyEvent.KEYCODE_DPAD_DOWN ||
            keyCode == KeyEvent.KEYCODE_DPAD_LEFT ||
            keyCode == KeyEvent.KEYCODE_DPAD_RIGHT ||
            keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
            return super.onKeyDown(keyCode, event);
        } else {
            
            mText.requestFocusFromTouch();
            mText.dispatchKeyEvent(event);
            return true;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mAwesomeTabs.destroy();
    }
}
