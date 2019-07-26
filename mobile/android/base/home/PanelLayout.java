




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.HomeConfig.ItemHandler;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;
import org.mozilla.gecko.util.StringUtils;

import android.content.Context;
import android.database.Cursor;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.FrameLayout;

import java.util.EnumSet;
import java.util.LinkedList;
import java.util.Map;
import java.util.WeakHashMap;




































abstract class PanelLayout extends FrameLayout {
    private static final String LOGTAG = "GeckoPanelLayout";

    protected final Map<View, ViewState> mViewStateMap;
    private final PanelConfig mPanelConfig;
    private final DatasetHandler mDatasetHandler;
    private final OnUrlOpenListener mUrlOpenListener;

    



    public interface DatasetBacked {
        public void setDataset(Cursor cursor);
        public void setFilterManager(FilterManager manager);
    }

    



    public static class DatasetRequest implements Parcelable {
        private final String datasetId;
        private final String filter;

        private DatasetRequest(Parcel in) {
            this.datasetId = in.readString();
            this.filter = in.readString();
        }

        public DatasetRequest(String datasetId, String filter) {
            this.datasetId = datasetId;
            this.filter = filter;
        }

        public String getDatasetId() {
            return datasetId;
        }

        public String getFilter() {
            return filter;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(datasetId);
            dest.writeString(filter);
        }

        public String toString() {
            return "{dataset: " + datasetId + ", filter: " + filter + "}";
        }

        public static final Creator<DatasetRequest> CREATOR = new Creator<DatasetRequest>() {
            public DatasetRequest createFromParcel(Parcel in) {
                return new DatasetRequest(in);
            }

            public DatasetRequest[] newArray(int size) {
                return new DatasetRequest[size];
            }
        };
    }

    



    public interface DatasetHandler {
        



        public void requestDataset(DatasetRequest request);

        




        public void resetDataset(String datasetId);
    }

    public interface PanelView {
        public void setOnItemOpenListener(OnItemOpenListener listener);
        public void setOnKeyListener(OnKeyListener listener);
    }

    public interface FilterManager {
        public FilterDetail getPreviousFilter();
        public boolean canGoBack();
        public void goBack();
    }

    public PanelLayout(Context context, PanelConfig panelConfig, DatasetHandler datasetHandler, OnUrlOpenListener urlOpenListener) {
        super(context);
        mViewStateMap = new WeakHashMap<View, ViewState>();
        mPanelConfig = panelConfig;
        mDatasetHandler = datasetHandler;
        mUrlOpenListener = urlOpenListener;
    }

    




    public final void deliverDataset(DatasetRequest request, Cursor cursor) {
        Log.d(LOGTAG, "Delivering request: " + request);
        updateViewsWithDataset(request.getDatasetId(), cursor);
    }

    



    public final void releaseDataset(String datasetId) {
        Log.d(LOGTAG, "Resetting dataset: " + datasetId);
        updateViewsWithDataset(datasetId, null);
    }

    



    protected final void requestDataset(DatasetRequest request) {
        Log.d(LOGTAG, "Requesting request: " + request);
        mDatasetHandler.requestDataset(request);
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

        final ViewState state = new ViewState(viewConfig);
        
        mViewStateMap.put(view, state);

        PanelView panelView = (PanelView) view;
        panelView.setOnItemOpenListener(new PanelOnItemOpenListener(state));
        panelView.setOnKeyListener(new PanelKeyListener(state));

        if (view instanceof DatasetBacked) {
            DatasetBacked datasetBacked = (DatasetBacked) view;
            datasetBacked.setFilterManager(new PanelFilterManager(state));
        }

        return view;
    }

    



    protected final void disposePanelView(View view) {
        Log.d(LOGTAG, "Disposing panel view");
        if (mViewStateMap.containsKey(view)) {
            
            
            maybeSetDataset(view, null);

            
            mViewStateMap.remove(view);
        }
    }

    private void updateViewsWithDataset(String datasetId, Cursor cursor) {
        for (Map.Entry<View, ViewState> entry : mViewStateMap.entrySet()) {
            final ViewState detail = entry.getValue();

            
            if (TextUtils.equals(detail.getDatasetId(), datasetId)) {
                final View view = entry.getKey();
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

    



    protected class ViewState {
        private final ViewConfig mViewConfig;
        private LinkedList<FilterDetail> mFilterStack;

        public ViewState(ViewConfig viewConfig) {
            mViewConfig = viewConfig;
        }

        public String getDatasetId() {
            return mViewConfig.getDatasetId();
        }

        public ItemHandler getItemHandler() {
            return mViewConfig.getItemHandler();
        }

        


        public FilterDetail getCurrentFilter() {
            if (mFilterStack == null) {
                return null;
            } else {
                return mFilterStack.peek();
            }
        }

        


        public FilterDetail getPreviousFilter() {
            if (!canPopFilter()) {
                return null;
            }

            return mFilterStack.get(1);
        }

        


        public void pushFilter(FilterDetail filter) {
            if (mFilterStack == null) {
                mFilterStack = new LinkedList<FilterDetail>();

                
                
                mFilterStack.push(new FilterDetail(null, mPanelConfig.getTitle()));
            }

            mFilterStack.push(filter);
        }

        




        public boolean popFilter() {
            if (!canPopFilter()) {
                return false;
            }

            mFilterStack.pop();
            return true;
        }

        public boolean canPopFilter() {
            return (mFilterStack != null && mFilterStack.size() > 1);
        }
    }

    static class FilterDetail {
        final String filter;
        final String title;

        public FilterDetail(String filter, String title) {
            this.filter = filter;
            this.title = title;
        }
    }

    


    private void pushFilterOnView(ViewState viewState, FilterDetail filterDetail) {
        viewState.pushFilter(filterDetail);

        final String filter = filterDetail.filter;
        final String datasetId = viewState.getDatasetId();
        mDatasetHandler.requestDataset(new DatasetRequest(datasetId, filter));
    }

    




    private boolean popFilterOnView(ViewState viewState) {
        if (viewState.popFilter()) {
            final FilterDetail current = viewState.getCurrentFilter();

            final String filter = (current == null ? null : current.filter);
            final String datasetId = viewState.getDatasetId();
            mDatasetHandler.requestDataset(new DatasetRequest(datasetId, filter));
            return true;
        } else {
            return false;
        }
    }

    public interface OnItemOpenListener {
        public void onItemOpen(String url, String title);
    }

    private class PanelOnItemOpenListener implements OnItemOpenListener {
        private ViewState mViewState;

        public PanelOnItemOpenListener(ViewState viewState) {
            mViewState = viewState;
        }

        @Override
        public void onItemOpen(String url, String title) {
            if (StringUtils.isFilterUrl(url)) {
                FilterDetail filterDetail = new FilterDetail(StringUtils.getFilterFromUrl(url), title);
                pushFilterOnView(mViewState, filterDetail);
            } else {
                EnumSet<OnUrlOpenListener.Flags> flags = EnumSet.noneOf(OnUrlOpenListener.Flags.class);
                if (mViewState.getItemHandler() == ItemHandler.INTENT) {
                    flags.add(OnUrlOpenListener.Flags.OPEN_WITH_INTENT);
                }

                mUrlOpenListener.onUrlOpen(url, flags);
            }
        }
    }

    private class PanelKeyListener implements View.OnKeyListener {
        private ViewState mViewState;

        public PanelKeyListener(ViewState viewState) {
            mViewState = viewState;
        }

        @Override
        public boolean onKey(View v, int keyCode, KeyEvent event) {
            if (event.getAction() == KeyEvent.ACTION_UP && keyCode == KeyEvent.KEYCODE_BACK) {
                return popFilterOnView(mViewState);
            }

            return false;
        }
    }

    private class PanelFilterManager implements FilterManager {
        private final ViewState mViewState;

        public PanelFilterManager(ViewState viewState) {
            mViewState = viewState;
        }

        @Override
        public FilterDetail getPreviousFilter() {
            return mViewState.getPreviousFilter();
        }

        @Override
        public boolean canGoBack() {
            return mViewState.canPopFilter();
        }

        @Override
        public void goBack() {
            popFilterOnView(mViewState);
        }
    }
}
