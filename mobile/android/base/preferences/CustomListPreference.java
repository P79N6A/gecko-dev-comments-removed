



package org.mozilla.gecko.preferences;

import org.mozilla.gecko.R;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.preference.Preference;
import android.view.View;
import android.widget.TextView;







public abstract class CustomListPreference extends Preference implements View.OnLongClickListener {
    protected String LOGTAG = "CustomListPreference";

    
    public static final int INDEX_SET_DEFAULT_BUTTON = 0;

    
    private String[] mDialogItems;

    
    protected AlertDialog mDialog;

    
    protected final String LABEL_IS_DEFAULT;
    protected final String LABEL_SET_AS_DEFAULT;
    protected final String LABEL_REMOVE;

    protected boolean mIsDefault;

    
    protected final CustomListCategory mParentCategory;

    






    public CustomListPreference(Context context, CustomListCategory parentCategory) {
        super(context);

        mParentCategory = parentCategory;
        setLayoutResource(getPreferenceLayoutResource());

        setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                CustomListPreference sPref = (CustomListPreference) preference;
                sPref.showDialog();
                return true;
            }
        });

        Resources res = getContext().getResources();

        
        LABEL_IS_DEFAULT = res.getString(R.string.pref_default);
        LABEL_SET_AS_DEFAULT = res.getString(R.string.pref_dialog_set_default);
        LABEL_REMOVE = res.getString(R.string.pref_dialog_remove);
    }

    


    protected abstract int getPreferenceLayoutResource();

    








    public void setIsDefault(boolean isDefault) {
        mIsDefault = isDefault;
        if (isDefault) {
            setOrder(0);
            setSummary(LABEL_IS_DEFAULT);
        } else {
            setOrder(1);
            setSummary("");
        }
    }

    private String[] getCachedDialogItems() {
        if (mDialogItems == null) {
            mDialogItems = createDialogItems();
        }
        return mDialogItems;
    }

    


    abstract protected String[] createDialogItems();

    


    public void showDialog() {
        final AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle(getTitle().toString());
        builder.setItems(getCachedDialogItems(), new DialogInterface.OnClickListener() {
            
            @Override
            public void onClick(DialogInterface dialog, int indexClicked) {
                hideDialog();
                onDialogIndexClicked(indexClicked);
            }
        });

        configureDialogBuilder(builder);

        
        mDialog = builder.create();
        mDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            
            @Override
            public void onShow(DialogInterface dialog) {
                configureShownDialog();
            }
        });
        mDialog.show();
    }

    


    protected void configureDialogBuilder(AlertDialog.Builder builder) {
        return;
    }

    abstract protected void onDialogIndexClicked(int index);

    




    protected void configureShownDialog() {
        
        final TextView defaultButton = (TextView) mDialog.getListView().getChildAt(INDEX_SET_DEFAULT_BUTTON);
        if (mIsDefault) {
            defaultButton.setEnabled(false);

            
            
            defaultButton.setOnClickListener(null);
        }
    }

    


    public void hideDialog() {
        if (mDialog != null && mDialog.isShowing()) {
            mDialog.dismiss();
        }
    }

    @Override
    public boolean onLongClick(View view) {
        
        showDialog();
        return true;
    }
}
