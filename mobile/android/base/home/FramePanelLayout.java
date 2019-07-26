




package org.mozilla.gecko.home;

import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;

import android.content.Context;
import android.util.Log;
import android.view.View;

class FramePanelLayout extends PanelLayout {
    private static final String LOGTAG = "GeckoFramePanelLayout";

    private final View mChildView;
    private final ViewConfig mChildConfig;

    public FramePanelLayout(Context context, PanelConfig panelConfig, DatasetHandler datasetHandler) {
        super(context, panelConfig, datasetHandler);

        
        
        mChildConfig = panelConfig.getViewAt(0);
        if (mChildConfig == null) {
            throw new IllegalStateException("FramePanelLayout requires a view in PanelConfig");
        }

        mChildView = createPanelView(mChildConfig);
        addView(mChildView);
    }

    @Override
    public void load() {
        Log.d(LOGTAG, "Loading");

        if (mChildView instanceof DatasetBacked) {
            Log.d(LOGTAG, "Requesting child dataset: " + mChildConfig.getDatasetId());
            requestDataset(mChildConfig.getDatasetId());
        }
    }
}
