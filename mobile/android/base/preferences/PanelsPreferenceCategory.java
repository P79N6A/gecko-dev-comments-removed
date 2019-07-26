



package org.mozilla.gecko.preferences;

import org.mozilla.gecko.home.HomeConfig;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.State;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import android.content.Context;
import android.util.AttributeSet;

public class PanelsPreferenceCategory extends CustomListCategory {
    public static final String LOGTAG = "PanelsPrefCategory";

    protected HomeConfig mHomeConfig;
    protected HomeConfig.Editor mConfigEditor;

    
    private static final int PANEL_PREFS_OFFSET = 1;

    protected UiAsyncTask<Void, Void, HomeConfig.State> mLoadTask;

    public PanelsPreferenceCategory(Context context) {
        super(context);
        initConfig(context);
    }

    public PanelsPreferenceCategory(Context context, AttributeSet attrs) {
        super(context, attrs);
        initConfig(context);
    }

    public PanelsPreferenceCategory(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initConfig(context);
    }

    protected void initConfig(Context context) {
        mHomeConfig = HomeConfig.getDefault(context);
    }

    @Override
    public void onAttachedToActivity() {
        super.onAttachedToActivity();

        loadHomeConfig();
    }

    


    private void loadHomeConfig() {
        mLoadTask = new UiAsyncTask<Void, Void, HomeConfig.State>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public HomeConfig.State doInBackground(Void... params) {
                return mHomeConfig.load();
            }

            @Override
            public void onPostExecute(HomeConfig.State configState) {
                mConfigEditor = configState.edit();
                displayHomeConfig(configState);
            }
        };
        mLoadTask.execute();
    }

    public void refresh() {
        refresh(null);
    }

    




    public void refresh(State state) {
        
        
        int prefCount = getPreferenceCount();
        while (prefCount > 1) {
            removePreference(getPreference(1));
            prefCount--;
        }

        if (state == null) {
            loadHomeConfig();
        } else {
            displayHomeConfig(state);
        }
    }

    private void displayHomeConfig(HomeConfig.State configState) {
        int index = 0;
        for (PanelConfig panelConfig : configState) {
            final boolean isRemovable = panelConfig.isDynamic();

            
            final PanelsPreference pref = new PanelsPreference(getContext(), PanelsPreferenceCategory.this, isRemovable, index);
            pref.setTitle(panelConfig.getTitle());
            pref.setKey(panelConfig.getId());
            
            addPreference(pref);

            if (panelConfig.isDisabled()) {
                pref.setHidden(true);
            }

            index++;
        }

        setPositionState();
        setDefaultFromConfig();
    }

    private void setPositionState() {
        final int prefCount = getPreferenceCount();

        
        final PanelsPreference firstPref = (PanelsPreference) getPreference(PANEL_PREFS_OFFSET);
        firstPref.setIsFirst();

        final PanelsPreference lastPref = (PanelsPreference) getPreference(prefCount - 1);
        lastPref.setIsLast();
    }

    private void setDefaultFromConfig() {
        final String defaultPanelId = mConfigEditor.getDefaultPanelId();
        if (defaultPanelId == null) {
            mDefaultReference = null;
            return;
        }

        final int prefCount = getPreferenceCount();

        
        for (int i = 1; i < prefCount; i++) {
            final PanelsPreference pref = (PanelsPreference) getPreference(i);

            if (defaultPanelId.equals(pref.getKey())) {
                super.setDefault(pref);
                break;
            }
        }
    }

    @Override
    public void setDefault(CustomListPreference pref) {
        super.setDefault(pref);

        final String id = pref.getKey();

        final String defaultPanelId = mConfigEditor.getDefaultPanelId();
        if (defaultPanelId != null && defaultPanelId.equals(id)) {
            return;
        }

        mConfigEditor.setDefault(id);
        mConfigEditor.apply();
    }

    @Override
    protected void onPrepareForRemoval() {
        if (mLoadTask != null) {
            mLoadTask.cancel(true);
        }
    }

    @Override
    public void uninstall(CustomListPreference pref) {
        mConfigEditor.uninstall(pref.getKey());
        mConfigEditor.apply();

        super.uninstall(pref);
    }

    public void moveUp(PanelsPreference pref) {
        final int panelIndex = pref.getIndex();
        if (panelIndex > 0) {
            mConfigEditor.moveTo(pref.getKey(), panelIndex - 1);
            final State state = mConfigEditor.apply();
            refresh(state);
        }
    }

    public void moveDown(PanelsPreference pref) {
        final int panelIndex = pref.getIndex();
        if (panelIndex < getPreferenceCount() - 1) {
            mConfigEditor.moveTo(pref.getKey(), panelIndex + 1);
            final State state = mConfigEditor.apply();
            refresh(state);
        }
    }

    






    protected void setHidden(PanelsPreference pref, boolean toHide) {
        mConfigEditor.setDisabled(pref.getKey(), toHide);
        mConfigEditor.apply();

        pref.setHidden(toHide);
        setDefaultFromConfig();
    }

    



    @Override
    protected void setFallbackDefault() {
        setDefaultFromConfig();
    }
}
