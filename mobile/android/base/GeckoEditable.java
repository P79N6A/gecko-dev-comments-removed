




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.InputConnectionHandler;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.ThreadUtils;

import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Spanned;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Selection;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.CharacterStyle;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Semaphore;


interface GeckoEditableClient {
    void sendEvent(GeckoEvent event);
    Editable getEditable();
    void setUpdateGecko(boolean update);
    Handler getInputConnectionHandler();
    boolean setInputConnectionHandler(Handler handler);
}



interface GeckoEditableListener {
    
    final int NOTIFY_IME_RESETINPUTSTATE = 0;
    final int NOTIFY_IME_REPLY_EVENT = 1;
    final int NOTIFY_IME_CANCELCOMPOSITION = 2;
    final int NOTIFY_IME_FOCUSCHANGE = 3;
    
    final int IME_FOCUS_STATE_FOCUS = 1;
    final int IME_FOCUS_STATE_BLUR = 0;
    
    final int IME_STATE_DISABLED = 0;
    final int IME_STATE_ENABLED = 1;
    final int IME_STATE_PASSWORD = 2;
    final int IME_STATE_PLUGIN = 3;

    void notifyIME(int type, int state);
    void notifyIMEEnabled(int state, String typeHint,
                          String modeHint, String actionHint);
    void onSelectionChange(int start, int end);
    void onTextChange(String text, int start, int oldEnd, int newEnd);
}






final class GeckoEditable
        implements InvocationHandler, Editable,
                   GeckoEditableClient, GeckoEditableListener {

    private static final boolean DEBUG = false;
    private static final String LOGTAG = "GeckoEditable";

    
    private InputFilter[] mFilters;

    private final SpannableStringBuilder mText;
    private final SpannableStringBuilder mChangedText;
    private final Editable mProxy;
    private final ActionQueue mActionQueue;

    
    
    
    private Handler mIcRunHandler;
    private Handler mIcPostHandler;

    private GeckoEditableListener mListener;
    private int mSavedSelectionStart;
    private volatile int mGeckoUpdateSeqno;
    private int mIcUpdateSeqno;
    private int mLastIcUpdateSeqno;
    private boolean mUpdateGecko;
    private boolean mFocused;

    





    private static final class Action {
        
        static final int TYPE_EVENT = 0;
        
        static final int TYPE_REPLACE_TEXT = 1;
        



        static final int TYPE_SET_SELECTION = 2;
        
        static final int TYPE_SET_SPAN = 3;
        
        static final int TYPE_REMOVE_SPAN = 4;
        
        static final int TYPE_ACKNOWLEDGE_FOCUS = 5;
        
        static final int TYPE_SET_HANDLER = 6;

        final int mType;
        int mStart;
        int mEnd;
        CharSequence mSequence;
        Object mSpanObject;
        int mSpanFlags;
        boolean mShouldUpdate;
        Handler mHandler;

        Action(int type) {
            mType = type;
        }

        static Action newReplaceText(CharSequence text, int start, int end) {
            if (start < 0 || start > end) {
                throw new IllegalArgumentException("invalid replace text offsets");
            }
            final Action action = new Action(TYPE_REPLACE_TEXT);
            action.mSequence = text;
            action.mStart = start;
            action.mEnd = end;
            return action;
        }

        static Action newSetSelection(int start, int end) {
            
            
            if (start < -1 || end < -1) {
                throw new IllegalArgumentException("invalid selection offsets");
            }
            final Action action = new Action(TYPE_SET_SELECTION);
            action.mStart = start;
            action.mEnd = end;
            return action;
        }

        static Action newSetSpan(Object object, int start, int end, int flags) {
            if (start < 0 || start > end) {
                throw new IllegalArgumentException("invalid span offsets");
            }
            final Action action = new Action(TYPE_SET_SPAN);
            action.mSpanObject = object;
            action.mStart = start;
            action.mEnd = end;
            action.mSpanFlags = flags;
            return action;
        }

        static Action newSetHandler(Handler handler) {
            final Action action = new Action(TYPE_SET_HANDLER);
            action.mHandler = handler;
            return action;
        }
    }

    

    private final class ActionQueue {
        private final ConcurrentLinkedQueue<Action> mActions;
        private final Semaphore mActionsActive;

        ActionQueue() {
            mActions = new ConcurrentLinkedQueue<Action>();
            mActionsActive = new Semaphore(1);
        }

        void offer(Action action) {
            if (DEBUG) {
                assertOnIcThread();
                Log.d(LOGTAG, "offer: Action(" +
                              getConstantName(Action.class, "TYPE_", action.mType) + ")");
            }
            

            if (action.mType != Action.TYPE_EVENT &&
                action.mType != Action.TYPE_ACKNOWLEDGE_FOCUS &&
                action.mType != Action.TYPE_SET_HANDLER) {
                action.mShouldUpdate = mUpdateGecko;
            }
            if (mActions.isEmpty()) {
                mActionsActive.acquireUninterruptibly();
                mActions.offer(action);
            } else synchronized(this) {
                
                mActionsActive.tryAcquire();
                mActions.offer(action);
            }
            switch (action.mType) {
            case Action.TYPE_EVENT:
            case Action.TYPE_SET_SELECTION:
            case Action.TYPE_SET_SPAN:
            case Action.TYPE_REMOVE_SPAN:
            case Action.TYPE_SET_HANDLER:
                GeckoAppShell.sendEventToGecko(
                        GeckoEvent.createIMEEvent(GeckoEvent.IME_SYNCHRONIZE));
                break;
            case Action.TYPE_REPLACE_TEXT:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createIMEReplaceEvent(
                        action.mStart, action.mEnd, action.mSequence.toString()));
                break;
            case Action.TYPE_ACKNOWLEDGE_FOCUS:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createIMEEvent(
                        GeckoEvent.IME_ACKNOWLEDGE_FOCUS));
                break;
            }
            ++mIcUpdateSeqno;
        }

        void poll() {
            if (DEBUG) {
                ThreadUtils.assertOnGeckoThread();
            }
            if (mActions.isEmpty()) {
                throw new IllegalStateException("empty actions queue");
            }
            mActions.poll();
            
            if (mActions.isEmpty()) {
                synchronized(this) {
                    if (mActions.isEmpty()) {
                        mActionsActive.release();
                    }
                }
            }
        }

        Action peek() {
            if (DEBUG) {
                ThreadUtils.assertOnGeckoThread();
            }
            if (mActions.isEmpty()) {
                throw new IllegalStateException("empty actions queue");
            }
            return mActions.peek();
        }

        void syncWithGecko() {
            if (DEBUG) {
                assertOnIcThread();
            }
            if (mFocused && !mActions.isEmpty()) {
                if (DEBUG) {
                    Log.d(LOGTAG, "syncWithGecko blocking on thread " +
                                  Thread.currentThread().getName());
                }
                mActionsActive.acquireUninterruptibly();
                mActionsActive.release();
            } else if (DEBUG && !mFocused) {
                Log.d(LOGTAG, "skipped syncWithGecko (no focus)");
            }
        }

        boolean isEmpty() {
            return mActions.isEmpty();
        }
    }

    GeckoEditable() {
        mActionQueue = new ActionQueue();
        mSavedSelectionStart = -1;
        mUpdateGecko = true;

        mText = new SpannableStringBuilder();
        mChangedText = new SpannableStringBuilder();

        final Class<?>[] PROXY_INTERFACES = { Editable.class };
        mProxy = (Editable)Proxy.newProxyInstance(
                Editable.class.getClassLoader(),
                PROXY_INTERFACES, this);

        LayerView v = GeckoApp.mAppContext.getLayerView();
        mListener = GeckoInputConnection.create(v, this);

        mIcRunHandler = mIcPostHandler = GeckoApp.mAppContext.mMainHandler;
    }

    private boolean onIcThread() {
        return mIcRunHandler.getLooper() == Looper.myLooper();
    }

    private void assertOnIcThread() {
        ThreadUtils.assertOnThread(mIcRunHandler.getLooper().getThread());
    }

    private void geckoPostToIc(Runnable runnable) {
        mIcPostHandler.post(runnable);
    }

    private void geckoUpdateGecko(final boolean force) {
        


        final int seqnoWhenPosted = mGeckoUpdateSeqno;

        geckoPostToIc(new Runnable() {
            @Override
            public void run() {
                mActionQueue.syncWithGecko();
                if (seqnoWhenPosted == mGeckoUpdateSeqno) {
                    icUpdateGecko(force);
                }
            }
        });
    }

    private Object getField(Object obj, String field, Object def) {
        try {
            return obj.getClass().getField(field).get(obj);
        } catch (Exception e) {
            return def;
        }
    }

    private void icUpdateGecko(boolean force) {

        if (!force && mIcUpdateSeqno == mLastIcUpdateSeqno) {
            if (DEBUG) {
                Log.d(LOGTAG, "icUpdateGecko() skipped");
            }
            return;
        }
        mLastIcUpdateSeqno = mIcUpdateSeqno;
        mActionQueue.syncWithGecko();

        if (DEBUG) {
            Log.d(LOGTAG, "icUpdateGecko()");
        }

        final int selStart = mText.getSpanStart(Selection.SELECTION_START);
        final int selEnd = mText.getSpanEnd(Selection.SELECTION_END);
        int composingStart = mText.length();
        int composingEnd = 0;
        Object[] spans = mText.getSpans(0, composingStart, Object.class);

        for (Object span : spans) {
            if ((mText.getSpanFlags(span) & Spanned.SPAN_COMPOSING) != 0) {
                composingStart = Math.min(composingStart, mText.getSpanStart(span));
                composingEnd = Math.max(composingEnd, mText.getSpanEnd(span));
            }
        }
        if (DEBUG) {
            Log.d(LOGTAG, " range = " + composingStart + "-" + composingEnd);
            Log.d(LOGTAG, " selection = " + selStart + "-" + selEnd);
        }
        if (composingStart >= composingEnd) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createIMEEvent(
                    GeckoEvent.IME_REMOVE_COMPOSITION));
            if (selStart >= 0 && selEnd >= 0) {
                GeckoAppShell.sendEventToGecko(
                        GeckoEvent.createIMESelectEvent(selStart, selEnd));
            }
            return;
        }

        if (selEnd >= composingStart && selEnd <= composingEnd) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createIMERangeEvent(
                    selEnd - composingStart, selEnd - composingStart,
                    GeckoEvent.IME_RANGE_CARETPOSITION, 0, 0, false, 0, 0, 0));
        }
        int rangeStart = composingStart;
        TextPaint tp = new TextPaint();
        TextPaint emptyTp = new TextPaint();
        
        
        emptyTp.setColor(0);
        do {
            int rangeType, rangeStyles = 0, rangeLineStyle = GeckoEvent.IME_RANGE_LINE_NONE;
            boolean rangeBoldLine = false;
            int rangeForeColor = 0, rangeBackColor = 0, rangeLineColor = 0;
            int rangeEnd = mText.nextSpanTransition(rangeStart, composingEnd, Object.class);

            if (selStart > rangeStart && selStart < rangeEnd) {
                rangeEnd = selStart;
            } else if (selEnd > rangeStart && selEnd < rangeEnd) {
                rangeEnd = selEnd;
            }
            CharacterStyle[] styleSpans =
                    mText.getSpans(rangeStart, rangeEnd, CharacterStyle.class);

            if (DEBUG) {
                Log.d(LOGTAG, " found " + styleSpans.length + " spans @ " +
                              rangeStart + "-" + rangeEnd);
            }

            if (styleSpans.length == 0) {
                rangeType = (selStart == rangeStart && selEnd == rangeEnd)
                            ? GeckoEvent.IME_RANGE_SELECTEDRAWTEXT
                            : GeckoEvent.IME_RANGE_RAWINPUT;
            } else {
                rangeType = (selStart == rangeStart && selEnd == rangeEnd)
                            ? GeckoEvent.IME_RANGE_SELECTEDCONVERTEDTEXT
                            : GeckoEvent.IME_RANGE_CONVERTEDTEXT;
                tp.set(emptyTp);
                for (CharacterStyle span : styleSpans) {
                    span.updateDrawState(tp);
                }
                int tpUnderlineColor = 0;
                float tpUnderlineThickness = 0.0f;
                
                if (Build.VERSION.SDK_INT >= 14) {
                    tpUnderlineColor = (Integer)getField(tp, "underlineColor", 0);
                    tpUnderlineThickness = (Float)getField(tp, "underlineThickness", 0.0f);
                }
                if (tpUnderlineColor != 0) {
                    rangeStyles |= GeckoEvent.IME_RANGE_UNDERLINE | GeckoEvent.IME_RANGE_LINECOLOR;
                    rangeLineColor = tpUnderlineColor;
                    
                    if (tpUnderlineThickness <= 0.5f) {
                        rangeLineStyle = GeckoEvent.IME_RANGE_LINE_DOTTED;
                    } else {
                        rangeLineStyle = GeckoEvent.IME_RANGE_LINE_SOLID;
                        if (tpUnderlineThickness >= 2.0f) {
                            rangeBoldLine = true;
                        }
                    }
                } else if (tp.isUnderlineText()) {
                    rangeStyles |= GeckoEvent.IME_RANGE_UNDERLINE;
                    rangeLineStyle = GeckoEvent.IME_RANGE_LINE_SOLID;
                }
                if (tp.getColor() != 0) {
                    rangeStyles |= GeckoEvent.IME_RANGE_FORECOLOR;
                    rangeForeColor = tp.getColor();
                }
                if (tp.bgColor != 0) {
                    rangeStyles |= GeckoEvent.IME_RANGE_BACKCOLOR;
                    rangeBackColor = tp.bgColor;
                }
            }
            GeckoAppShell.sendEventToGecko(GeckoEvent.createIMERangeEvent(
                    rangeStart - composingStart, rangeEnd - composingStart,
                    rangeType, rangeStyles, rangeLineStyle, rangeBoldLine,
                    rangeForeColor, rangeBackColor, rangeLineColor));
            rangeStart = rangeEnd;

            if (DEBUG) {
                Log.d(LOGTAG, " added " + rangeType +
                              " : " + Integer.toHexString(rangeStyles) +
                              " : " + Integer.toHexString(rangeForeColor) +
                              " : " + Integer.toHexString(rangeBackColor));
            }
        } while (rangeStart < composingEnd);

        GeckoAppShell.sendEventToGecko(GeckoEvent.createIMECompositionEvent(
                composingStart, composingEnd));
    }

    

    @Override
    public void sendEvent(final GeckoEvent event) {
        if (DEBUG) {
            Log.d(LOGTAG, "sendEvent(" + event + ")");
        }
        if (!onIcThread()) {
            
            
            mIcRunHandler.post(new Runnable() {
                @Override
                public void run() {
                    sendEvent(event);
                }
            });
            return;
        }
        







        GeckoAppShell.sendEventToGecko(event);
        mActionQueue.offer(new Action(Action.TYPE_EVENT));
    }

    @Override
    public Editable getEditable() {
        if (!onIcThread()) {
            
            if (DEBUG) {
                Log.i(LOGTAG, "getEditable() called on non-IC thread");
            }
            return null;
        }
        return mProxy;
    }

    @Override
    public void setUpdateGecko(boolean update) {
        if (!onIcThread()) {
            
            if (DEBUG) {
                Log.i(LOGTAG, "setUpdateGecko() called on non-IC thread");
            }
            return;
        }
        if (update) {
            icUpdateGecko(false);
        }
        mUpdateGecko = update;
    }

    @Override
    public Handler getInputConnectionHandler() {
        if (DEBUG) {
            assertOnIcThread();
        }
        return mIcRunHandler;
    }

    @Override
    public boolean setInputConnectionHandler(Handler handler) {
        if (handler == mIcPostHandler) {
            return true;
        }
        if (!mFocused) {
            return false;
        }
        if (DEBUG) {
            assertOnIcThread();
        }
        
        
        
        
        
        
        
        
        
        
        
        
        mActionQueue.offer(Action.newSetHandler(handler));
        mActionQueue.syncWithGecko();
        return true;
    }

    private void geckoSetIcHandler(final Handler newHandler) {
        geckoPostToIc(new Runnable() { 
            @Override
            public void run() {
                synchronized (newHandler) {
                    mIcRunHandler = newHandler;
                    newHandler.notify();
                }
            }
        });

        
        
        
        mIcPostHandler = newHandler;

        geckoPostToIc(new Runnable() { 
            @Override
            public void run() {
                synchronized (newHandler) {
                    while (mIcRunHandler != newHandler) {
                        try {
                            newHandler.wait();
                        } catch (InterruptedException e) {
                        }
                    }
                }
            }
        });
    }

    

    private void geckoActionReply() {
        if (DEBUG) {
            
            ThreadUtils.assertOnGeckoThread();
        }
        final Action action = mActionQueue.peek();

        if (DEBUG) {
            Log.d(LOGTAG, "reply: Action(" +
                          getConstantName(Action.class, "TYPE_", action.mType) + ")");
        }
        switch (action.mType) {
        case Action.TYPE_SET_SELECTION:
            final int len = mText.length();
            final int curStart = Selection.getSelectionStart(mText);
            final int curEnd = Selection.getSelectionEnd(mText);
            
            
            final int selStart = Math.min(action.mStart < 0 ? curStart : action.mStart, len);
            final int selEnd = Math.min(action.mEnd < 0 ? curEnd : action.mEnd, len);

            if (selStart < action.mStart || selEnd < action.mEnd) {
                Log.w(LOGTAG, "IME sync error: selection out of bounds");
            }
            Selection.setSelection(mText, selStart, selEnd);
            geckoPostToIc(new Runnable() {
                @Override
                public void run() {
                    mActionQueue.syncWithGecko();
                    final int start = Selection.getSelectionStart(mText);
                    final int end = Selection.getSelectionEnd(mText);
                    if (selStart == start && selEnd == end) {
                        
                        
                        mListener.onSelectionChange(start, end);
                    }
                }
            });
            break;
        case Action.TYPE_SET_SPAN:
            mText.setSpan(action.mSpanObject, action.mStart, action.mEnd, action.mSpanFlags);
            break;
        case Action.TYPE_SET_HANDLER:
            geckoSetIcHandler(action.mHandler);
            break;
        }
        if (action.mShouldUpdate) {
            geckoUpdateGecko(false);
        }
    }

    @Override
    public void notifyIME(final int type, final int state) {
        if (DEBUG) {
            
            ThreadUtils.assertOnGeckoThread();
            
            if (type != NOTIFY_IME_REPLY_EVENT) {
                Log.d(LOGTAG, "notifyIME(" +
                              getConstantName(GeckoEditableListener.class, "NOTIFY_IME_", type) +
                              ", " + state + ")");
            }
        }
        if (type == NOTIFY_IME_REPLY_EVENT) {
            try {
                if (mFocused) {
                    
                    
                    geckoActionReply();
                } else if (DEBUG) {
                    Log.d(LOGTAG, "discarding stale reply");
                }
            } finally {
                
                
                mActionQueue.poll();
            }
            return;
        }
        geckoPostToIc(new Runnable() {
            @Override
            public void run() {
                if (type == NOTIFY_IME_FOCUSCHANGE) {
                    if (state == IME_FOCUS_STATE_BLUR) {
                        mFocused = false;
                    } else {
                        mFocused = true;
                        
                        mActionQueue.offer(new Action(Action.TYPE_ACKNOWLEDGE_FOCUS));
                    }
                }
                
                
                
                mActionQueue.syncWithGecko();
                mListener.notifyIME(type, state);
            }
        });
    }

    @Override
    public void notifyIMEEnabled(final int state, final String typeHint,
                          final String modeHint, final String actionHint) {
        
        
        if (DEBUG) {
            Log.d(LOGTAG, "notifyIMEEnabled(" +
                          getConstantName(GeckoEditableListener.class, "IME_STATE_", state) +
                          ", \"" + typeHint + "\", \"" + modeHint + "\", \"" + actionHint + "\")");
        }
        geckoPostToIc(new Runnable() {
            @Override
            public void run() {
                
                mActionQueue.syncWithGecko();
                
                
                
                LayerView v = GeckoApp.mAppContext.getLayerView();
                if (v != null) {
                    mListener = GeckoInputConnection.create(v, GeckoEditable.this);
                    v.setInputConnectionHandler((InputConnectionHandler)mListener);
                    mListener.notifyIMEEnabled(state, typeHint, modeHint, actionHint);
                }
            }
        });
    }

    @Override
    public void onSelectionChange(final int start, final int end) {
        if (DEBUG) {
            
            ThreadUtils.assertOnGeckoThread();
            Log.d(LOGTAG, "onSelectionChange(" + start + ", " + end + ")");
        }
        if (start < 0 || start > mText.length() || end < 0 || end > mText.length()) {
            throw new IllegalArgumentException("invalid selection notification range");
        }
        final int seqnoWhenPosted = ++mGeckoUpdateSeqno;

        


        if (!mActionQueue.isEmpty() &&
            mActionQueue.peek().mType == Action.TYPE_EVENT) {
            Selection.setSelection(mText, start, end);
            return;
        }

        geckoPostToIc(new Runnable() {
            @Override
            public void run() {
                mActionQueue.syncWithGecko();
                


                if (mGeckoUpdateSeqno == seqnoWhenPosted) {
                    



                    boolean oldUpdateGecko = mUpdateGecko;
                    mUpdateGecko = false;
                    Selection.setSelection(mProxy, start, end);
                    mUpdateGecko = oldUpdateGecko;
                }
            }
        });
    }

    private void geckoReplaceText(int start, int oldEnd, CharSequence newText) {
        
        
        mText.delete(start, oldEnd);
        mText.insert(start, newText);
    }

    @Override
    public void onTextChange(final String text, final int start,
                      final int unboundedOldEnd, final int unboundedNewEnd) {
        if (DEBUG) {
            
            ThreadUtils.assertOnGeckoThread();
            Log.d(LOGTAG, "onTextChange(\"" + text + "\", " + start + ", " +
                          unboundedOldEnd + ", " + unboundedNewEnd + ")");
        }
        if (start < 0 || start > unboundedOldEnd) {
            throw new IllegalArgumentException("invalid text notification range");
        }
        

        final int oldEnd = unboundedOldEnd > mText.length() ? mText.length() : unboundedOldEnd;
        
        if (start != 0 && unboundedNewEnd != (start + text.length())) {
            throw new IllegalArgumentException("newEnd does not match text");
        }
        final int newEnd = start + text.length();

        



        ++mGeckoUpdateSeqno;

        mChangedText.clearSpans();
        mChangedText.replace(0, mChangedText.length(), text);
        
        TextUtils.copySpansFrom(mText, start, Math.min(oldEnd, newEnd),
                                Object.class, mChangedText, 0);

        if (!mActionQueue.isEmpty()) {
            final Action action = mActionQueue.peek();
            if (action.mType == Action.TYPE_REPLACE_TEXT &&
                    start <= action.mStart &&
                    action.mStart + action.mSequence.length() <= newEnd) {

                
                final int actionNewEnd = action.mStart + action.mSequence.length();
                int selStart = Selection.getSelectionStart(mText);
                int selEnd = Selection.getSelectionEnd(mText);

                
                mChangedText.replace(action.mStart - start, actionNewEnd - start,
                                     action.mSequence);
                geckoReplaceText(start, oldEnd, mChangedText);

                
                
                
                if (selStart >= start && selStart <= oldEnd) {
                    selStart = selStart < action.mStart ? selStart :
                               selStart < action.mEnd   ? actionNewEnd :
                                                          selStart + actionNewEnd - action.mEnd;
                    mText.setSpan(Selection.SELECTION_START, selStart, selStart,
                                  Spanned.SPAN_POINT_POINT);
                }
                if (selEnd >= start && selEnd <= oldEnd) {
                    selEnd = selEnd < action.mStart ? selEnd :
                             selEnd < action.mEnd   ? actionNewEnd :
                                                      selEnd + actionNewEnd - action.mEnd;
                    mText.setSpan(Selection.SELECTION_END, selEnd, selEnd,
                                  Spanned.SPAN_POINT_POINT);
                }
            } else {
                geckoReplaceText(start, oldEnd, mChangedText);
            }
        } else {
            geckoReplaceText(start, oldEnd, mChangedText);
        }
        geckoPostToIc(new Runnable() {
            @Override
            public void run() {
                mListener.onTextChange(text, start, oldEnd, newEnd);
            }
        });
    }

    

    static String getConstantName(Class<?> cls, String prefix, Object value) {
        for (Field fld : cls.getDeclaredFields()) {
            try {
                if (fld.getName().startsWith(prefix) &&
                    fld.get(null).equals(value)) {
                    return fld.getName();
                }
            } catch (IllegalAccessException e) {
            }
        }
        return String.valueOf(value);
    }

    static StringBuilder debugAppend(StringBuilder sb, Object obj) {
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

    @Override
    public Object invoke(Object proxy, Method method, Object[] args)
                         throws Throwable {
        Object target;
        final Class<?> methodInterface = method.getDeclaringClass();
        if (DEBUG) {
            
            assertOnIcThread();
        }
        if (methodInterface == Editable.class ||
                methodInterface == Appendable.class ||
                methodInterface == Spannable.class) {
            
            target = this;
        } else {
            
            
            mActionQueue.syncWithGecko();
            target = mText;
        }
        Object ret;
        try {
            ret = method.invoke(target, args);
        } catch (InvocationTargetException e) {
            
            
            
            
            
            if (!(e.getCause() instanceof IndexOutOfBoundsException)) {
                
                
                throw e;
            }
            Log.w(LOGTAG, "Exception in GeckoEditable." + method.getName(), e.getCause());
            Class<?> retClass = method.getReturnType();
            if (retClass != Void.TYPE && retClass.isPrimitive()) {
                ret = retClass.newInstance();
            } else if (retClass == String.class) {
                ret = "";
            } else {
                ret = null;
            }
        }
        if (DEBUG) {
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
        }
        return ret;
    }

    

    @Override
    public void removeSpan(Object what) {
        if (what == Selection.SELECTION_START ||
                what == Selection.SELECTION_END) {
            Log.w(LOGTAG, "selection removed with removeSpan()");
        }
        if (mText.getSpanStart(what) >= 0) { 
            
            mText.removeSpan(what);
            mActionQueue.offer(new Action(Action.TYPE_REMOVE_SPAN));
        }
    }

    @Override
    public void setSpan(Object what, int start, int end, int flags) {
        if (what == Selection.SELECTION_START) {
            if ((flags & Spanned.SPAN_INTERMEDIATE) != 0) {
                
                mSavedSelectionStart = start;
            } else {
                mActionQueue.offer(Action.newSetSelection(start, -1));
            }
        } else if (what == Selection.SELECTION_END) {
            mActionQueue.offer(Action.newSetSelection(mSavedSelectionStart, end));
            mSavedSelectionStart = -1;
        } else {
            mActionQueue.offer(Action.newSetSpan(what, start, end, flags));
        }
    }

    

    @Override
    public Editable append(CharSequence text) {
        return replace(mProxy.length(), mProxy.length(), text, 0, text.length());
    }

    @Override
    public Editable append(CharSequence text, int start, int end) {
        return replace(mProxy.length(), mProxy.length(), text, start, end);
    }

    @Override
    public Editable append(char text) {
        return replace(mProxy.length(), mProxy.length(), String.valueOf(text), 0, 1);
    }

    

    @Override
    public InputFilter[] getFilters() {
        return mFilters;
    }

    @Override
    public void setFilters(InputFilter[] filters) {
        mFilters = filters;
    }

    @Override
    public void clearSpans() {
        

        Log.w(LOGTAG, "selection cleared with clearSpans()");
        mText.clearSpans();
    }

    @Override
    public Editable replace(int st, int en,
            CharSequence source, int start, int end) {

        CharSequence text = source;
        if (start < 0 || start > end || end > text.length()) {
            throw new IllegalArgumentException("invalid replace offsets");
        }
        if (start != 0 || end != text.length()) {
            text = text.subSequence(start, end);
        }
        if (mFilters != null) {
            
            for (int i = 0; i < mFilters.length; ++i) {
                final CharSequence cs = mFilters[i].filter(
                        text, 0, text.length(), mProxy, st, en);
                if (cs != null) {
                    text = cs;
                }
            }
        }
        if (text == source) {
            
            text = new SpannableString(source);
        }
        mActionQueue.offer(Action.newReplaceText(text,
                Math.min(st, en), Math.max(st, en)));
        return mProxy;
    }

    @Override
    public void clear() {
        replace(0, mProxy.length(), "", 0, 0);
    }

    @Override
    public Editable delete(int st, int en) {
        return replace(st, en, "", 0, 0);
    }

    @Override
    public Editable insert(int where, CharSequence text,
                                int start, int end) {
        return replace(where, where, text, start, end);
    }

    @Override
    public Editable insert(int where, CharSequence text) {
        return replace(where, where, text, 0, text.length());
    }

    @Override
    public Editable replace(int st, int en, CharSequence text) {
        return replace(st, en, text, 0, text.length());
    }

    

    @Override
    public void getChars(int start, int end, char[] dest, int destoff) {
        


        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    

    @Override
    public int getSpanEnd(Object tag) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    public int getSpanFlags(Object tag) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    public int getSpanStart(Object tag) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    public <T> T[] getSpans(int start, int end, Class<T> type) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    @SuppressWarnings("rawtypes") 
    public int nextSpanTransition(int start, int limit, Class type) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    

    @Override
    public char charAt(int index) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    public int length() {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    public CharSequence subSequence(int start, int end) {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }

    @Override
    public String toString() {
        throw new UnsupportedOperationException("method must be called through mProxy");
    }
}

