




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.PropertyAnimationListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnCommitListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnDismissListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnFilterListener;
import org.mozilla.gecko.widget.ThemedLinearLayout;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.InputMethodManager;







public class ToolbarEditLayout extends ThemedLinearLayout {

    private final ToolbarEditText mEditText;

    private OnFocusChangeListener mFocusChangeListener;

    public ToolbarEditLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        setOrientation(HORIZONTAL);

        LayoutInflater.from(context).inflate(R.layout.toolbar_edit_layout, this);
        mEditText = (ToolbarEditText) findViewById(R.id.url_edit_text);
    }

    @Override
    public void onAttachedToWindow() {
        mEditText.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (mFocusChangeListener != null) {
                    mFocusChangeListener.onFocusChange(ToolbarEditLayout.this, hasFocus);
                }
            }
        });
    }

    @Override
    public void setOnFocusChangeListener(OnFocusChangeListener listener) {
        mFocusChangeListener = listener;
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        mEditText.setEnabled(enabled);
    }

    @Override
    public void setPrivateMode(boolean isPrivate) {
        super.setPrivateMode(isPrivate);
        mEditText.setPrivateMode(isPrivate);
    }

    void setToolbarPrefs(final ToolbarPrefs prefs) {
        mEditText.setToolbarPrefs(prefs);
    }

    private void showSoftInput() {
        InputMethodManager imm =
               (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
    }

    void prepareShowAnimation(final PropertyAnimator animator) {
        if (animator == null) {
            mEditText.requestFocus();
            showSoftInput();
            return;
        }

        animator.addPropertyAnimationListener(new PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                mEditText.requestFocus();
            }

            @Override
            public void onPropertyAnimationEnd() {
                showSoftInput();
            }
        });
    }

    void setOnCommitListener(OnCommitListener listener) {
        mEditText.setOnCommitListener(listener);
    }

    void setOnDismissListener(OnDismissListener listener) {
        mEditText.setOnDismissListener(listener);
    }

    void setOnFilterListener(OnFilterListener listener) {
        mEditText.setOnFilterListener(listener);
    }

    void onEditSuggestion(String suggestion) {
        mEditText.setText(suggestion);
        mEditText.setSelection(mEditText.getText().length());
        mEditText.requestFocus();

        showSoftInput();
    }

    void setText(String text) {
        mEditText.setText(text);
    }

    String getText() {
        return mEditText.getText().toString();
    }
}
