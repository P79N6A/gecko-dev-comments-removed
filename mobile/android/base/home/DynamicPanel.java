




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.PanelLayout.ContextMenuRegistry;
import org.mozilla.gecko.home.PanelLayout.DatasetHandler;
import org.mozilla.gecko.home.PanelLayout.DatasetRequest;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;




















public class DynamicPanel extends HomeFragment {
    private static final String LOGTAG = "GeckoDynamicPanel";

    
    private static final String DATASET_REQUEST = "dataset_request";

    
    private static final int RESULT_LIMIT = 100;

    
    private FrameLayout mView;

    
    private PanelLayout mPanelLayout;

    
    private PanelAuthLayout mPanelAuthLayout;

    
    private PanelAuthCache mPanelAuthCache;

    
    
    private UiAsyncTask<Void, Void, Boolean> mAuthStateTask;

    
    private PanelConfig mPanelConfig;

    
    private PanelLoaderCallbacks mLoaderCallbacks;

    
    private UIMode mUIMode;

    





    private enum UIMode {
        PANEL,
        AUTH
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

        mPanelAuthCache = new PanelAuthCache(getActivity());
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mView = new FrameLayout(getActivity());
        return mView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        
        
        if (mUIMode != null) {
            setUIMode(mUIMode);
        }

        mPanelAuthCache.setOnChangeListener(new PanelAuthChangeListener());
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mView = null;
        mPanelLayout = null;
        mPanelAuthLayout = null;

        mPanelAuthCache.setOnChangeListener(null);

        if (mAuthStateTask != null) {
            mAuthStateTask.cancel(true);
            mAuthStateTask = null;
        }
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

        if (requiresAuth()) {
            mAuthStateTask = new UiAsyncTask<Void, Void, Boolean>(ThreadUtils.getBackgroundHandler()) {
                @Override
                public synchronized Boolean doInBackground(Void... params) {
                    return mPanelAuthCache.isAuthenticated(mPanelConfig.getId());
                }

                @Override
                public void onPostExecute(Boolean isAuthenticated) {
                    mAuthStateTask = null;
                    setUIMode(isAuthenticated ? UIMode.PANEL : UIMode.AUTH);
                }
            };
            mAuthStateTask.execute();
        } else {
            setUIMode(UIMode.PANEL);
        }
    }

    


    private boolean requiresAuth() {
        return mPanelConfig.getAuthConfig() != null;
    }

    


    private void createPanelLayout() {
        final ContextMenuRegistry contextMenuRegistry = new ContextMenuRegistry() {
            @Override
            public void register(View view) {
                registerForContextMenu(view);
            }
        };

        switch(mPanelConfig.getLayoutType()) {
            case FRAME:
                final PanelDatasetHandler datasetHandler = new PanelDatasetHandler();
                mPanelLayout = new FramePanelLayout(getActivity(), mPanelConfig, datasetHandler,
                        mUrlOpenListener, contextMenuRegistry);
                break;

            default:
                throw new IllegalStateException("Unrecognized layout type in DynamicPanel");
        }

        Log.d(LOGTAG, "Created layout of type: " + mPanelConfig.getLayoutType());
        mView.addView(mPanelLayout);
    }

    


    private void createPanelAuthLayout() {
        mPanelAuthLayout = new PanelAuthLayout(getActivity(), mPanelConfig);
        mView.addView(mPanelAuthLayout, 0);
    }

    private void setUIMode(UIMode mode) {
        switch(mode) {
            case PANEL:
                if (mPanelAuthLayout != null) {
                    mPanelAuthLayout.setVisibility(View.GONE);
                }
                if (mPanelLayout == null) {
                    createPanelLayout();
                }
                mPanelLayout.setVisibility(View.VISIBLE);

                
                
                
                
                
                if (mUIMode != mode && canLoad()) {
                    mPanelLayout.load();
                }
                break;

            case AUTH:
                if (mPanelLayout != null) {
                    mPanelLayout.setVisibility(View.GONE);
                }
                if (mPanelAuthLayout == null) {
                    createPanelAuthLayout();
                }
                mPanelAuthLayout.setVisibility(View.VISIBLE);
                break;

            default:
                throw new IllegalStateException("Unrecognized UIMode in DynamicPanel");
        }

        mUIMode = mode;
    }

    



    private class PanelDatasetHandler implements DatasetHandler {
        @Override
        public void requestDataset(DatasetRequest request) {
            Log.d(LOGTAG, "Requesting request: " + request);

            
            
            if (!getCanLoadHint()) {
                return;
            }

            final Bundle bundle = new Bundle();
            bundle.putParcelable(DATASET_REQUEST, request);

            getLoaderManager().restartLoader(request.getViewIndex(),
                                             bundle, mLoaderCallbacks);
        }

        @Override
        public void resetDataset(int viewIndex) {
            Log.d(LOGTAG, "Resetting dataset: " + viewIndex);

            final LoaderManager lm = getLoaderManager();

            
            
            final Loader<?> datasetLoader = lm.getLoader(viewIndex);
            if (datasetLoader != null) {
                datasetLoader.reset();
            }
        }
    }

    


    private static class PanelDatasetLoader extends SimpleCursorLoader {
        private DatasetRequest mRequest;

        public PanelDatasetLoader(Context context, DatasetRequest request) {
            super(context);
            mRequest = request;
        }

        public DatasetRequest getRequest() {
            return mRequest;
        }

        @Override
        public void onContentChanged() {
            
            
            
            final DatasetRequest newRequest =
                   new DatasetRequest(mRequest.getViewIndex(),
                                      DatasetRequest.Type.DATASET_LOAD,
                                      mRequest.getDatasetId(),
                                      mRequest.getFilterDetail());

            mRequest = newRequest;
            super.onContentChanged();
        }

        @Override
        public Cursor loadCursor() {
            final ContentResolver cr = getContext().getContentResolver();

            final String selection;
            final String[] selectionArgs;

            
            if (mRequest.getFilter() == null) {
                selection = HomeItems.FILTER + " IS NULL";
                selectionArgs = null;
            } else {
                selection = HomeItems.FILTER + " = ?";
                selectionArgs = new String[] { mRequest.getFilter() };
            }

            final Uri queryUri = HomeItems.CONTENT_URI.buildUpon()
                                                      .appendQueryParameter(BrowserContract.PARAM_DATASET_ID,
                                                                            mRequest.getDatasetId())
                                                      .appendQueryParameter(BrowserContract.PARAM_LIMIT,
                                                                            String.valueOf(RESULT_LIMIT))
                                                      .build();

            
            
            return cr.query(queryUri, null, selection, selectionArgs, null);
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

            if (mPanelLayout != null) {
                mPanelLayout.deliverDataset(request, cursor);
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            final DatasetRequest request = getRequestFromLoader(loader);
            Log.d(LOGTAG, "Resetting loader for request: " + request);

            if (mPanelLayout != null) {
                mPanelLayout.releaseDataset(request.getViewIndex());
            }
        }

        private DatasetRequest getRequestFromLoader(Loader<Cursor> loader) {
            final PanelDatasetLoader datasetLoader = (PanelDatasetLoader) loader;
            return datasetLoader.getRequest();
        }
    }

    private class PanelAuthChangeListener implements PanelAuthCache.OnChangeListener {
        @Override
        public void onChange(String panelId, boolean isAuthenticated) {
            if (!mPanelConfig.getId().equals(panelId)) {
                return;
            }

            setUIMode(isAuthenticated ? UIMode.PANEL : UIMode.AUTH);
        }
    }
}
