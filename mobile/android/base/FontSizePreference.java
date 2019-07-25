




package org.mozilla.gecko;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Resources;
import android.preference.DialogPreference;
import android.text.method.ScrollingMovementMethod;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

class FontSizePreference extends DialogPreference {
    private static final String LOGTAG = "FontSizePreference";
    private static final int TWIP_TO_PT_RATIO = 20; 
    private static final int PREVIEW_FONT_SIZE_UNIT = TypedValue.COMPLEX_UNIT_PT;
    private static final int DEFAULT_FONT_INDEX = 2;

    private final Context mContext;
    private TextView mPreviewFontView;
    private Button mIncreaseFontButton;
    private Button mDecreaseFontButton;

    private final String[] mFontTwipValues;
    
    private int mSavedFontIndex = DEFAULT_FONT_INDEX;
    
    private int mPreviewFontIndex = mSavedFontIndex;
    public FontSizePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        final Resources res = mContext.getResources();
        mFontTwipValues = res.getStringArray(R.array.pref_font_size_values);
    }

    @Override
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
        final LayoutInflater inflater =
            (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View dialogView = inflater.inflate(R.layout.font_size_preference, null);
        mPreviewFontView = (TextView) dialogView.findViewById(R.id.preview);
        mPreviewFontView.setMovementMethod(new ScrollingMovementMethod());

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

        builder.setView(dialogView);
    }

    



    private void updatePreviewFontSize(String twip) {
        mPreviewFontView.setTextSize(PREVIEW_FONT_SIZE_UNIT, convertTwipStrToPT(twip));
        mPreviewFontView.invalidate();
    }

    private float convertTwipStrToPT(String twip) {
        return Float.parseFloat(twip) / TWIP_TO_PT_RATIO; 
    }
}
