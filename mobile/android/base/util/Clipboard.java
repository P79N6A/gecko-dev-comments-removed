



package org.mozilla.gecko.util;

import java.util.concurrent.SynchronousQueue;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;

import android.content.ClipData;
import android.content.Context;
import android.util.Log;

public final class Clipboard {
    
    
    
    volatile static Context mContext;
    private final static String LOGTAG = "GeckoClipboard";
    private final static SynchronousQueue<String> sClipboardQueue = new SynchronousQueue<String>();

    private Clipboard() {
    }

    public static void init(final Context c) {
        if (mContext != null) {
            Log.w(LOGTAG, "Clipboard.init() called twice!");
            return;
        }
        mContext = c.getApplicationContext();
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
                
                
                if (Versions.feature11Plus) {
                    final android.content.ClipboardManager cm = (android.content.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
                    final ClipData clip = ClipData.newPlainText("Text", text);
                    try {
                        cm.setPrimaryClip(clip);
                    } catch (NullPointerException e) {
                        
                        
                        
                    }
                    return;
                }

                
                android.text.ClipboardManager cm = (android.text.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
                cm.setText(text);
            }
        });
    }

    


    @WrapElementForJNI
    public static boolean hasText() {
        if (Versions.feature11Plus) {
            android.content.ClipboardManager cm = (android.content.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
            return cm.hasPrimaryClip();
        }

        
        android.text.ClipboardManager cm = (android.text.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
        return cm.hasText();
    }

    


    @WrapElementForJNI
    public static void clearText() {
        setText(null);
    }

    




    @SuppressWarnings("deprecation")
    static String getClipboardTextImpl() {
        if (Versions.feature11Plus) {
            android.content.ClipboardManager cm = (android.content.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm.hasPrimaryClip()) {
                ClipData clip = cm.getPrimaryClip();
                if (clip != null) {
                    ClipData.Item item = clip.getItemAt(0);
                    return item.coerceToText(mContext).toString();
                }
            }
        } else {
            android.text.ClipboardManager cm = (android.text.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm.hasText()) {
                return cm.getText().toString();
            }
        }
        return null;
    }
}
