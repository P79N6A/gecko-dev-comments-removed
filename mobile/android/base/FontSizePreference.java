




package org.mozilla.gecko;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.preference.DialogPreference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.text.method.ScrollingMovementMethod;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.widget.Button;
import android.widget.TextView;

import java.util.HashMap;

class FontSizePreference extends DialogPreference {
    private static final String LOGTAG = "FontSizePreference";
    private static final int TWIP_TO_PT_RATIO = 20; 
    private static final int PREVIEW_FONT_SIZE_UNIT = TypedValue.COMPLEX_UNIT_PT;
    private static final int DEFAULT_FONT_INDEX = 2;

    
    private static final float LINE_SPACING_ADD = 0f; 
    private static final float LINE_SPACING_MULT = 1.0f;

    private static final float MIN_TEXTVIEW_WIDTH_DIP = 360; 
    
    private static final float TEXTVIEW_WIDTH_SCALE_FACTOR = 3;

    private final Context mContext;
    private int mCurrentOrientation;
    private TextView mPreviewFontView;
    private Button mIncreaseFontButton;
    private Button mDecreaseFontButton;

    private final String[] mFontTwipValues;
    private final String[] mFontSizeNames; 
    
    private int mSavedFontIndex = DEFAULT_FONT_INDEX;
    
    private int mPreviewFontIndex = mSavedFontIndex;
    private final HashMap<String, Integer> mFontTwipToIndexMap;

    private boolean mPreviewFontViewHeightSet;

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
        mCurrentOrientation = res.getConfiguration().orientation;
    }

    @Override
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
        final LayoutInflater inflater =
            (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View dialogView = inflater.inflate(R.layout.font_size_preference, null);
        mPreviewFontView = (TextView) dialogView.findViewById(R.id.preview);
        mPreviewFontView.setMovementMethod(new ScrollingMovementMethod());
        
        mPreviewFontView.setIncludeFontPadding(false);
        
        mPreviewFontView.setLineSpacing(LINE_SPACING_ADD, LINE_SPACING_MULT);

        mDecreaseFontButton = (Button) dialogView.findViewById(R.id.decrease_preview_font_button);
        mIncreaseFontButton = (Button) dialogView.findViewById(R.id.increase_preview_font_button);
        mDecreaseFontButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                updatePreviewFontSize(mFontTwipValues[--mPreviewFontIndex]);
                mIncreaseFontButton.setEnabled(true);
                
                if (mPreviewFontIndex == 0) {
                    mDecreaseFontButton.setEnabled(false);
                }
            }
        });
        mIncreaseFontButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                updatePreviewFontSize(mFontTwipValues[++mPreviewFontIndex]);
        
                mDecreaseFontButton.setEnabled(true);
                
                if (mPreviewFontIndex == mFontTwipValues.length - 1) {
                    mIncreaseFontButton.setEnabled(false);
                }
            }
        });

        
        setPreviewFontViewWidth();
        setPreviewFontViewHeightListener();
        setFontSizeToMaximum(); 

        builder.setView(dialogView);
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

    protected void onConfigurationChanged(Configuration newConfig) {
        if (mCurrentOrientation != newConfig.orientation) {
            mCurrentOrientation = newConfig.orientation;

            
            if (mPreviewFontView != null) {
                
                setPreviewFontViewWidth();
                mPreviewFontViewHeightSet = false;
                setFontSizeToMaximum(); 
            }
        }
    }

    


    private void setFontSizeToMaximum() {
        updatePreviewFontSize(mFontTwipValues[mFontTwipValues.length - 1]);
        
        
        
        
    }

    





    private void setPreviewFontViewHeightListener() {
        mPreviewFontViewHeightSet = false;
        final ViewTreeObserver vto = mPreviewFontView.getViewTreeObserver(); 
        vto.addOnGlobalLayoutListener(new OnGlobalLayoutListener() { 
            @Override 
            public void onGlobalLayout() { 
                if (!mPreviewFontViewHeightSet) {
                    mPreviewFontViewHeightSet = true;
                    final int desiredHeight = (int) (mPreviewFontView.getLineCount() *
                            mPreviewFontView.getLineHeight() * LINE_SPACING_MULT + LINE_SPACING_ADD +
                            mPreviewFontView.getPaddingTop() + mPreviewFontView.getPaddingBottom());
                    mPreviewFontView.setHeight(desiredHeight);

                    
                    setButtonState(mPreviewFontIndex);
                    updatePreviewFontSize(mFontTwipValues[mPreviewFontIndex]);
                }
            } 
        }); 
    }

    






    private void setPreviewFontViewWidth() {
        final DisplayMetrics metrics = mContext.getResources().getDisplayMetrics();
        final float density = metrics.density;

        final float actualWidthDip = metrics.widthPixels / density;
        float scaleExtraDip = (actualWidthDip - MIN_TEXTVIEW_WIDTH_DIP) / TEXTVIEW_WIDTH_SCALE_FACTOR;
        scaleExtraDip = scaleExtraDip >= 0 ? scaleExtraDip : 0;
        final float desiredWidthDip = MIN_TEXTVIEW_WIDTH_DIP + scaleExtraDip;
        final int desiredWidthPx = Math.round(desiredWidthDip * density);
        mPreviewFontView.setWidth(desiredWidthPx);
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
            mPreviewFontView.setTextSize(PREVIEW_FONT_SIZE_UNIT, 1);
        } else {
            mPreviewFontView.setTextSize(PREVIEW_FONT_SIZE_UNIT, pt);
        }
        mPreviewFontView.scrollTo(0, 0);
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
