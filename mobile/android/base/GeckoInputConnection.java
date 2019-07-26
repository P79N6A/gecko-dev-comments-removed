




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.InputConnectionHandler;

import android.R;
import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
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
import java.util.concurrent.SynchronousQueue;

class GeckoInputConnection
    extends BaseInputConnection
    implements InputConnectionHandler, GeckoEditableListener {

    private static final boolean DEBUG = false;
    protected static final String LOGTAG = "GeckoInputConnection";

    private static final int INLINE_IME_MIN_DISPLAY_SIZE = 480;

    private static Handler sBackgroundHandler;

    private class ThreadUtils {
        private Editable mUiEditable;
        private Object mUiEditableReturn;
        private Exception mUiEditableException;
        private final SynchronousQueue<Runnable> mIcRunnableSync;
        private final Runnable mIcSignalRunnable;

        public ThreadUtils() {
            mIcRunnableSync = new SynchronousQueue<Runnable>();
            mIcSignalRunnable = new Runnable() {
                @Override public void run() {
                }
            };
        }

        private void runOnIcThread(Handler icHandler, final Runnable runnable) {
            if (DEBUG) {
                GeckoApp.assertOnUiThread();
                Log.d(LOGTAG, "runOnIcThread() on thread " +
                              icHandler.getLooper().getThread().getName());
            }
            Runnable runner = new Runnable() {
                @Override public void run() {
                    try {
                        Runnable queuedRunnable = mIcRunnableSync.take();
                        if (DEBUG && queuedRunnable != runnable) {
                            throw new IllegalThreadStateException("sync error");
                        }
                        queuedRunnable.run();
                    } catch (InterruptedException e) {
                    }
                }
            };
            try {
                
                icHandler.post(runner);
                
                mIcRunnableSync.put(runnable);
            } catch (InterruptedException e) {
            } finally {
                
                icHandler.removeCallbacks(runner);
            }
        }

        public void endWaitForUiThread() {
            if (DEBUG) {
                GeckoApp.assertOnUiThread();
                Log.d(LOGTAG, "endWaitForUiThread()");
            }
            try {
                mIcRunnableSync.put(mIcSignalRunnable);
            } catch (InterruptedException e) {
            }
        }

        public void waitForUiThread(Handler icHandler) {
            if (DEBUG) {
                GeckoApp.assertOnThread(icHandler.getLooper().getThread());
                Log.d(LOGTAG, "waitForUiThread() blocking on thread " +
                              icHandler.getLooper().getThread().getName());
            }
            try {
                Runnable runnable = null;
                do {
                    runnable = mIcRunnableSync.take();
                    runnable.run();
                } while (runnable != mIcSignalRunnable);
            } catch (InterruptedException e) {
            }
        }

        public Editable getEditableForUiThread(final Handler uiHandler,
                                               final Handler icHandler) {
            if (DEBUG) {
                GeckoApp.assertOnThread(uiHandler.getLooper().getThread());
            }
            if (icHandler.getLooper() == uiHandler.getLooper()) {
                
                return getEditable();
            }
            
            
            if (mUiEditable != null) {
                return mUiEditable;
            }
            final InvocationHandler invokeEditable = new InvocationHandler() {
                @Override public Object invoke(final Object proxy,
                                               final Method method,
                                               final Object[] args) throws Throwable {
                    if (DEBUG) {
                        GeckoApp.assertOnThread(uiHandler.getLooper().getThread());
                        Log.d(LOGTAG, "UiEditable." + method.getName() + "() blocking");
                    }
                    synchronized (icHandler) {
                        
                        mUiEditableReturn = null;
                        mUiEditableException = null;
                        
                        
                        runOnIcThread(icHandler, new Runnable() {
                            @Override public void run() {
                                synchronized (icHandler) {
                                    try {
                                        mUiEditableReturn = method.invoke(
                                            mEditableClient.getEditable(), args);
                                    } catch (Exception e) {
                                        mUiEditableException = e;
                                    }
                                    if (DEBUG) {
                                        Log.d(LOGTAG, "UiEditable." + method.getName() +
                                                      "() returning");
                                    }
                                    icHandler.notify();
                                }
                            }
                        });
                        
                        icHandler.wait();
                        if (mUiEditableException != null) {
                            throw mUiEditableException;
                        }
                        return mUiEditableReturn;
                    }
                }
            };
            mUiEditable = (Editable) Proxy.newProxyInstance(Editable.class.getClassLoader(),
                new Class<?>[] { Editable.class }, invokeEditable);
            return mUiEditable;
        }
    }

    private final ThreadUtils mThreadUtils = new ThreadUtils();

    
    private int mIMEState;
    private String mIMETypeHint = "";
    private String mIMEModeHint = "";
    private String mIMEActionHint = "";

    private String mCurrentInputMethod = "";

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
        if (editable == null) {
            return false;
        }
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
        if (editable == null) {
            return null;
        }
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

    private void tryRestartInput() {
        
        
        if (SystemClock.uptimeMillis() < mLastRestartInputTime + 200) {
            return;
        }
        restartInput();
    }

    private void restartInput() {

        mLastRestartInputTime = SystemClock.uptimeMillis();

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

    private void resetInputConnection() {
        if (mBatchEditCount != 0) {
            Log.w(LOGTAG, "resetting with mBatchEditCount = " + mBatchEditCount);
            mBatchEditCount = 0;
        }
        mBatchSelectionChanged = false;
        mBatchTextChanged = false;
        mUpdateRequest = null;

        mCurrentInputMethod = "";

        
    }

    @Override
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
        final View v = getView();
        final Editable editable = getEditable();
        if (imm == null || v == null || editable == null) {
            return;
        }
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

    @Override
    public void onSelectionChange(int start, int end) {

        if (mBatchEditCount > 0) {
            
            mBatchSelectionChanged = true;
            return;
        }
        notifySelectionChange(start, end);
    }

    private void notifySelectionChange(int start, int end) {

        final InputMethodManager imm = getInputMethodManager();
        final View v = getView();
        final Editable editable = getEditable();
        if (imm == null || v == null || editable == null) {
            return;
        }
        imm.updateSelection(v, start, end, getComposingSpanStart(editable),
                            getComposingSpanEnd(editable));
    }

    private static synchronized Handler getBackgroundHandler() {
        if (sBackgroundHandler != null) {
            return sBackgroundHandler;
        }
        
        
        
        
        Thread backgroundThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                synchronized (GeckoInputConnection.class) {
                    sBackgroundHandler = new Handler();
                    GeckoInputConnection.class.notify();
                }
                Looper.loop();
                sBackgroundHandler = null;
            }
        }, LOGTAG);
        backgroundThread.setDaemon(true);
        backgroundThread.start();
        while (sBackgroundHandler == null) {
            try {
                
                GeckoInputConnection.class.wait();
            } catch (InterruptedException e) {
            }
        }
        return sBackgroundHandler;
    }

    private boolean canReturnCustomHandler() {
        if (mIMEState == IME_STATE_DISABLED) {
            return false;
        }
        for (StackTraceElement frame : Thread.currentThread().getStackTrace()) {
            
            
            
            
            
            
            if ("startInputInner".equals(frame.getMethodName()) &&
                "android.view.inputmethod.InputMethodManager".equals(frame.getClassName())) {
                
                return true;
            }
        }
        return false;
    }

    @Override
    public Handler getHandler(Handler defHandler) {
        if (!canReturnCustomHandler()) {
            return defHandler;
        }
        
        
        final Handler newHandler = sBackgroundHandler != null
                                 ? sBackgroundHandler
                                 : getBackgroundHandler();
        if (mEditableClient.setInputConnectionHandler(newHandler)) {
            return newHandler;
        }
        
        return mEditableClient.getInputConnectionHandler();
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mIMEState == IME_STATE_DISABLED) {
            return null;
        }

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

        if (DEBUG) {
            Log.d(LOGTAG, "mapped IME states to: inputType = " +
                          Integer.toHexString(outAttrs.inputType) + ", imeOptions = " +
                          Integer.toHexString(outAttrs.imeOptions));
        }

        String prevInputMethod = mCurrentInputMethod;
        mCurrentInputMethod = InputMethods.getCurrentInputMethod(app);
        if (DEBUG) {
            Log.d(LOGTAG, "IME: CurrentInputMethod=" + mCurrentInputMethod);
        }

        
        if (!mCurrentInputMethod.equals(prevInputMethod)) {
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

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        
        
        
        super.sendKeyEvent(event);
        final View v = getView();
        if (v == null) {
            return false;
        }
        final Handler icHandler = mEditableClient.getInputConnectionHandler();
        final Handler mainHandler = v.getRootView().getHandler();
        if (icHandler.getLooper() != mainHandler.getLooper()) {
            
            
            
            mainHandler.post(new Runnable() {
                @Override public void run() {
                    mThreadUtils.endWaitForUiThread();
                }
            });
            mThreadUtils.waitForUiThread(icHandler);
        }
        return false; 
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        return false;
    }

    @Override
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
            view == null) {
            mEditableClient.sendEvent(GeckoEvent.createKeyEvent(event));
            return true;
        }

        Handler uiHandler = view.getRootView().getHandler();
        Handler icHandler = mEditableClient.getInputConnectionHandler();
        Editable uiEditable = mThreadUtils.getEditableForUiThread(uiHandler, icHandler);
        if (!keyListener.onKeyDown(view, uiEditable, keyCode, event)) {
            mEditableClient.sendEvent(GeckoEvent.createKeyEvent(event));
        }
        return true;
    }

    @Override
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
            view == null) {
            mEditableClient.sendEvent(GeckoEvent.createKeyEvent(event));
            return true;
        }

        Handler uiHandler = view.getRootView().getHandler();
        Handler icHandler = mEditableClient.getInputConnectionHandler();
        Editable uiEditable = mThreadUtils.getEditableForUiThread(uiHandler, icHandler);
        if (!keyListener.onKeyUp(view, uiEditable, keyCode, event)) {
            mEditableClient.sendEvent(GeckoEvent.createKeyEvent(event));
        }
        return true;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        while ((repeatCount--) != 0) {
            if (!processKeyDown(keyCode, event) ||
                !processKeyUp(keyCode, event)) {
                return false;
            }
        }
        return true;
    }

    @Override
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

    @Override
    public boolean isIMEEnabled() {
        
        return mIMEState != IME_STATE_DISABLED;
    }

    @Override
    public void notifyIME(final int type, final int state) {
        switch (type) {

            case NOTIFY_IME_CANCELCOMPOSITION:
                
                setComposingText("", 0);
                

            case NOTIFY_IME_RESETINPUTSTATE:
                
                finishComposingText();
                tryRestartInput();
                break;

            case NOTIFY_IME_FOCUSCHANGE:
                
                resetInputConnection();
                break;

            default:
                if (DEBUG) {
                    throw new IllegalArgumentException("Unexpected NOTIFY_IME=" + type);
                }
                break;
        }
    }

    @Override
    public void notifyIMEEnabled(final int state, final String typeHint,
                                 final String modeHint, final String actionHint) {
        
        
        
        if (typeHint != null &&
            (typeHint.equals("date") ||
             typeHint.equals("time") ||
             (Build.VERSION.SDK_INT > 10 && (typeHint.equals("datetime") ||
                                             typeHint.equals("month") ||
                                             typeHint.equals("week") ||
                                             typeHint.equals("datetime-local"))))) {
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
            @Override
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
    private StringBuilder mCallLevel;

    private DebugGeckoInputConnection(View targetView,
                                      GeckoEditableClient editable) {
        super(targetView, editable);
        mCallLevel = new StringBuilder();
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

    @Override
    public Object invoke(Object proxy, Method method, Object[] args)
            throws Throwable {

        StringBuilder log = new StringBuilder(mCallLevel);
        log.append("> ").append(method.getName()).append("(");
        for (Object arg : args) {
            
            if ("notifyIME".equals(method.getName()) && arg == args[0]) {
                log.append(GeckoEditable.getConstantName(
                    GeckoEditableListener.class, "NOTIFY_IME_", arg));
            } else if ("notifyIMEEnabled".equals(method.getName()) && arg == args[0]) {
                log.append(GeckoEditable.getConstantName(
                    GeckoEditableListener.class, "IME_STATE_", arg));
            } else {
                GeckoEditable.debugAppend(log, arg);
            }
            log.append(", ");
        }
        if (args.length > 0) {
            log.setLength(log.length() - 2);
        }
        log.append(")");
        Log.d(LOGTAG, log.toString());

        mCallLevel.append(' ');
        Object ret = method.invoke(this, args);
        if (ret == this) {
            ret = mProxy;
        }
        mCallLevel.setLength(Math.max(0, mCallLevel.length() - 1));

        log.setLength(mCallLevel.length());
        log.append("< ").append(method.getName());
        if (!method.getReturnType().equals(Void.TYPE)) {
            GeckoEditable.debugAppend(log.append(": "), ret);
        }
        Log.d(LOGTAG, log.toString());
        return ret;
    }
}
