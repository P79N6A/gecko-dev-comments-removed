




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.R;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.util.PrefUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.content.SharedPreferences;
import android.preference.DialogPreference;
import android.util.AttributeSet;

import java.util.HashSet;
import java.util.Set;

class MultiChoicePreference extends DialogPreference implements DialogInterface.OnMultiChoiceClickListener {
    private static final String LOGTAG = "GeckoMultiChoicePreference";

    private boolean mValues[];
    private boolean mPrevValues[];
    private CharSequence mEntryValues[];
    private CharSequence mEntries[];
    private CharSequence mInitialValues[];

    public MultiChoicePreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.MultiChoicePreference);
        mEntries = a.getTextArray(R.styleable.MultiChoicePreference_entries);
        mEntryValues = a.getTextArray(R.styleable.MultiChoicePreference_entryValues);
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

    




    public void setEntryValues(CharSequence[] entryValues) {
        mEntryValues = entryValues.clone();
        loadPersistedValues();
    }

    




    public void setEntryValues(int entryValuesResId) {
        setEntryValues(getContext().getResources().getTextArray(entryValuesResId));
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

    




    public CharSequence[] getEntryValues() {
        return mEntryValues.clone();
    }

    





    public CharSequence[] getInitialValues() {
        return mInitialValues.clone();
    }

    public void setValue(final int i, final boolean value) {
        mValues[i] = value;
        mPrevValues = mValues.clone();
    }

    





    public Set<String> getValues() {
        final Set<String> values = new HashSet<String>();

        if (mValues == null) {
            return values;
        }

        for (int i = 0; i < mValues.length; i++) {
            if (mValues[i]) {
                values.add(mEntryValues[i].toString());
            }
        }

        return values;
    }

    @Override
    public void onClick(DialogInterface dialog, int which, boolean val) {
    }

    @Override
    protected void onPrepareDialogBuilder(Builder builder) {
        if (mEntries == null || mInitialValues == null || mEntryValues == null) {
            throw new IllegalStateException(
                    "MultiChoicePreference requires entries, entryValues, and initialValues arrays.");
        }

        if (mEntries.length != mEntryValues.length || mEntries.length != mInitialValues.length) {
            throw new IllegalStateException(
                    "MultiChoicePreference entries, entryValues, and initialValues arrays must be the same length");
        }

        builder.setMultiChoiceItems(mEntries, mValues, this);
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

        if (!callChangeListener(getValues())) {
            return;
        }

        persist();
    }

    
    public boolean persist() {
        if (isPersistent()) {
            final SharedPreferences.Editor edit = GeckoSharedPrefs.forProfile(getContext()).edit();
            final boolean res = persist(edit);
            edit.apply();
            return res;
        }

        return false;
    }

    
    protected boolean persist(SharedPreferences.Editor edit) {
        if (isPersistent()) {
            Set<String> vals = getValues();
            PrefUtils.putStringSet(edit, getKey(), vals).apply();;
            return true;
        }

        return false;
    }

    
    public Set<String> getPersistedStrings(Set<String> defaultVal) {
        if (!isPersistent()) {
            return defaultVal;
        }

        final SharedPreferences prefs = GeckoSharedPrefs.forProfile(getContext());
        return PrefUtils.getStringSet(prefs, getKey(), defaultVal);
    }

    




    protected void loadPersistedValues() {
        final int entryCount = mInitialValues.length;
        mValues = new boolean[entryCount];

        if (entryCount != mEntries.length || entryCount != mEntryValues.length) {
            throw new IllegalStateException(
                    "MultiChoicePreference entryValues and initialValues arrays must be the same length");
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final Set<String> stringVals = getPersistedStrings(null);

                for (int i = 0; i < entryCount; i++) {
                    if (stringVals != null) {
                        mValues[i] = stringVals.contains(mEntryValues[i]);
                    } else {
                        final boolean defaultVal = mInitialValues[i].equals("true");
                        mValues[i] = defaultVal;
                    }
                }

                mPrevValues = mValues.clone();
            }
        });
    }
}
