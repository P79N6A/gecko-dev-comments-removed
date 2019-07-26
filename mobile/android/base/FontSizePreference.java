




package org.mozilla.gecko;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.HashMap;

class FontSizePreference extends DialogPreference {
    private static final String LOGTAG = "FontSizePreference";
    private static final int TWIP_TO_PT_RATIO = 20; 
    private static final int PREVIEW_FONT_SIZE_UNIT = TypedValue.COMPLEX_UNIT_PT;
    private static final int DEFAULT_FONT_INDEX = 2;

    private final Context mContext;
    
    private ScrollView mScrollingContainer;
    private TextView mPreviewFontView;
    private Button mIncreaseFontButton;
    private Button mDecreaseFontButton;

    private final String[] mFontTwipValues;
    private final String[] mFontSizeNames; 
    
    private int mSavedFontIndex = DEFAULT_FONT_INDEX;
    
    private int mPreviewFontIndex = mSavedFontIndex;
    private final HashMap<String, Integer> mFontTwipToIndexMap;

    public FontSizePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        final Resources res = mContext.getResources();
        mFontTwipValues = res.getStringArray(R.array.pref_font_size_values);
        mFontSizeNames = res.getStringArray(R.array.pref_font_size_entries);
        mFontTwipToIndexMap = new HashMap<String, Integer>();
        for (int i = 0; i < mFontTwipValues.length; ++i) {
            mFontTwipToIndexMap.put(mFontTwipValues[i], i);
        }
    }

    @Override
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
        final LayoutInflater inflater =
            (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View dialogView = inflater.inflate(R.layout.font_size_preference, null);
        initInternalViews(dialogView);
        updatePreviewFontSize(mFontTwipValues[mPreviewFontIndex]);

        builder.setTitle(null);
        builder.setView(dialogView);
    }

    
    private void initInternalViews(View dialogView) {
        mScrollingContainer = (ScrollView) dialogView.findViewById(R.id.scrolling_container);
        
        mScrollingContainer.setBackgroundColor(Color.WHITE);
        mPreviewFontView = (TextView) dialogView.findViewById(R.id.preview);

        mDecreaseFontButton = (Button) dialogView.findViewById(R.id.decrease_preview_font_button);
        mIncreaseFontButton = (Button) dialogView.findViewById(R.id.increase_preview_font_button);
        setButtonState(mPreviewFontIndex);
        mDecreaseFontButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mPreviewFontIndex = Math.max(mPreviewFontIndex - 1, 0);
                updatePreviewFontSize(mFontTwipValues[mPreviewFontIndex]);
                mIncreaseFontButton.setEnabled(true);
                
                if (mPreviewFontIndex == 0) {
                    mDecreaseFontButton.setEnabled(false);
                }
            }
        });
        mIncreaseFontButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mPreviewFontIndex = Math.min(mPreviewFontIndex + 1, mFontTwipValues.length - 1);
                updatePreviewFontSize(mFontTwipValues[mPreviewFontIndex]);

                mDecreaseFontButton.setEnabled(true);
                
                if (mPreviewFontIndex == mFontTwipValues.length - 1) {
                    mIncreaseFontButton.setEnabled(false);
                }
            }
        });
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        if (!positiveResult) {
            mPreviewFontIndex = mSavedFontIndex;
            return;
        }
        mSavedFontIndex = mPreviewFontIndex;
        final String twipVal = mFontTwipValues[mSavedFontIndex];
        final OnPreferenceChangeListener prefChangeListener = getOnPreferenceChangeListener();
        if (prefChangeListener == null) {
            Log.e(LOGTAG, "PreferenceChangeListener is null. FontSizePreference will not be saved to Gecko.");
            return;
        }
        prefChangeListener.onPreferenceChange(this, twipVal);
    }

    



    protected void setSavedFontSize(String twip) {
        final Integer index = mFontTwipToIndexMap.get(twip);
        if (index != null) {
            mSavedFontIndex = index;
            mPreviewFontIndex = mSavedFontIndex;
            return;
        }
        resetSavedFontSizeToDefault();
        Log.e(LOGTAG, "setSavedFontSize: Given font size does not exist in twip values map. Reverted to default font size.");
    }

    



    private void updatePreviewFontSize(String twip) {
        float pt = convertTwipStrToPT(twip);
        
        
        if (pt == 0) {
            
            
            ViewGroup parentView = (ViewGroup) mScrollingContainer.getParent();
            parentView.removeAllViews();
            final LayoutInflater inflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View dialogView = inflater.inflate(R.layout.font_size_preference, parentView);
            initInternalViews(dialogView);
            mPreviewFontView.setTextSize(PREVIEW_FONT_SIZE_UNIT, 1);
        } else {
            mPreviewFontView.setTextSize(PREVIEW_FONT_SIZE_UNIT, pt);
        }
        mScrollingContainer.scrollTo(0, 0);
    }

    


    private void resetSavedFontSizeToDefault() {
        mSavedFontIndex = DEFAULT_FONT_INDEX;
        mPreviewFontIndex = mSavedFontIndex;
    }

    private void setButtonState(int index) {
        if (index == 0) {
            mDecreaseFontButton.setEnabled(false);
        } else if (index == mFontTwipValues.length - 1) {
            mIncreaseFontButton.setEnabled(false);
        }
    }

    


    protected String getSavedFontSizeName() {
        return mFontSizeNames[mSavedFontIndex];
    }

    private float convertTwipStrToPT(String twip) {
        return Float.parseFloat(twip) / TWIP_TO_PT_RATIO;
    }
}
