



package org.mozilla.gecko.preferences;

import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnShowListener;
import android.content.res.Resources;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class PanelsPreference extends CustomListPreference {
    protected String LOGTAG = "PanelsPreference";

    
    private static final int STATE_IS_FIRST = 0;
    private static final int STATE_IS_LAST = 1;

    



    private static final int INDEX_DISPLAY_BUTTON = 1;
    private static final int INDEX_REORDER_BUTTON = 2;

    
    private static final int INDEX_MOVE_UP_BUTTON = 0;
    private static final int INDEX_MOVE_DOWN_BUTTON = 1;

    private String LABEL_HIDE;
    private String LABEL_SHOW;

    private View preferenceView;
    protected boolean mIsHidden = false;

    private boolean mAnimate;
    private static final int ANIMATION_DURATION_MS = 400;

    
    private int mPositionState = -1;
    private final int mIndex;

    public PanelsPreference(Context context, CustomListCategory parentCategory, int index, boolean animate) {
        super(context, parentCategory);
        mIndex = index;
        mAnimate = animate;
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
            preferenceView = group;
        }

        if (mAnimate) {
            ViewHelper.setAlpha(preferenceView, 0);

            final PropertyAnimator animator = new PropertyAnimator(ANIMATION_DURATION_MS);
            animator.attach(preferenceView, Property.ALPHA, 1);
            animator.start();

            
            mAnimate = false;
        }
    }

    @Override
    protected String[] createDialogItems() {
        final Resources res = getContext().getResources();
        final String labelReorder = res.getString(R.string.pref_panels_reorder);

        LABEL_HIDE = res.getString(R.string.pref_panels_hide);
        LABEL_SHOW = res.getString(R.string.pref_panels_show);

        return new String[] { LABEL_SET_AS_DEFAULT, LABEL_HIDE, labelReorder };
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

            case INDEX_DISPLAY_BUTTON:
                
                ((PanelsPreferenceCategory) mParentCategory).setHidden(this, !mIsHidden);
                break;

            case INDEX_REORDER_BUTTON:
                
                final Dialog orderDialog = makeReorderDialog();
                orderDialog.show();
                break;

            default:
                Log.w(LOGTAG, "Selected index out of range: " + index);
        }
    }

    @Override
    protected void configureShownDialog() {
        super.configureShownDialog();

        
        final TextView hideButton = (TextView) mDialog.getListView().getChildAt(INDEX_DISPLAY_BUTTON);
        hideButton.setText(mIsHidden ? LABEL_SHOW : LABEL_HIDE);
    }


    private Dialog makeReorderDialog() {
        final AlertDialog.Builder builder = new AlertDialog.Builder(getContext());

        final Resources res = getContext().getResources();
        final String labelUp = res.getString(R.string.pref_panels_move_up);
        final String labelDown = res.getString(R.string.pref_panels_move_down);

        builder.setTitle(getTitle());
        builder.setItems(new String[] { labelUp, labelDown }, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int index) {
                dialog.dismiss();
                switch (index) {
                    case INDEX_MOVE_UP_BUTTON:
                        ((PanelsPreferenceCategory) mParentCategory).moveUp(PanelsPreference.this);
                        break;

                    case INDEX_MOVE_DOWN_BUTTON:
                        ((PanelsPreferenceCategory) mParentCategory).moveDown(PanelsPreference.this);
                        break;
                }
            }
        });

        final Dialog dialog = builder.create();
        dialog.setOnShowListener(new OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
               setReorderItemsEnabled(dialog);
            }
        });

        return dialog;
    }

    public void setIsFirst() {
        mPositionState = STATE_IS_FIRST;
    }

    public void setIsLast() {
        mPositionState = STATE_IS_LAST;
    }

    



    private void setReorderItemsEnabled(DialogInterface dialog) {
        
        switch (mPositionState) {
            case STATE_IS_FIRST:
                final TextView itemUp = (TextView) ((AlertDialog) dialog).getListView().getChildAt(INDEX_MOVE_UP_BUTTON);
                itemUp.setEnabled(false);
                
                itemUp.setOnClickListener(null);
                break;

            case STATE_IS_LAST:
                final TextView itemDown = (TextView) ((AlertDialog) dialog).getListView().getChildAt(INDEX_MOVE_DOWN_BUTTON);
                itemDown.setEnabled(false);
                
                itemDown.setOnClickListener(null);
                break;

            default:
                
                break;
        }
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

    public int getIndex() {
        return mIndex;
    }
}
