




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.db.DBUtils;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PanelLayout.DatasetHandler;
import org.mozilla.gecko.home.PanelLayout.DatasetRequest;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;




















public class DynamicPanel extends HomeFragment {
    private static final String LOGTAG = "GeckoDynamicPanel";

    
    private static final String DATASET_REQUEST = "dataset_request";

    
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
                mLayout = new FramePanelLayout(getActivity(), mPanelConfig, datasetHandler, mUrlOpenListener);
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

    private void restartDatasetLoader(DatasetRequest request) {
        final Bundle bundle = new Bundle();
        bundle.putParcelable(DATASET_REQUEST, request);

        
        final int loaderId = generateLoaderId(request.getDatasetId());
        getLoaderManager().restartLoader(loaderId, bundle, mLoaderCallbacks);
    }

    



    private class PanelDatasetHandler implements DatasetHandler {
        @Override
        public void requestDataset(DatasetRequest request) {
            Log.d(LOGTAG, "Requesting request: " + request);

            
            
            if (!getCanLoadHint()) {
                return;
            }

            restartDatasetLoader(request);
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
        private final DatasetRequest mRequest;

        public PanelDatasetLoader(Context context, DatasetRequest request) {
            super(context);
            mRequest = request;
        }

        public DatasetRequest getRequest() {
            return mRequest;
        }

        @Override
        public Cursor loadCursor() {
            final ContentResolver cr = getContext().getContentResolver();

            final String selection;
            final String[] selectionArgs;

            
            if (mRequest.getFilter() == null) {
                selection = DBUtils.concatenateWhere(HomeItems.DATASET_ID + " = ?", HomeItems.FILTER + " IS NULL");
                selectionArgs = new String[] { mRequest.getDatasetId() };
            } else {
                selection = DBUtils.concatenateWhere(HomeItems.DATASET_ID + " = ?", HomeItems.FILTER + " = ?");
                selectionArgs = new String[] { mRequest.getDatasetId(), mRequest.getFilter() };
            }

            
            return cr.query(HomeItems.CONTENT_URI, null, selection, selectionArgs, null);
        }
    }

    


    private class PanelLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            final DatasetRequest request = (DatasetRequest) args.getParcelable(DATASET_REQUEST);

            Log.d(LOGTAG, "Creating loader for request: " + request);
            return new PanelDatasetLoader(getActivity(), request);
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
            final DatasetRequest request = getRequestFromLoader(loader);

            Log.d(LOGTAG, "Finished loader for request: " + request);
            mLayout.deliverDataset(request, cursor);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            final DatasetRequest request = getRequestFromLoader(loader);
            Log.d(LOGTAG, "Resetting loader for request: " + request);
            if (mLayout != null) {
                mLayout.releaseDataset(request.getDatasetId());
            }
        }

        private DatasetRequest getRequestFromLoader(Loader<Cursor> loader) {
            final PanelDatasetLoader datasetLoader = (PanelDatasetLoader) loader;
            return datasetLoader.getRequest();
        }
    }
}
