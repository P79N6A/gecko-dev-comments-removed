



package org.mozilla.gecko.preferences;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.Resources;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class PanelsPreference extends CustomListPreference {
    protected String LOGTAG = "PanelsPreference";

    private static final int INDEX_SHOW_BUTTON = 1;
    private static final int INDEX_REMOVE_BUTTON = 2;

    private String LABEL_HIDE;
    private String LABEL_SHOW;

    protected boolean mIsHidden = false;

    public PanelsPreference(Context context, CustomListCategory parentCategory) {
        super(context, parentCategory);
    }

    @Override
    protected int getPreferenceLayoutResource() {
        return R.layout.preference_panels;
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        
        view.setEnabled(!mIsHidden);

        if (view instanceof ViewGroup) {
            final ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                group.getChildAt(i).setEnabled(!mIsHidden);
            }
        }
    }

    @Override
    protected String[] getDialogStrings() {
        Resources res = getContext().getResources();
        LABEL_HIDE = res.getString(R.string.pref_panels_hide);
        LABEL_SHOW = res.getString(R.string.pref_panels_show);

        
        
        return new String[] { LABEL_SET_AS_DEFAULT,
                              LABEL_HIDE };
    }

    @Override
    public void setIsDefault(boolean isDefault) {
        mIsDefault = isDefault;
        if (isDefault) {
            setSummary(LABEL_IS_DEFAULT);
            if (mIsHidden) {
                
                setHidden(false);
            }
        } else {
            setSummary("");
        }
    }

    @Override
    protected void onDialogIndexClicked(int index) {
        switch(index) {
            case INDEX_SET_DEFAULT_BUTTON:
                mParentCategory.setDefault(this);
                break;

            case INDEX_SHOW_BUTTON:
                ((PanelsPreferenceCategory) mParentCategory).setHidden(this, !mIsHidden);
                break;

            case INDEX_REMOVE_BUTTON:
                mParentCategory.uninstall(this);
                break;

            default:
                Log.w(LOGTAG, "Selected index out of range: " + index);
        }
    }

    @Override
    protected void configureShownDialog() {
        super.configureShownDialog();

        
        final TextView hideButton = (TextView) mDialog.getListView().getChildAt(INDEX_SHOW_BUTTON);
        hideButton.setText(mIsHidden ? LABEL_SHOW : LABEL_HIDE);
    }

    public void setHidden(boolean toHide) {
        if (toHide) {
            setIsDefault(false);
        }

        if (mIsHidden != toHide) {
            mIsHidden = toHide;
            notifyChanged();
        }
    }

    public boolean isHidden() {
        return mIsHidden;
    }
}
