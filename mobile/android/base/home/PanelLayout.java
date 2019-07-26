




package org.mozilla.gecko.home;

import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;

import android.content.Context;
import android.database.Cursor;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;

import java.util.ArrayList;
import java.util.List;




































abstract class PanelLayout extends FrameLayout {
    private static final String LOGTAG = "GeckoPanelLayout";

    private final List<ViewEntry> mViewEntries;
    private final DatasetHandler mDatasetHandler;
    private final OnUrlOpenListener mUrlOpenListener;

    



    public interface DatasetBacked {
        public void setDataset(Cursor cursor);
    }

    



    public interface DatasetHandler {
        



        public void requestDataset(String datasetId);

        




        public void resetDataset(String datasetId);
    }

    public interface PanelView {
        public void setOnUrlOpenListener(OnUrlOpenListener listener);
    }

    public PanelLayout(Context context, PanelConfig panelConfig, DatasetHandler datasetHandler, OnUrlOpenListener urlOpenListener) {
        super(context);
        mViewEntries = new ArrayList<ViewEntry>();
        mDatasetHandler = datasetHandler;
        mUrlOpenListener = urlOpenListener;
    }

    




    public final void deliverDataset(String datasetId, Cursor cursor) {
        Log.d(LOGTAG, "Delivering dataset: " + datasetId);
        updateViewsWithDataset(datasetId, cursor);
    }

    



    public final void releaseDataset(String datasetId) {
        Log.d(LOGTAG, "Resetting dataset: " + datasetId);
        updateViewsWithDataset(datasetId, null);
    }

    



    protected final void requestDataset(String datasetId) {
        Log.d(LOGTAG, "Requesting dataset: " + datasetId);
        mDatasetHandler.requestDataset(datasetId);
    }

    



    protected final void resetDataset(String datasetId) {
        mDatasetHandler.resetDataset(datasetId);
    }

    





    protected final View createPanelView(ViewConfig viewConfig) {
        final View view;

        Log.d(LOGTAG, "Creating panel view: " + viewConfig.getType());

        switch(viewConfig.getType()) {
            case LIST:
                view = new PanelListView(getContext(), viewConfig);
                break;

            case GRID:
                view = new PanelGridView(getContext(), viewConfig);
                break;

            default:
                throw new IllegalStateException("Unrecognized view type in " + getClass().getSimpleName());
        }

        final ViewEntry entry = new ViewEntry(view, viewConfig);
        mViewEntries.add(entry);

        ((PanelView) view).setOnUrlOpenListener(mUrlOpenListener);

        return view;
    }

    



    protected final void disposePanelView(View view) {
        Log.d(LOGTAG, "Disposing panel view");

        final int count = mViewEntries.size();
        for (int i = 0; i < count; i++) {
            final View entryView = mViewEntries.get(i).getView();
            if (view == entryView) {
                
                
                maybeSetDataset(entryView, null);

                
                mViewEntries.remove(i);
                break;
            }
        }
    }

    private void updateViewsWithDataset(String datasetId, Cursor cursor) {
        final int count = mViewEntries.size();
        for (int i = 0; i < count; i++) {
            final ViewEntry entry = mViewEntries.get(i);

            
            if (TextUtils.equals(entry.getDatasetId(), datasetId)) {
                final View view = entry.getView();
                maybeSetDataset(view, cursor);
            }
        }
    }

    private void maybeSetDataset(View view, Cursor cursor) {
        if (view instanceof DatasetBacked) {
            final DatasetBacked dsb = (DatasetBacked) view;
            dsb.setDataset(cursor);
        }
    }

    




    public abstract void load();

    



    private static class ViewEntry {
        private final View mView;
        private final ViewConfig mViewConfig;

        public ViewEntry(View view, ViewConfig viewConfig) {
            mView = view;
            mViewConfig = viewConfig;
        }

        public View getView() {
            return mView;
        }

        public String getDatasetId() {
            return mViewConfig.getDatasetId();
        }
    }
}
