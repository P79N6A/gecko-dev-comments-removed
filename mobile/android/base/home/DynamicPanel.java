




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.PanelLayout.DatasetHandler;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import java.util.EnumSet;




















public class DynamicPanel extends HomeFragment {
    private static final String LOGTAG = "GeckoDynamicPanel";

    
    private static final String DATASET_ID = "dataset_id";

    
    private PanelLayout mLayout;

    
    private PanelConfig mPanelConfig;

    
    private PanelLoaderCallbacks mLoaderCallbacks;

    
    private OnUrlOpenListener mUrlOpenListener;

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUrlOpenListener = null;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Bundle args = getArguments();
        if (args != null) {
            mPanelConfig = (PanelConfig) args.getParcelable(HomePager.PANEL_CONFIG_ARG);
        }

        if (mPanelConfig == null) {
            throw new IllegalStateException("Can't create a DynamicPanel without a PanelConfig");
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        switch(mPanelConfig.getLayoutType()) {
            case FRAME:
                final PanelDatasetHandler datasetHandler = new PanelDatasetHandler();
                mLayout = new FramePanelLayout(getActivity(), mPanelConfig, datasetHandler);
                break;

            default:
                throw new IllegalStateException("Unrecognized layout type in DynamicPanel");
        }

        Log.d(LOGTAG, "Created layout of type: " + mPanelConfig.getLayoutType());

        return mLayout;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mLayout = null;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mLoaderCallbacks = new PanelLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    protected void load() {
        Log.d(LOGTAG, "Loading layout");
        mLayout.load();
    }

    private static int generateLoaderId(String datasetId) {
        return datasetId.hashCode();
    }

    



    private class PanelDatasetHandler implements DatasetHandler {
        @Override
        public void requestDataset(String datasetId) {
            Log.d(LOGTAG, "Requesting dataset: " + datasetId);

            
            
            if (!getCanLoadHint()) {
                return;
            }

            final Bundle bundle = new Bundle();
            bundle.putString(DATASET_ID, datasetId);

            
            final int loaderId = generateLoaderId(datasetId);
            getLoaderManager().restartLoader(loaderId, bundle, mLoaderCallbacks);
        }

        @Override
        public void resetDataset(String datasetId) {
            Log.d(LOGTAG, "Resetting dataset: " + datasetId);

            final LoaderManager lm = getLoaderManager();
            final int loaderId = generateLoaderId(datasetId);

            
            
            final Loader<?> datasetLoader = lm.getLoader(loaderId);
            if (datasetLoader != null) {
                datasetLoader.reset();
            }
        }
    }

    


    private static class PanelDatasetLoader extends SimpleCursorLoader {
        private final String mDatasetId;

        public PanelDatasetLoader(Context context, String datasetId) {
            super(context);
            mDatasetId = datasetId;
        }

        public String getDatasetId() {
            return mDatasetId;
        }

        @Override
        public Cursor loadCursor() {
            final ContentResolver cr = getContext().getContentResolver();

            
            final Uri fakeItemsUri = HomeItems.CONTENT_FAKE_URI.buildUpon().
                appendQueryParameter(BrowserContract.PARAM_PROFILE, "default").build();

            final String selection = HomeItems.DATASET_ID + " = ?";
            final String[] selectionArgs = new String[] { mDatasetId };

            Log.i(LOGTAG, "Loading fake data for list provider: " + mDatasetId);

            return cr.query(fakeItemsUri, null, selection, selectionArgs, null);
        }
    }

    


    private class PanelLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            final String datasetId = args.getString(DATASET_ID);

            Log.d(LOGTAG, "Creating loader for dataset: " + datasetId);
            return new PanelDatasetLoader(getActivity(), datasetId);
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
            final PanelDatasetLoader datasetLoader = (PanelDatasetLoader) loader;

            Log.d(LOGTAG, "Finished loader for dataset: " + datasetLoader.getDatasetId());
            mLayout.deliverDataset(datasetLoader.getDatasetId(), cursor);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            final PanelDatasetLoader datasetLoader = (PanelDatasetLoader) loader;
            Log.d(LOGTAG, "Resetting loader for dataset: " + datasetLoader.getDatasetId());
            if (mLayout != null) {
                mLayout.releaseDataset(datasetLoader.getDatasetId());
            }
        }
    }
}
