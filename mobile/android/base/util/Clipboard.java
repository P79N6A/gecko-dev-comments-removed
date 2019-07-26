



package org.mozilla.gecko.util;

import android.content.ClipData;
import android.content.Context;
import android.os.Build;
import android.util.Log;

import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;

import java.util.concurrent.SynchronousQueue;

public final class Clipboard {
    private static Context mContext;
    private final static String LOG_TAG = "Clipboard";
    private final static SynchronousQueue<String> sClipboardQueue = new SynchronousQueue<String>();

    private Clipboard() {
    }

    public static void init(Context c) {
        if (mContext != null) {
            Log.w(LOG_TAG, "Clipboard.init() called twice!");
            return;
        }
        mContext = c;
    }

    @WrapElementForJNI(stubName = "GetClipboardTextWrapper")
    public static String getText() {
        
        
        

        if (ThreadUtils.isOnUiThread() || ThreadUtils.isOnBackgroundThread()) {
            return getClipboardTextImpl();
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                String text = getClipboardTextImpl();
                try {
                    sClipboardQueue.put(text != null ? text : "");
                } catch (InterruptedException ie) {}
            }
        });
        try {
            return sClipboardQueue.take();
        } catch (InterruptedException ie) {
            return "";
        }
    }

    @WrapElementForJNI(stubName = "SetClipboardText")
    public static void setText(final CharSequence text) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            @SuppressWarnings("deprecation")
            public void run() {
                if (Build.VERSION.SDK_INT >= 11) {
                    android.content.ClipboardManager cm = getClipboardManager11(mContext);
                    ClipData clip = ClipData.newPlainText("Text", text);
                    try {
                        cm.setPrimaryClip(clip);
                    } catch (NullPointerException e) {
                        
                        
                        
                    }
                } else {
                    android.text.ClipboardManager cm = getClipboardManager(mContext);
                    cm.setText(text);
                }
            }
        });
    }

    




    @WrapElementForJNI
    public static boolean hasText() {
        String text = getText();
        return text != null;
    }

    


    @WrapElementForJNI
    public static void clearText() {
        setText(null);
    }

    private static android.content.ClipboardManager getClipboardManager11(Context context) {
        
        
        return (android.content.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
    }

    private static android.text.ClipboardManager getClipboardManager(Context context) {
        return (android.text.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
    }

    


    @SuppressWarnings("deprecation")
    private static String getClipboardTextImpl() {
        if (Build.VERSION.SDK_INT >= 11) {
            android.content.ClipboardManager cm = getClipboardManager11(mContext);
            if (cm.hasPrimaryClip()) {
                ClipData clip = cm.getPrimaryClip();
                if (clip != null) {
                    ClipData.Item item = clip.getItemAt(0);
                    return item.coerceToText(mContext).toString();
                }
            }
        } else {
            android.text.ClipboardManager cm = getClipboardManager(mContext);
            if (cm.hasText()) {
                return cm.getText().toString();
            }
        }
        return null;
    }
}
