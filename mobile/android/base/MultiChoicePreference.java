




package org.mozilla.gecko;

import org.mozilla.gecko.util.ThreadUtils;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.preference.DialogPreference;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.widget.Button;

class MultiChoicePreference extends DialogPreference {
    private static final String LOGTAG = "GeckoMultiChoicePreference";

    private boolean mValues[];
    private boolean mPrevValues[];
    private CharSequence mEntryKeys[];
    private CharSequence mEntries[];
    private CharSequence mInitialValues[];

    public MultiChoicePreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.MultiChoicePreference);
        mEntries = a.getTextArray(R.styleable.MultiChoicePreference_entries);
        mEntryKeys = a.getTextArray(R.styleable.MultiChoicePreference_entryKeys);
        mInitialValues = a.getTextArray(R.styleable.MultiChoicePreference_initialValues);
        a.recycle();

        loadPersistedValues();
    }

    public MultiChoicePreference(Context context) {
        this(context, null);
    }

    









    public void setEntries(CharSequence[] entries) {
        mEntries = entries.clone();
    }
    
    


    public void setEntries(int entriesResId) {
        setEntries(getContext().getResources().getTextArray(entriesResId));
    }

    




    public void setEntryKeys(CharSequence[] entryKeys) {
        mEntryKeys = entryKeys.clone();
        loadPersistedValues();
    }

    


    public void setEntryKeys(int entryKeysResId) {
        setEntryKeys(getContext().getResources().getTextArray(entryKeysResId));
    }

    







    public void setInitialValues(CharSequence[] initialValues) {
        mInitialValues = initialValues.clone();
        loadPersistedValues();
    }

    


    public void setInitialValues(int initialValuesResId) {
        setInitialValues(getContext().getResources().getTextArray(initialValuesResId));
    }

    




    public CharSequence[] getEntries() {
        return mEntries.clone();
    }

    




    public CharSequence[] getEntryKeys() {
        return mEntryKeys.clone();
    }

    





    public CharSequence[] getInitialValues() {
        return mInitialValues.clone();
    }

    





    public boolean[] getValues() {
        return mValues.clone();
    }

    @Override
    protected void onPrepareDialogBuilder(Builder builder) {
        if (mEntries == null || mEntryKeys == null || mInitialValues == null) {
            throw new IllegalStateException(
                    "MultiChoicePreference requires entries, entryKeys, and initialValues arrays.");
        }

        if (mEntries.length != mEntryKeys.length || mEntryKeys.length != mInitialValues.length) {
            throw new IllegalStateException(
                    "MultiChoicePreference entries, entryKeys, and initialValues arrays must be the same length");
        }

        builder.setMultiChoiceItems(mEntries, mValues, new DialogInterface.OnMultiChoiceClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which, boolean val) {
                

                
                boolean enabled = false;
                for (int i = 0; i < mValues.length; i++) {
                    if (mValues[i]) {
                        enabled = true;
                        break;
                    }
                }
                Button button = ((AlertDialog) dialog).getButton(DialogInterface.BUTTON_POSITIVE);
                if (button.isEnabled() != enabled)
                    button.setEnabled(enabled);
            }
        });
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        if (mPrevValues == null || mInitialValues == null) {
            
            
            return;
        }

        if (!positiveResult) {
            
            mValues = mPrevValues.clone();
            return;
        } else {
            mPrevValues = mValues.clone();
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < mEntryKeys.length; i++) {
                    String key = mEntryKeys[i].toString();
                    persistBoolean(key, mValues[i]);
                }
            }
        });
    }

    protected boolean persistBoolean(String key, boolean value) {
        if (isPersistent()) {
            if (value == getPersistedBoolean(!value)) {
                
                return true;
            }
            
            PreferenceManager.getDefaultSharedPreferences(getContext())
                             .edit().putBoolean(key, value).commit();
            return true;
        }
        return false;
    }

    protected boolean getPersistedBoolean(String key, boolean defaultReturnValue) {
        if (!isPersistent())
            return defaultReturnValue;
        
        return PreferenceManager.getDefaultSharedPreferences(getContext())
                                .getBoolean(key, defaultReturnValue);
    }

    




    private void loadPersistedValues() {
        if (mEntryKeys == null || mInitialValues == null)
            return;

        final int entryCount = mEntryKeys.length;
        if (entryCount != mEntries.length || entryCount != mInitialValues.length) {
            throw new IllegalStateException(
                    "MultiChoicePreference entryKeys and initialValues arrays must be the same length");
        }

        mValues = new boolean[entryCount];
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < entryCount; i++) {
                    String key = mEntryKeys[i].toString();
                    boolean initialValue = mInitialValues[i].equals("true");
                    mValues[i] = getPersistedBoolean(key, initialValue);
                }
                mPrevValues = mValues.clone();
            }
        });
    }
}
