




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.R;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.content.SharedPreferences;
import android.widget.Button;
import android.util.AttributeSet;
import android.util.Log;

import java.util.Set;




class MultiPrefMultiChoicePreference extends MultiChoicePreference {
    private static final String LOGTAG = "GeckoMultiPrefPreference";
    private static final String IMPORT_SUFFIX = "_imported_";
    private final CharSequence[] keys;

    public MultiPrefMultiChoicePreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.MultiPrefMultiChoicePreference);
        keys = a.getTextArray(R.styleable.MultiPrefMultiChoicePreference_entryKeys);
        a.recycle();

        loadPersistedValues();
    }

    
    private boolean getPersistedBoolean(SharedPreferences prefs, String key, boolean defaultReturnValue) {
        if (!isPersistent()) {
            return defaultReturnValue;
        }

        return prefs.getBoolean(key, defaultReturnValue);
    }

    
    @Override
    protected synchronized void loadPersistedValues() {
        
        super.loadPersistedValues();

        
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(getContext());
        final boolean imported = getPersistedBoolean(prefs, getKey() + IMPORT_SUFFIX, false);
        if (imported) {
            return;
        }

        
        final CharSequence[] init = getInitialValues();
        final CharSequence[] entries = getEntries();
        if (keys == null || init == null) {
            return;
        }

        final int entryCount = keys.length;
        if (entryCount != entries.length || entryCount != init.length) {
            throw new IllegalStateException("MultiChoicePreference entryKeys and initialValues arrays must be the same length");
        }

        
        final SharedPreferences.Editor edit = prefs.edit();
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                try {
                    
                    for (int i = 0; i < entryCount; i++) {
                        String key = keys[i].toString();
                        boolean initialValue = "true".equals(init[i]);
                        boolean val = getPersistedBoolean(prefs, key, initialValue);

                        
                        setValue(i, val);
                        edit.remove(key);
                    }

                    persist(edit);
                    edit.putBoolean(getKey() + IMPORT_SUFFIX, true);
                    edit.commit();
                } catch(Exception ex) {
                    Log.i(LOGTAG, "Err", ex);
                }
            }
        });
    }


    @Override
    public void onClick(DialogInterface dialog, int which, boolean val) {
        
        boolean enabled = false;
        final Set<String> values = getValues();

        enabled = (values.size() > 0);
        final Button button = ((AlertDialog) dialog).getButton(DialogInterface.BUTTON_POSITIVE);
        if (button.isEnabled() != enabled) {
            button.setEnabled(enabled);
        }
    }

}
