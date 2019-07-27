




package org.mozilla.gecko.toolbar;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.speech.RecognizerIntent;
import android.widget.Button;
import android.widget.ImageButton;
import org.mozilla.gecko.ActivityHandlerHelper;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.PropertyAnimationListener;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnCommitListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnDismissListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnFilterListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.TabEditingState;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.InputOptionsUtils;
import org.mozilla.gecko.widget.ThemedLinearLayout;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.InputMethodManager;

import java.util.List;







public class ToolbarEditLayout extends ThemedLinearLayout {

    private final ToolbarEditText mEditText;

    private final ImageButton mVoiceInput;
    private final ImageButton mQrCode;

    private OnFocusChangeListener mFocusChangeListener;

    private boolean showKeyboardOnFocus = false; 

    public ToolbarEditLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        setOrientation(HORIZONTAL);

        LayoutInflater.from(context).inflate(R.layout.toolbar_edit_layout, this);
        mEditText = (ToolbarEditText) findViewById(R.id.url_edit_text);

        mVoiceInput = (ImageButton) findViewById(R.id.mic);
        mQrCode = (ImageButton) findViewById(R.id.qrcode);
    }

    @Override
    public void onAttachedToWindow() {
        mEditText.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (mFocusChangeListener != null) {
                    mFocusChangeListener.onFocusChange(ToolbarEditLayout.this, hasFocus);

                    
                    if (hasFocus) {
                        if (voiceIsEnabled(getContext(), getResources().getString(R.string.voicesearch_prompt))) {
                            mVoiceInput.setVisibility(View.VISIBLE);
                        } else {
                            mVoiceInput.setVisibility(View.GONE);
                        }

                        if (qrCodeIsEnabled(getContext())) {
                            mQrCode.setVisibility(View.VISIBLE);
                        } else {
                            mQrCode.setVisibility(View.GONE);
                        }
                    }
                }
            }
        });

        mVoiceInput.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchVoiceRecognizer();
            }
        });

        mQrCode.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchQRCodeReader();
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

    


    public void onParentFocus() {
        if (showKeyboardOnFocus) {
            showKeyboardOnFocus = false;

            Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    mEditText.requestFocus();
                    showSoftInput();
                }
            });
        }

        
        if (qrCodeIsEnabled(getContext())) {
            mQrCode.setVisibility(View.VISIBLE);
        } else {
            mQrCode.setVisibility(View.GONE);
        }
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

    protected void saveTabEditingState(final TabEditingState editingState) {
        editingState.lastEditingText = mEditText.getNonAutocompleteText();
        editingState.selectionStart = mEditText.getSelectionStart();
        editingState.selectionEnd = mEditText.getSelectionEnd();
   }

    protected void restoreTabEditingState(final TabEditingState editingState) {
        mEditText.setText(editingState.lastEditingText);
        mEditText.setSelection(editingState.selectionStart, editingState.selectionEnd);
    }

    private boolean voiceIsEnabled(Context context, String prompt) {
        
        if(!AppConstants.NIGHTLY_BUILD) {
            return false;
        }
        final boolean voiceIsSupported = InputOptionsUtils.supportsVoiceRecognizer(context, prompt);
        if (!voiceIsSupported) {
            return false;
        }
        return GeckoSharedPrefs.forApp(context)
                .getBoolean(GeckoPreferences.PREFS_VOICE_INPUT_ENABLED, true);
    }

    private void launchVoiceRecognizer() {
        final Intent intent = InputOptionsUtils.createVoiceRecognizerIntent(getResources().getString(R.string.voicesearch_prompt));

        Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
        ActivityHandlerHelper.startIntentForActivity(activity, intent, new ActivityResultHandler() {
            @Override
            public void onActivityResult(int resultCode, Intent data) {
                switch (resultCode) {
                    case RecognizerIntent.RESULT_CLIENT_ERROR:
                    case RecognizerIntent.RESULT_NETWORK_ERROR:
                    case RecognizerIntent.RESULT_SERVER_ERROR:
                        
                        handleVoiceSearchError(false);
                        break;
                    case RecognizerIntent.RESULT_AUDIO_ERROR:
                    case RecognizerIntent.RESULT_NO_MATCH:
                        
                        handleVoiceSearchError(true);
                        break;
                    case Activity.RESULT_CANCELED:
                        break;
                }

                if (resultCode != Activity.RESULT_OK) {
                    return;
                }

                
                
                
                List<String> voiceStrings = data.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);
                String text = voiceStrings.get(0);
                mEditText.setText(text);
                mEditText.setSelection(0, text.length());

                final InputMethodManager imm =
                        (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
            }
        });
    }

    private void handleVoiceSearchError(boolean offerRetry) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext())
                .setTitle(R.string.voicesearch_failed_title)
                .setIcon(R.drawable.icon).setNeutralButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });

        if (offerRetry) {
            builder.setMessage(R.string.voicesearch_failed_message_recoverable)
                    .setNegativeButton(R.string.voicesearch_failed_retry, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            launchVoiceRecognizer();
                        }
                    });
        } else {
            builder.setMessage(R.string.voicesearch_failed_message);
        }

        AlertDialog dialog = builder.create();

        dialog.show();
    }

    private boolean qrCodeIsEnabled(Context context) {
        
        if(!AppConstants.NIGHTLY_BUILD) {
            return false;
        }
        final boolean qrCodeIsSupported = InputOptionsUtils.supportsQrCodeReader(context);
        if (!qrCodeIsSupported) {
            return false;
        }
        return GeckoSharedPrefs.forApp(context)
                .getBoolean(GeckoPreferences.PREFS_QRCODE_ENABLED, true);
    }

    private void launchQRCodeReader() {
        final Intent intent = InputOptionsUtils.createQRCodeReaderIntent();

        Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
        ActivityHandlerHelper.startIntentForActivity(activity, intent, new ActivityResultHandler() {
            @Override
            public void onActivityResult(int resultCode, Intent intent) {
                if (resultCode == Activity.RESULT_OK) {
                    String text = intent.getStringExtra("SCAN_RESULT");
                    if (!StringUtils.isSearchQuery(text, false)) {
                        mEditText.setText(text);
                        mEditText.selectAll();

                        
                        
                        
                        showKeyboardOnFocus = true;
                    }
                }
                
                
                
            }
        });
    }
}
