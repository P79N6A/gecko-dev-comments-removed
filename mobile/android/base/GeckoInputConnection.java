




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.InputConnectionHandler;

import android.R;
import android.content.Context;
import android.os.Build;
import android.os.SystemClock;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.method.KeyListener;
import android.text.method.TextKeyListener;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

class GeckoInputConnection
    extends BaseInputConnection
    implements InputConnectionHandler, GeckoEditableListener {

    private static final boolean DEBUG = false;
    protected static final String LOGTAG = "GeckoInputConnection";

    private static final int INLINE_IME_MIN_DISPLAY_SIZE = 480;

    private static int mIMEState;
    private static String mIMETypeHint = "";
    private static String mIMEModeHint = "";
    private static String mIMEActionHint = "";

    private String mCurrentInputMethod;

    private final GeckoEditableClient mEditableClient;
    protected int mBatchEditCount;
    private ExtractedTextRequest mUpdateRequest;
    private final ExtractedText mUpdateExtract = new ExtractedText();
    private boolean mBatchSelectionChanged;
    private boolean mBatchTextChanged;
    private long mLastRestartInputTime;
    private final InputConnection mPluginInputConnection;

    public static GeckoEditableListener create(View targetView,
                                               GeckoEditableClient editable) {
        if (DEBUG)
            return DebugGeckoInputConnection.create(targetView, editable);
        else
            return new GeckoInputConnection(targetView, editable);
    }

    protected GeckoInputConnection(View targetView,
                                   GeckoEditableClient editable) {
        super(targetView, true);
        mEditableClient = editable;
        mIMEState = IME_STATE_DISABLED;
        
        mPluginInputConnection = new BaseInputConnection(targetView, false);
    }

    @Override
    public synchronized boolean beginBatchEdit() {
        mBatchEditCount++;
        mEditableClient.setUpdateGecko(false);
        return true;
    }

    @Override
    public synchronized boolean endBatchEdit() {
        if (mBatchEditCount > 0) {
            mBatchEditCount--;
            if (mBatchEditCount == 0) {
                if (mBatchTextChanged) {
                    notifyTextChange();
                    mBatchTextChanged = false;
                }
                if (mBatchSelectionChanged) {
                    Editable editable = getEditable();
                    notifySelectionChange(Selection.getSelectionStart(editable),
                                           Selection.getSelectionEnd(editable));
                    mBatchSelectionChanged = false;
                }
                mEditableClient.setUpdateGecko(true);
            }
        } else {
            Log.w(LOGTAG, "endBatchEdit() called, but mBatchEditCount == 0?!");
        }
        return true;
    }

    @Override
    public Editable getEditable() {
        return mEditableClient.getEditable();
    }

    @Override
    public boolean performContextMenuAction(int id) {
        Editable editable = getEditable();
        int selStart = Selection.getSelectionStart(editable);
        int selEnd = Selection.getSelectionEnd(editable);

        switch (id) {
            case R.id.selectAll:
                setSelection(0, editable.length());
                break;
            case R.id.cut:
                
                if (selStart == selEnd) {
                    
                    GeckoAppShell.setClipboardText(editable.toString());
                    editable.clear();
                } else {
                    GeckoAppShell.setClipboardText(
                            editable.toString().substring(
                                Math.min(selStart, selEnd),
                                Math.max(selStart, selEnd)));
                    editable.delete(selStart, selEnd);
                }
                break;
            case R.id.paste:
                commitText(GeckoAppShell.getClipboardText(), 1);
                break;
            case R.id.copy:
                
                String copiedText = selStart == selEnd ? "" :
                                    editable.toString().substring(
                                        Math.min(selStart, selEnd),
                                        Math.max(selStart, selEnd));
                GeckoAppShell.setClipboardText(copiedText);
                break;
        }
        return true;
    }

    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest req, int flags) {
        if (req == null)
            return null;

        if ((flags & GET_EXTRACTED_TEXT_MONITOR) != 0)
            mUpdateRequest = req;

        Editable editable = getEditable();
        int selStart = Selection.getSelectionStart(editable);
        int selEnd = Selection.getSelectionEnd(editable);

        ExtractedText extract = new ExtractedText();
        extract.flags = 0;
        extract.partialStartOffset = -1;
        extract.partialEndOffset = -1;
        extract.selectionStart = selStart;
        extract.selectionEnd = selEnd;
        extract.startOffset = 0;
        extract.text = editable;

        return extract;
    }

    private static View getView() {
        return GeckoApp.mAppContext.getLayerView();
    }

    private static InputMethodManager getInputMethodManager() {
        View view = getView();
        if (view == null) {
            return null;
        }
        Context context = view.getContext();
        return InputMethods.getInputMethodManager(context);
    }

    private static void showSoftInput() {
        final InputMethodManager imm = getInputMethodManager();
        if (imm != null) {
            final View v = getView();
            imm.showSoftInput(v, 0);
        }
    }

    private static void hideSoftInput() {
        final InputMethodManager imm = getInputMethodManager();
        if (imm != null) {
            final View v = getView();
            imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
        }
    }

    private void restartInput() {
        
        
        long time = SystemClock.uptimeMillis();
        if (time < mLastRestartInputTime + 200) {
            return;
        }
        mLastRestartInputTime = time;

        final InputMethodManager imm = getInputMethodManager();
        if (imm == null) {
            return;
        }
        final View v = getView();
        
        
        
        
        
        
        
        if (InputMethods.needsSoftResetWorkaround(mCurrentInputMethod)) {
            
            
            
            
            notifySelectionChange(-1, -1);
        }
        imm.restartInput(v);
    }

    public void onTextChange(String text, int start, int oldEnd, int newEnd) {

        if (mUpdateRequest == null) {
            return;
        }

        if (mBatchEditCount > 0) {
            
            mBatchTextChanged = true;
            return;
        }
        notifyTextChange();
    }

    private void notifyTextChange() {

        final InputMethodManager imm = getInputMethodManager();
        if (imm == null) {
            return;
        }
        final View v = getView();
        final Editable editable = getEditable();

        mUpdateExtract.flags = 0;
        
        mUpdateExtract.partialStartOffset = -1;
        mUpdateExtract.partialEndOffset = -1;
        mUpdateExtract.selectionStart =
                Selection.getSelectionStart(editable);
        mUpdateExtract.selectionEnd =
                Selection.getSelectionEnd(editable);
        mUpdateExtract.startOffset = 0;
        mUpdateExtract.text = editable;

        imm.updateExtractedText(v, mUpdateRequest.token,
                                mUpdateExtract);
    }

    public void onSelectionChange(int start, int end) {

        if (mBatchEditCount > 0) {
            
            mBatchSelectionChanged = true;
            return;
        }
        notifySelectionChange(start, end);
    }

    private void notifySelectionChange(int start, int end) {

        final InputMethodManager imm = getInputMethodManager();
        if (imm == null) {
            return;
        }
        final View v = getView();
        final Editable editable = getEditable();
        imm.updateSelection(v, start, end, getComposingSpanStart(editable),
                            getComposingSpanEnd(editable));
    }

    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
        outAttrs.imeOptions = EditorInfo.IME_ACTION_NONE;
        outAttrs.actionLabel = null;

        if (mIMEState == IME_STATE_PASSWORD)
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
        else if (mIMEState == IME_STATE_PLUGIN)
            outAttrs.inputType = InputType.TYPE_NULL; 
        else if (mIMETypeHint.equalsIgnoreCase("url"))
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_URI;
        else if (mIMETypeHint.equalsIgnoreCase("email"))
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
        else if (mIMETypeHint.equalsIgnoreCase("search"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEARCH;
        else if (mIMETypeHint.equalsIgnoreCase("tel"))
            outAttrs.inputType = InputType.TYPE_CLASS_PHONE;
        else if (mIMETypeHint.equalsIgnoreCase("number") ||
                 mIMETypeHint.equalsIgnoreCase("range"))
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER
                                 | InputType.TYPE_NUMBER_FLAG_SIGNED
                                 | InputType.TYPE_NUMBER_FLAG_DECIMAL;
        else if (mIMETypeHint.equalsIgnoreCase("week") ||
                 mIMETypeHint.equalsIgnoreCase("month"))
            outAttrs.inputType = InputType.TYPE_CLASS_DATETIME
                                  | InputType.TYPE_DATETIME_VARIATION_DATE;
        else if (mIMEModeHint.equalsIgnoreCase("numeric"))
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER |
                                 InputType.TYPE_NUMBER_FLAG_SIGNED |
                                 InputType.TYPE_NUMBER_FLAG_DECIMAL;
        else if (mIMEModeHint.equalsIgnoreCase("digit"))
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER;
        else {
            outAttrs.inputType |= InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;
            if (mIMEModeHint.equalsIgnoreCase("uppercase"))
                outAttrs.inputType |= InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
            else if (mIMEModeHint.equalsIgnoreCase("titlecase"))
                outAttrs.inputType |= InputType.TYPE_TEXT_FLAG_CAP_WORDS;
            else if (mIMEModeHint.equalsIgnoreCase("autocapitalized"))
                outAttrs.inputType |= InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
            
        }

        if (mIMEActionHint.equalsIgnoreCase("go"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_GO;
        else if (mIMEActionHint.equalsIgnoreCase("done"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE;
        else if (mIMEActionHint.equalsIgnoreCase("next"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_NEXT;
        else if (mIMEActionHint.equalsIgnoreCase("search"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEARCH;
        else if (mIMEActionHint.equalsIgnoreCase("send"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEND;
        else if (mIMEActionHint.length() > 0) {
            if (DEBUG)
                Log.w(LOGTAG, "Unexpected mIMEActionHint=\"" + mIMEActionHint + "\"");
            outAttrs.actionLabel = mIMEActionHint;
        }

        GeckoApp app = GeckoApp.mAppContext;
        DisplayMetrics metrics = app.getResources().getDisplayMetrics();
        if (Math.min(metrics.widthPixels, metrics.heightPixels) > INLINE_IME_MIN_DISPLAY_SIZE) {
            
            
            outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_EXTRACT_UI
                                   | EditorInfo.IME_FLAG_NO_FULLSCREEN;
        }

        String prevInputMethod = mCurrentInputMethod;
        mCurrentInputMethod = InputMethods.getCurrentInputMethod(app);
        if (DEBUG) {
            Log.d(LOGTAG, "IME: CurrentInputMethod=" + mCurrentInputMethod);
        }

        
        if (mCurrentInputMethod != prevInputMethod) {
            FormAssistPopup popup = app.mFormAssistPopup;
            if (popup != null) {
                popup.onInputMethodChanged(mCurrentInputMethod);
            }
        }

        if (mIMEState == IME_STATE_PLUGIN) {
            
            outAttrs.initialSelStart = 0;
            outAttrs.initialSelEnd = 0;
            return mPluginInputConnection;
        }
        Editable editable = getEditable();
        outAttrs.initialSelStart = Selection.getSelectionStart(editable);
        outAttrs.initialSelEnd = Selection.getSelectionEnd(editable);
        return this;
    }

    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        return false;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return processKeyDown(keyCode, event);
    }

    private boolean processKeyDown(int keyCode, KeyEvent event) {
        if (keyCode > KeyEvent.getMaxKeyCode())
            return false;

        switch (keyCode) {
            case KeyEvent.KEYCODE_MENU:
            case KeyEvent.KEYCODE_BACK:
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_SEARCH:
                return false;
            case KeyEvent.KEYCODE_ENTER:
                if ((event.getFlags() & KeyEvent.FLAG_EDITOR_ACTION) != 0 &&
                    mIMEActionHint.equalsIgnoreCase("next"))
                    event = new KeyEvent(event.getAction(), KeyEvent.KEYCODE_TAB);
                break;
            default:
                break;
        }

        View view = getView();
        KeyListener keyListener = TextKeyListener.getInstance();

        
        if (mIMEState == IME_STATE_DISABLED ||
                mIMEState == IME_STATE_PLUGIN ||
                keyCode == KeyEvent.KEYCODE_ENTER ||
                keyCode == KeyEvent.KEYCODE_DEL ||
                keyCode == KeyEvent.KEYCODE_TAB ||
                (event.getFlags() & KeyEvent.FLAG_SOFT_KEYBOARD) != 0 ||
                !keyListener.onKeyDown(view, getEditable(), keyCode, event)) {
            mEditableClient.sendEvent(GeckoEvent.createKeyEvent(event));
        }
        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return processKeyUp(keyCode, event);
    }

    private boolean processKeyUp(int keyCode, KeyEvent event) {
        if (keyCode > KeyEvent.getMaxKeyCode())
            return false;

        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
            case KeyEvent.KEYCODE_SEARCH:
            case KeyEvent.KEYCODE_MENU:
                return false;
            default:
                break;
        }

        View view = getView();
        KeyListener keyListener = TextKeyListener.getInstance();

        if (mIMEState == IME_STATE_DISABLED ||
            mIMEState == IME_STATE_PLUGIN ||
            keyCode == KeyEvent.KEYCODE_ENTER ||
            keyCode == KeyEvent.KEYCODE_DEL ||
            (event.getFlags() & KeyEvent.FLAG_SOFT_KEYBOARD) != 0 ||
            !keyListener.onKeyUp(view, getEditable(), keyCode, event)) {
            mEditableClient.sendEvent(GeckoEvent.createKeyEvent(event));
        }

        return true;
    }

    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        while ((repeatCount--) != 0) {
            if (!processKeyDown(keyCode, event) ||
                !processKeyUp(keyCode, event)) {
                return false;
            }
        }
        return true;
    }

    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        View v = getView();
        switch (keyCode) {
            case KeyEvent.KEYCODE_MENU:
                InputMethodManager imm = getInputMethodManager();
                imm.toggleSoftInputFromWindow(v.getWindowToken(),
                                              InputMethodManager.SHOW_FORCED, 0);
                return true;
            default:
                break;
        }
        return false;
    }

    public boolean isIMEEnabled() {
        
        return mIMEState != IME_STATE_DISABLED;
    }

    public void notifyIME(final int type, final int state) {
        switch (type) {

            case NOTIFY_IME_CANCELCOMPOSITION:
                
                setComposingText("", 0);
                

            case NOTIFY_IME_RESETINPUTSTATE:
                
                finishComposingText();
                restartInput();
                break;

            case NOTIFY_IME_FOCUSCHANGE:
                
                mBatchEditCount = 0;
                mUpdateRequest = null;
                break;

            default:
                if (DEBUG) {
                    throw new IllegalArgumentException("Unexpected NOTIFY_IME=" + type);
                }
                break;
        }
    }

    public void notifyIMEEnabled(final int state, final String typeHint,
                                 final String modeHint, final String actionHint) {
        
        
        
        if (typeHint.equals("date") || typeHint.equals("time") ||
            (Build.VERSION.SDK_INT > 10 &&
            (typeHint.equals("datetime") || typeHint.equals("month") ||
            typeHint.equals("week") || typeHint.equals("datetime-local")))) {
            mIMEState = IME_STATE_DISABLED;
            return;
        }

        

        mIMEState = state;
        mIMETypeHint = (typeHint == null) ? "" : typeHint;
        mIMEModeHint = (modeHint == null) ? "" : modeHint;
        mIMEActionHint = (actionHint == null) ? "" : actionHint;

        View v = getView();
        if (v == null || !v.hasFocus()) {
            
            
            
            
            return;
        }
        restartInput();
        GeckoApp.mAppContext.mMainHandler.postDelayed(new Runnable() {
            public void run() {
                if (mIMEState == IME_STATE_DISABLED) {
                    hideSoftInput();
                } else {
                    showSoftInput();
                }
            }
        }, 200); 
    }
}

final class DebugGeckoInputConnection
        extends GeckoInputConnection
        implements InvocationHandler {

    private InputConnection mProxy;

    private DebugGeckoInputConnection(View targetView,
                                      GeckoEditableClient editable) {
        super(targetView, editable);
    }

    public static GeckoEditableListener create(View targetView,
                                               GeckoEditableClient editable) {
        final Class[] PROXY_INTERFACES = { InputConnection.class,
                InputConnectionHandler.class,
                GeckoEditableListener.class };
        DebugGeckoInputConnection dgic =
                new DebugGeckoInputConnection(targetView, editable);
        dgic.mProxy = (InputConnection)Proxy.newProxyInstance(
                GeckoInputConnection.class.getClassLoader(),
                PROXY_INTERFACES, dgic);
        return (GeckoEditableListener)dgic.mProxy;
    }

    private static StringBuilder debugAppend(StringBuilder sb, Object obj) {
        if (obj == null) {
            sb.append("null");
        } else if (obj instanceof GeckoEditable) {
            sb.append("GeckoEditable");
        } else if (Proxy.isProxyClass(obj.getClass())) {
            debugAppend(sb, Proxy.getInvocationHandler(obj));
        } else if (obj instanceof CharSequence) {
            sb.append("\"").append(obj.toString().replace('\n', '\u21b2')).append("\"");
        } else if (obj.getClass().isArray()) {
            sb.append(obj.getClass().getComponentType().getSimpleName()).append("[")
              .append(java.lang.reflect.Array.getLength(obj)).append("]");
        } else {
            sb.append(obj.toString());
        }
        return sb;
    }

    public Object invoke(Object proxy, Method method, Object[] args)
            throws Throwable {

        GeckoApp.assertOnUiThread();
        Object ret = method.invoke(this, args);
        if (ret == this) {
            ret = mProxy;
        }

        StringBuilder log = new StringBuilder(method.getName());
        log.append("(");
        for (Object arg : args) {
            debugAppend(log, arg).append(", ");
        }
        if (args.length > 0) {
            log.setLength(log.length() - 2);
        }
        if (method.getReturnType().equals(Void.TYPE)) {
            log.append(")");
        } else {
            debugAppend(log.append(") = "), ret);
        }
        Log.d(LOGTAG, log.toString());

        return ret;
    }
}

