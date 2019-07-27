



package org.mozilla.search.autocomplete;

import android.content.Context;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.TextView;

import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.search.R;

public class ClearableEditText extends FrameLayout {

    private EditText editText;
    private ImageButton clearButton;
    private InputMethodManager inputMethodManager;

    private TextListener listener;

    private boolean active;

    public interface TextListener {
        public void onChange(String text);
        public void onSubmit(String text);
        public void onFocusChange(boolean hasFocus);
    }

    public ClearableEditText(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.clearable_edit_text, this);

        editText = (EditText) findViewById(R.id.edit_text);
        editText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (listener != null) {
                    listener.onChange(s.toString());
                }

                updateClearButtonVisibility();
            }
        });

        
        editText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (listener != null && actionId == EditorInfo.IME_ACTION_SEARCH) {
                    
                    Telemetry.sendUIEvent(TelemetryContract.Event.SEARCH, TelemetryContract.Method.ACTIONBAR, "text");
                    listener.onSubmit(v.getText().toString());
                    return true;
                }
                return false;
            }
        });

        editText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (listener != null) {
                    listener.onFocusChange(hasFocus);
                }
            }
        });

        clearButton = (ImageButton) findViewById(R.id.clear_button);
        clearButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                editText.setText("");
            }
        });

        inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
    }

    public void setText(String text) {
        editText.setText(text);

        
        editText.setSelection(text.length());
    }

    public String getText() {
        return editText.getText().toString();
    }

    public void setActive(boolean active) {
        if (this.active == active) {
            return;
        }
        this.active = active;

        updateClearButtonVisibility();

        editText.setFocusable(active);
        editText.setFocusableInTouchMode(active);

        final int leftDrawable = active ? R.drawable.search_icon_active : R.drawable.search_icon_inactive;
        editText.setCompoundDrawablesWithIntrinsicBounds(leftDrawable, 0, 0, 0);

        if (active) {
            editText.requestFocus();
            inputMethodManager.showSoftInput(editText, InputMethodManager.SHOW_IMPLICIT);
        } else {
            editText.clearFocus();
            inputMethodManager.hideSoftInputFromWindow(editText.getWindowToken(), 0);
        }
    }

    private void updateClearButtonVisibility() {
        
        final boolean visible = active && (editText.getText().length() > 0);
        clearButton.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void setTextListener(TextListener listener) {
        this.listener = listener;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent e) {
        
        
        
        return !active;
    }
}
