



package org.mozilla.gecko.preferences;

import android.content.Context;
import android.preference.PreferenceCategory;
import android.util.AttributeSet;

public abstract class CustomListCategory extends PreferenceCategory {
    protected CustomListPreference mDefaultReference;

    public CustomListCategory(Context context) {
        super(context);
    }

    public CustomListCategory(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public CustomListCategory(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onAttachedToActivity() {
        super.onAttachedToActivity();

        setOrderingAsAdded(true);
    }

    



    protected void setFallbackDefault() {
        if (getPreferenceCount() > 0) {
            CustomListPreference aItem = (CustomListPreference) getPreference(0);
            setDefault(aItem);
        }
    }

    





    public void uninstall(CustomListPreference item) {
        removePreference(item);
        if (item == mDefaultReference) {
            
            setFallbackDefault();
        }
    }

    





    public void setDefault(CustomListPreference item) {
        if (mDefaultReference != null) {
            mDefaultReference.setIsDefault(false);
        }

        item.setIsDefault(true);
        mDefaultReference = item;
    }
}
