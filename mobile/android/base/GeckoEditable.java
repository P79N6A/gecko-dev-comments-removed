




package org.mozilla.gecko;

import android.text.Editable;
import android.text.InputFilter;
import android.text.Spanned;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Selection;
import android.text.style.UnderlineSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.BackgroundColorSpan;
import android.util.Log;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Semaphore;


interface GeckoEditableClient {
    void sendEvent(GeckoEvent event);
    Editable getEditable();
    void setUpdateGecko(boolean update);
    void setListener(GeckoEditableListener listener);
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
    private final Editable mProxy;
    private GeckoEditableListener mListener;
    private final ActionQueue mActionQueue;

    private int mSavedSelectionStart;
    private volatile int mGeckoUpdateSeqno;
    private int mUIUpdateSeqno;
    private int mLastUIUpdateSeqno;
    private boolean mUpdateGecko;
    private boolean mFocused;

    





    private static final class Action {
        
        static final int TYPE_EVENT = 0;
        
        static final int TYPE_REPLACE_TEXT = 1;
        



        static final int TYPE_SET_SELECTION = 2;
        
        static final int TYPE_SET_SPAN = 3;
        
        static final int TYPE_REMOVE_SPAN = 4;

        final int mType;
        int mStart;
        int mEnd;
        CharSequence mSequence;
        Object mSpanObject;
        int mSpanFlags;
        boolean mShouldUpdate;

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
                GeckoApp.assertOnUiThread();
            }
            

            if (action.mType != Action.TYPE_EVENT) {
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
                GeckoAppShell.sendEventToGecko(
                        GeckoEvent.createIMEEvent(GeckoEvent.IME_SYNCHRONIZE));
                break;
            case Action.TYPE_REPLACE_TEXT:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createIMEReplaceEvent(
                        action.mStart, action.mEnd, action.mSequence.toString()));
                break;
            }
            ++mUIUpdateSeqno;
        }

        void poll() {
            if (DEBUG) {
                GeckoApp.assertOnGeckoThread();
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
                GeckoApp.assertOnGeckoThread();
            }
            if (mActions.isEmpty()) {
                throw new IllegalStateException("empty actions queue");
            }
            return mActions.peek();
        }

        void syncWithGecko() {
            if (DEBUG) {
                GeckoApp.assertOnUiThread();
            }
            if (mFocused && !mActions.isEmpty()) {
                mActionsActive.acquireUninterruptibly();
                mActionsActive.release();
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

        final Class<?>[] PROXY_INTERFACES = { Editable.class };
        mProxy = (Editable)Proxy.newProxyInstance(
                Editable.class.getClassLoader(),
                PROXY_INTERFACES, this);
    }

    private static void geckoPostToUI(Runnable runnable) {
        GeckoApp.mAppContext.mMainHandler.post(runnable);
    }

    private void geckoUpdateGecko(final boolean force) {
        


        final int seqnoWhenPosted = mGeckoUpdateSeqno;

        geckoPostToUI(new Runnable() {
            public void run() {
                mActionQueue.syncWithGecko();
                if (seqnoWhenPosted == mGeckoUpdateSeqno) {
                    uiUpdateGecko(force);
                }
            }
        });
    }

    private void uiUpdateGecko(boolean force) {

        if (!force && mUIUpdateSeqno == mLastUIUpdateSeqno) {
            if (DEBUG) {
                Log.d(LOGTAG, "uiUpdateGecko() skipped");
            }
            return;
        }
        mLastUIUpdateSeqno = mUIUpdateSeqno;
        mActionQueue.syncWithGecko();

        if (DEBUG) {
            Log.d(LOGTAG, "uiUpdateGecko()");
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
                    GeckoEvent.IME_RANGE_CARETPOSITION, 0, 0, 0));
        }
        int rangeStart = composingStart;
        do {
            int rangeType, rangeStyles = 0;
            int rangeForeColor = 0, rangeBackColor = 0;
            int rangeEnd = mText.nextSpanTransition(rangeStart, composingEnd, Object.class);

            if (selStart > rangeStart && selStart < rangeEnd) {
                rangeEnd = selStart;
            } else if (selEnd > rangeStart && selEnd < rangeEnd) {
                rangeEnd = selEnd;
            }
            spans = mText.getSpans(rangeStart, rangeEnd, Object.class);

            if (DEBUG) {
                Log.d(LOGTAG, " found " + spans.length + " spans @ " +
                              rangeStart + "-" + rangeEnd);
            }

            if (spans.length == 0) {
                rangeType = (selStart == rangeStart && selEnd == rangeEnd)
                            ? GeckoEvent.IME_RANGE_SELECTEDRAWTEXT
                            : GeckoEvent.IME_RANGE_RAWINPUT;
            } else {
                rangeType = (selStart == rangeStart && selEnd == rangeEnd)
                            ? GeckoEvent.IME_RANGE_SELECTEDCONVERTEDTEXT
                            : GeckoEvent.IME_RANGE_CONVERTEDTEXT;
                for (Object span : spans) {
                    if (span instanceof UnderlineSpan) {
                        rangeStyles |= GeckoEvent.IME_RANGE_UNDERLINE;
                    } else if (span instanceof ForegroundColorSpan) {
                        rangeStyles |= GeckoEvent.IME_RANGE_FORECOLOR;
                        rangeForeColor =
                            ((ForegroundColorSpan)span).getForegroundColor();
                    } else if (span instanceof BackgroundColorSpan) {
                        rangeStyles |= GeckoEvent.IME_RANGE_BACKCOLOR;
                        rangeBackColor =
                            ((BackgroundColorSpan)span).getBackgroundColor();
                    }
                }
            }
            GeckoAppShell.sendEventToGecko(GeckoEvent.createIMERangeEvent(
                    rangeStart - composingStart, rangeEnd - composingStart,
                    rangeType, rangeStyles, rangeForeColor, rangeBackColor));
            rangeStart = rangeEnd;

            if (DEBUG) {
                Log.d(LOGTAG, " added " + rangeType + " : " + rangeStyles +
                              " : " + Integer.toHexString(rangeForeColor) +
                              " : " + Integer.toHexString(rangeBackColor));
            }
        } while (rangeStart < composingEnd);

        GeckoAppShell.sendEventToGecko(GeckoEvent.createIMECompositionEvent(
                composingStart, composingEnd));
    }

    

    @Override
    public void sendEvent(GeckoEvent event) {
        if (DEBUG) {
            
            GeckoApp.assertOnUiThread();
        }
        







        GeckoAppShell.sendEventToGecko(event);
        mActionQueue.offer(new Action(Action.TYPE_EVENT));
    }

    @Override
    public Editable getEditable() {
        if (DEBUG) {
            
            GeckoApp.assertOnUiThread();
        }
        return mProxy;
    }

    @Override
    public void setUpdateGecko(boolean update) {
        if (DEBUG) {
            
            GeckoApp.assertOnUiThread();
        }
        if (update) {
            uiUpdateGecko(false);
        }
        mUpdateGecko = update;
    }

    @Override
    public void setListener(GeckoEditableListener listener) {
        if (DEBUG) {
            
            GeckoApp.assertOnUiThread();
        }
        mListener = listener;
    }

    

    void geckoActionReply() {
        if (DEBUG) {
            
            GeckoApp.assertOnGeckoThread();
        }
        final Action action = mActionQueue.peek();

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
            geckoPostToUI(new Runnable() {
                public void run() {
                    mActionQueue.syncWithGecko();
                    final int start = Selection.getSelectionStart(mText);
                    final int end = Selection.getSelectionEnd(mText);
                    if (mListener != null &&
                            selStart == start && selEnd == end) {
                        
                        
                        mListener.onSelectionChange(start, end);
                    }
                }
            });
            break;
        case Action.TYPE_SET_SPAN:
            mText.setSpan(action.mSpanObject, action.mStart, action.mEnd, action.mSpanFlags);
            break;
        }
        if (action.mShouldUpdate) {
            geckoUpdateGecko(false);
        }
        mActionQueue.poll();
    }

    @Override
    public void notifyIME(final int type, final int state) {
        if (DEBUG) {
            
            GeckoApp.assertOnGeckoThread();
        }
        if (type == NOTIFY_IME_REPLY_EVENT) {
            geckoActionReply();
            return;
        }
        geckoPostToUI(new Runnable() {
            public void run() {
                
                mActionQueue.syncWithGecko();
                if (type == NOTIFY_IME_FOCUSCHANGE) {
                    if (state == IME_FOCUS_STATE_BLUR) {
                        mFocused = false;
                    } else {
                        mFocused = true;
                        
                        GeckoAppShell.sendEventToGecko(GeckoEvent.createIMEEvent(
                                GeckoEvent.IME_ACKNOWLEDGE_FOCUS));
                    }
                }
                if (mListener != null) {
                    mListener.notifyIME(type, state);
                }
            }
        });
    }

    @Override
    public void notifyIMEEnabled(final int state, final String typeHint,
                          final String modeHint, final String actionHint) {
        if (DEBUG) {
            
            GeckoApp.assertOnGeckoThread();
        }
        geckoPostToUI(new Runnable() {
            public void run() {
                
                mActionQueue.syncWithGecko();
                if (mListener != null) {
                    mListener.notifyIMEEnabled(state, typeHint,
                                               modeHint, actionHint);
                }
            }
        });
    }

    @Override
    public void onSelectionChange(final int start, final int end) {
        if (DEBUG) {
            
            GeckoApp.assertOnGeckoThread();
        }
        if (start < 0 || start > mText.length() || end < 0 || end > mText.length()) {
            throw new IllegalArgumentException("invalid selection notification range");
        }
        final int seqnoWhenPosted = ++mGeckoUpdateSeqno;

        geckoPostToUI(new Runnable() {
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

    @Override
    public void onTextChange(final String text, final int start,
                      final int unboundedOldEnd, final int unboundedNewEnd) {
        if (DEBUG) {
            
            GeckoApp.assertOnGeckoThread();
        }
        if (start < 0 || start > unboundedOldEnd) {
            throw new IllegalArgumentException("invalid text notification range");
        }
        

        final int oldEnd = unboundedOldEnd > mText.length() ? mText.length() : unboundedOldEnd;
        
        if (unboundedNewEnd < (start + text.length())) {
            throw new IllegalArgumentException("newEnd does not match text");
        }
        final int newEnd = start + text.length();

        



        ++mGeckoUpdateSeqno;

        if (!mActionQueue.isEmpty()) {
            final Action action = mActionQueue.peek();
            if (action.mType == Action.TYPE_REPLACE_TEXT &&
                    action.mStart == start &&
                    text.equals(action.mSequence.toString())) {
                
                mText.replace(start, oldEnd, action.mSequence,
                              0, action.mSequence.length());
            } else {
                mText.replace(start, oldEnd, text, 0, text.length());
            }
        } else {
            mText.replace(start, oldEnd, text, 0, text.length());
        }
        geckoPostToUI(new Runnable() {
            public void run() {
                if (mListener != null) {
                    mListener.onTextChange(text, start, oldEnd, newEnd);
                }
            }
        });
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

    @Override
    public Object invoke(Object proxy, Method method, Object[] args)
                         throws Throwable {
        Object target;
        final Class<?> methodInterface = method.getDeclaringClass();
        if (DEBUG) {
            
            GeckoApp.assertOnUiThread();
        }
        if (methodInterface == Editable.class ||
                methodInterface == Appendable.class ||
                methodInterface == Spannable.class) {
            
            target = this;
        } else {
            
            
            mActionQueue.syncWithGecko();
            target = mText;
        }
        Object ret = method.invoke(target, args);
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

    

    private static boolean isCompositionSpan(Object what, int flags) {
        return (flags & Spanned.SPAN_COMPOSING) != 0 ||
                what instanceof UnderlineSpan ||
                what instanceof ForegroundColorSpan ||
                what instanceof BackgroundColorSpan;
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

