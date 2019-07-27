




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.ActivityHandlerHelper;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.CustomEditText;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.InputMethods;
import org.mozilla.gecko.R;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnCommitListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnDismissListener;
import org.mozilla.gecko.toolbar.BrowserToolbar.OnFilterListener;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.util.StringUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.speech.RecognizerIntent;
import android.text.Editable;
import android.text.NoCopySpan;
import android.text.Selection;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.BackgroundColorSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.view.inputmethod.InputMethodManager;
import android.view.ViewParent;
import android.view.accessibility.AccessibilityEvent;
import android.widget.TextView;

import java.util.List;






public class ToolbarEditText extends CustomEditText
                             implements AutocompleteHandler {

    private static final String LOGTAG = "GeckoToolbarEditText";
    private static final NoCopySpan AUTOCOMPLETE_SPAN = new NoCopySpan.Concrete();

    private final Context mContext;

    private OnCommitListener mCommitListener;
    private OnDismissListener mDismissListener;
    private OnFilterListener mFilterListener;

    private ToolbarPrefs mPrefs;

    
    private String mAutoCompleteResult = "";
    
    private int mAutoCompletePrefixLength;
    
    private boolean mSettingAutoComplete;
    
    private Object[] mAutoCompleteSpans;
    
    private boolean mDiscardAutoCompleteResult;

    public ToolbarEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    void setOnCommitListener(OnCommitListener listener) {
        mCommitListener = listener;
    }

    void setOnDismissListener(OnDismissListener listener) {
        mDismissListener = listener;
    }

    void setOnFilterListener(OnFilterListener listener) {
        mFilterListener = listener;
    }

    @Override
    public void onAttachedToWindow() {
        setOnKeyListener(new KeyListener());
        setOnKeyPreImeListener(new KeyPreImeListener());
        setOnSelectionChangedListener(new SelectionChangeListener());
        addTextChangedListener(new TextChangeListener());
        configureCompoundDrawables();
    }

    @Override
    public void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);

        if (gainFocus) {
            resetAutocompleteState();
            return;
        }

        removeAutocomplete(getText());

        final InputMethodManager imm = InputMethods.getInputMethodManager(mContext);
        try {
            imm.restartInput(this);
            imm.hideSoftInputFromWindow(getWindowToken(), 0);
        } catch (NullPointerException e) {
            Log.e(LOGTAG, "InputMethodManagerService, why are you throwing"
                          + " a NullPointerException? See bug 782096", e);
        }
    }

    @Override
    public void setText(final CharSequence text, final TextView.BufferType type) {
        super.setText(text, type);

        
        resetAutocompleteState();
    }

    @Override
    public void sendAccessibilityEventUnchecked(AccessibilityEvent event) {
        
        
        
        if (event.getEventType() == AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED &&
            getParent() != null && !isShown()) {
            onInitializeAccessibilityEvent(event);
            dispatchPopulateAccessibilityEvent(event);
            getParent().requestSendAccessibilityEvent(this, event);
        } else {
            super.sendAccessibilityEventUnchecked(event);
        }
    }

    void setToolbarPrefs(final ToolbarPrefs prefs) {
        mPrefs = prefs;
    }

    



    private void beginSettingAutocomplete() {
        beginBatchEdit();
        mSettingAutoComplete = true;
    }

    


    private void endSettingAutocomplete() {
        mSettingAutoComplete = false;
        endBatchEdit();
    }

    


    private void resetAutocompleteState() {
        mAutoCompleteSpans = new Object[] {
            
            AUTOCOMPLETE_SPAN,
            
            new BackgroundColorSpan(getHighlightColor())
        };

        mAutoCompleteResult = "";

        
        
        mAutoCompletePrefixLength = getText().length();

        
        setCursorVisible(true);
    }

    protected String getNonAutocompleteText() {
        return getNonAutocompleteText(getText());
    }

    




    private static String getNonAutocompleteText(final Editable text) {
        final int start = text.getSpanStart(AUTOCOMPLETE_SPAN);
        if (start < 0) {
            
            return text.toString();
        }

        
        return TextUtils.substring(text, 0, start);
    }

    




    private boolean removeAutocomplete(final Editable text) {
        final int start = text.getSpanStart(AUTOCOMPLETE_SPAN);
        if (start < 0) {
            
            return false;
        }

        beginSettingAutocomplete();

        
        text.delete(start, text.length());

        
        
        mAutoCompleteResult = "";

        
        setCursorVisible(true);

        endSettingAutocomplete();
        return true;
    }

    




    private boolean commitAutocomplete(final Editable text) {
        final int start = text.getSpanStart(AUTOCOMPLETE_SPAN);
        if (start < 0) {
            
            return false;
        }

        beginSettingAutocomplete();

        
        for (final Object span : mAutoCompleteSpans) {
            text.removeSpan(span);
        }

        
        
        mAutoCompletePrefixLength = text.length();

        
        setCursorVisible(true);

        endSettingAutocomplete();

        
        if (mFilterListener != null) {
            mFilterListener.onFilter(text.toString(), null);
        }
        return true;
    }

    




    @Override
    public final void onAutocomplete(final String result) {
        
        
        if (mDiscardAutoCompleteResult) {
            return;
        }

        if (!isEnabled() || result == null) {
            mAutoCompleteResult = "";
            return;
        }

        final Editable text = getText();
        final int textLength = text.length();
        final int resultLength = result.length();
        final int autoCompleteStart = text.getSpanStart(AUTOCOMPLETE_SPAN);
        mAutoCompleteResult = result;

        if (autoCompleteStart > -1) {
            

            
            
            if (!TextUtils.regionMatches(result, 0, text, 0, autoCompleteStart)) {
                return;
            }

            beginSettingAutocomplete();

            
            
            text.replace(autoCompleteStart, textLength, result, autoCompleteStart, resultLength);

            
            if (autoCompleteStart == resultLength) {
                setCursorVisible(true);
            }

            endSettingAutocomplete();

        } else {
            

            
            
            if (resultLength <= textLength ||
                    !TextUtils.regionMatches(result, 0, text, 0, textLength)) {
                return;
            }

            final Object[] spans = text.getSpans(textLength, textLength, Object.class);
            final int[] spanStarts = new int[spans.length];
            final int[] spanEnds = new int[spans.length];
            final int[] spanFlags = new int[spans.length];

            
            for (int i = 0; i < spans.length; i++) {
                final Object span = spans[i];
                final int spanFlag = text.getSpanFlags(span);

                
                
                if ((spanFlag & Spanned.SPAN_COMPOSING) == 0 &&
                        (span != Selection.SELECTION_START) &&
                        (span != Selection.SELECTION_END)) {
                    continue;
                }

                spanStarts[i] = text.getSpanStart(span);
                spanEnds[i] = text.getSpanEnd(span);
                spanFlags[i] = spanFlag;
            }

            beginSettingAutocomplete();

            
            text.append(result, textLength, resultLength);

            
            for (final Object span : mAutoCompleteSpans) {
                text.setSpan(span, textLength, resultLength, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            
            setCursorVisible(false);

            
            
            
            
            bringPointIntoView(resultLength);

            
            for (int i = 0; i < spans.length; i++) {
                final int spanFlag = spanFlags[i];
                if (spanFlag == 0) {
                    
                    continue;
                }
                text.setSpan(spans[i], spanStarts[i], spanEnds[i], spanFlag);
            }

            endSettingAutocomplete();
        }
    }

    private static boolean hasCompositionString(Editable content) {
        Object[] spans = content.getSpans(0, content.length(), Object.class);

        if (spans != null) {
            for (Object span : spans) {
                if ((content.getSpanFlags(span) & Spanned.SPAN_COMPOSING) != 0) {
                    
                    return true;
                }
            }
        }

        return false;
    }

    





    @Override
    public InputConnection onCreateInputConnection(final EditorInfo outAttrs) {
        final InputConnection ic = super.onCreateInputConnection(outAttrs);
        if (ic == null) {
            return null;
        }

        return new InputConnectionWrapper(ic, false) {
            @Override
            public boolean deleteSurroundingText(final int beforeLength, final int afterLength) {
                if (removeAutocomplete(getText())) {
                    
                    
                    
                    
                    
                    final InputMethodManager imm = InputMethods.getInputMethodManager(mContext);
                    if (imm != null) {
                        imm.restartInput(ToolbarEditText.this);
                    }
                    return false;
                }
                return super.deleteSurroundingText(beforeLength, afterLength);
            }

            private boolean removeAutocompleteOnComposing(final CharSequence text) {
                final Editable editable = getText();
                final int composingStart = BaseInputConnection.getComposingSpanStart(editable);
                final int composingEnd = BaseInputConnection.getComposingSpanEnd(editable);
                
                
                if (composingStart >= 0 &&
                    composingEnd >= 0 &&
                    (composingEnd - composingStart) > text.length() &&
                    removeAutocomplete(editable)) {
                    
                    
                    finishComposingText();
                    setComposingRegion(composingStart, composingEnd);
                    return true;
                }
                return false;
            }

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                if (removeAutocompleteOnComposing(text)) {
                    return false;
                }
                return super.commitText(text, newCursorPosition);
            }

            @Override
            public boolean setComposingText(final CharSequence text, final int newCursorPosition) {
                if (removeAutocompleteOnComposing(text)) {
                    return false;
                }
                return super.setComposingText(text, newCursorPosition);
            }
        };
    }

    




    private void configureCompoundDrawables() {
        if (!AppConstants.NIGHTLY_BUILD || !supportsVoiceRecognizer()) {
            
            setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
            return;
        }
        setOnTouchListener(new VoiceSearchOnTouchListener());
    }

    private boolean supportsVoiceRecognizer() {
        final Intent intent = createVoiceRecognizerIntent();
        return intent.resolveActivity(getContext().getPackageManager()) != null;
    }

    private Intent createVoiceRecognizerIntent() {
        final Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH);
        intent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 1);
        intent.putExtra(RecognizerIntent.EXTRA_PROMPT, getResources().getString(R.string.voicesearch_prompt));
        return intent;
    }

    private void launchVoiceRecognizer() {
        final Intent intent = createVoiceRecognizerIntent();

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
                setText(text);
                setSelection(0, text.length());
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

    private class SelectionChangeListener implements OnSelectionChangedListener {
        @Override
        public void onSelectionChanged(final int selStart, final int selEnd) {
            
            

            final Editable text = getText();
            final int start = text.getSpanStart(AUTOCOMPLETE_SPAN);

            if (start < 0 || (start == selStart && start == selEnd)) {
                
                
                return;
            }

            if (selStart <= start && selEnd <= start) {
                
                removeAutocomplete(text);
            } else {
                
                commitAutocomplete(text);
            }
        }
    }

    private class TextChangeListener implements TextWatcher {
        @Override
        public void afterTextChanged(final Editable editable) {
            if (!isEnabled() || mSettingAutoComplete) {
                return;
            }

            final String text = getNonAutocompleteText(editable);
            final int textLength = text.length();
            boolean doAutocomplete = mPrefs.shouldAutocomplete();

            if (StringUtils.isSearchQuery(text, false)) {
                doAutocomplete = false;
            } else if (mAutoCompletePrefixLength > textLength) {
                
                doAutocomplete = false;
            }

            mAutoCompletePrefixLength = textLength;

            
            
            mDiscardAutoCompleteResult = !doAutocomplete;

            if (doAutocomplete && mAutoCompleteResult.startsWith(text)) {
                
                
                onAutocomplete(mAutoCompleteResult);
                doAutocomplete = false;
            } else {
                
                
                removeAutocomplete(editable);
            }

            if (mFilterListener != null) {
                mFilterListener.onFilter(text, doAutocomplete ? ToolbarEditText.this : null);
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count,
                                      int after) {
            
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before,
                                  int count) {
            
        }
    }

    private class KeyPreImeListener implements OnKeyPreImeListener {
        @Override
        public boolean onKeyPreIme(View v, int keyCode, KeyEvent event) {
            
            if (event.getAction() != KeyEvent.ACTION_DOWN) {
                return false;
            }

            if (keyCode == KeyEvent.KEYCODE_ENTER) {
                
                
                final Editable content = getText();
                if (!hasCompositionString(content)) {
                    if (mCommitListener != null) {
                        mCommitListener.onCommit();
                    }

                    return true;
                }
            }

            if (keyCode == KeyEvent.KEYCODE_BACK) {
                
                clearFocus();
                return true;
            }

            return false;
        }
    }

    private class KeyListener implements View.OnKeyListener {
        @Override
        public boolean onKey(View v, int keyCode, KeyEvent event) {
            if (keyCode == KeyEvent.KEYCODE_ENTER || GamepadUtils.isActionKey(event)) {
                if (event.getAction() != KeyEvent.ACTION_DOWN) {
                    return true;
                }

                if (mCommitListener != null) {
                    mCommitListener.onCommit();
                }

                return true;
            }

            if (GamepadUtils.isBackKey(event)) {
                if (mDismissListener != null) {
                    mDismissListener.onDismiss();
                }

                return true;
            }

            if ((keyCode == KeyEvent.KEYCODE_DEL ||
                (Versions.feature11Plus &&
                 keyCode == KeyEvent.KEYCODE_FORWARD_DEL)) &&
                removeAutocomplete(getText())) {
                
                return true;
            }

            return false;
        }
    }

    private class VoiceSearchOnTouchListener implements View.OnTouchListener {
        private int mVoiceSearchIconIndex = -1;
        private Drawable mVoiceSearchIcon;

        public VoiceSearchOnTouchListener() {
            Drawable[] drawables = getCompoundDrawables();
            for (int i = 0; i < drawables.length; i++) {
                if (drawables[i] != null) {
                    mVoiceSearchIcon = drawables[i];
                    mVoiceSearchIconIndex = i;
                }
            }
        }

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            boolean tapped;
            switch (mVoiceSearchIconIndex) {
                case 0:
                    tapped = event.getX() < (getPaddingLeft() + mVoiceSearchIcon.getIntrinsicWidth());
                    break;
                case 2:
                    tapped = event.getX() > (getWidth() - getPaddingRight() - mVoiceSearchIcon.getIntrinsicWidth());
                    break;
                default:
                    tapped = false;
            }
            if (tapped) {
                launchVoiceRecognizer();
            }
            return tapped;
        }
    }
}
