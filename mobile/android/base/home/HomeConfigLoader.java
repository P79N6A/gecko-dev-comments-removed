




package org.mozilla.gecko.home;

import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.OnChangeListener;

import android.content.Context;
import android.support.v4.content.AsyncTaskLoader;

import java.util.List;

public class HomeConfigLoader extends AsyncTaskLoader<HomeConfig.State> {
    private final HomeConfig mConfig;
    private HomeConfig.State mConfigState;

    public HomeConfigLoader(Context context, HomeConfig homeConfig) {
        super(context);
        mConfig = homeConfig;
    }

    @Override
    public HomeConfig.State loadInBackground() {
        return mConfig.load();
    }

    @Override
    public void deliverResult(HomeConfig.State configState) {
        if (isReset()) {
            mConfigState = null;
            return;
        }

        mConfigState = configState;
        mConfig.setOnChangeListener(new ForceLoadChangeListener());

        if (isStarted()) {
            super.deliverResult(configState);
        }
    }

    @Override
    protected void onStartLoading() {
        if (mConfigState != null) {
            deliverResult(mConfigState);
        }

        if (takeContentChanged() || mConfigState == null) {
            forceLoad();
        }
    }

    @Override
    protected void onStopLoading() {
        cancelLoad();
    }

    @Override
    public void onCanceled(HomeConfig.State configState) {
        mConfigState = null;
    }

    @Override
    protected void onReset() {
        super.onReset();

        
        onStopLoading();

        mConfigState = null;
        mConfig.setOnChangeListener(null);
    }

    private class ForceLoadChangeListener implements OnChangeListener {
        @Override
        public void onChange() {
            onContentChanged();
        }
    }
}
