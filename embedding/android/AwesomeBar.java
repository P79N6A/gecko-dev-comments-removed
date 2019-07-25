






































package org.mozilla.gecko;

import java.io.File;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.EditText;

public class AwesomeBar extends Activity {
    static final String URL_KEY = "url";
    static final String TITLE_KEY = "title";
    static final String CURRENT_URL_KEY = "currenturl";
    static final String TYPE_KEY = "type";
    static enum Type { ADD, EDIT };

    private static final String LOG_NAME = "AwesomeBar";

    private String mType;
    private AwesomeBarTabs mAwesomeTabs;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(LOG_NAME, "creating awesomebar");

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.awesomebar_search);

        mAwesomeTabs = (AwesomeBarTabs) findViewById(R.id.awesomebar_tabs);
        mAwesomeTabs.setOnUrlOpenListener(new AwesomeBarTabs.OnUrlOpenListener() {
            public void onUrlOpen(AwesomeBarTabs tabs, String url) {
                Intent resultIntent = new Intent();
                resultIntent.putExtra(URL_KEY, url);
                resultIntent.putExtra(TYPE_KEY, mType);

                setResult(Activity.RESULT_OK, resultIntent);
                finish();
            }
        });

        final EditText text = (EditText)findViewById(R.id.awesomebar_text);

        Intent intent = getIntent();
        String currentUrl = intent.getStringExtra(CURRENT_URL_KEY);
        mType = intent.getStringExtra(TYPE_KEY);
        if (currentUrl != null) {
            text.setText(currentUrl);
            text.selectAll();
        }

        text.addTextChangedListener(new TextWatcher() {
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

        text.setOnKeyListener(new View.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    if (event.getAction() != KeyEvent.ACTION_DOWN)
                        return true;

                    Intent resultIntent = new Intent();
                    resultIntent.putExtra(URL_KEY, text.getText().toString());
                    resultIntent.putExtra(TYPE_KEY, mType);
                    setResult(Activity.RESULT_OK, resultIntent);
                    finish();
                    return true;
                } else {
                    return false;
                }
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mAwesomeTabs.destroy();
    }
}
