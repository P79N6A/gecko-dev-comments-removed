




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.HomeConfig.EmptyViewConfig;
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
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.lang.ref.SoftReference;
import java.util.EnumSet;
import java.util.LinkedList;
import java.util.Map;
import java.util.WeakHashMap;

import com.squareup.picasso.Picasso;




































abstract class PanelLayout extends FrameLayout {
    private static final String LOGTAG = "GeckoPanelLayout";

    protected final SparseArray<ViewState> mViewStates;
    private final PanelConfig mPanelConfig;
    private final DatasetHandler mDatasetHandler;
    private final OnUrlOpenListener mUrlOpenListener;
    private final ContextMenuRegistry mContextMenuRegistry;

    



    public interface DatasetBacked {
        public void setDataset(Cursor cursor);
        public void setFilterManager(FilterManager manager);
    }

    



    public static class DatasetRequest implements Parcelable {
        public enum Type implements Parcelable {
            DATASET_LOAD,
            FILTER_PUSH,
            FILTER_POP;

            @Override
            public int describeContents() {
                return 0;
            }

            @Override
            public void writeToParcel(Parcel dest, int flags) {
                dest.writeInt(ordinal());
            }

            public static final Creator<Type> CREATOR = new Creator<Type>() {
                @Override
                public Type createFromParcel(final Parcel source) {
                    return Type.values()[source.readInt()];
                }

                @Override
                public Type[] newArray(final int size) {
                    return new Type[size];
                }
            };
        }

        private final int mViewIndex;
        private final Type mType;
        private final String mDatasetId;
        private final FilterDetail mFilterDetail;

        private DatasetRequest(Parcel in) {
            this.mViewIndex = in.readInt();
            this.mType = (Type) in.readParcelable(getClass().getClassLoader());
            this.mDatasetId = in.readString();
            this.mFilterDetail = (FilterDetail) in.readParcelable(getClass().getClassLoader());
        }

        public DatasetRequest(int index, String datasetId, FilterDetail filterDetail) {
            this(index, Type.DATASET_LOAD, datasetId, filterDetail);
        }

        public DatasetRequest(int index, Type type, String datasetId, FilterDetail filterDetail) {
            this.mViewIndex = index;
            this.mType = type;
            this.mDatasetId = datasetId;
            this.mFilterDetail = filterDetail;
        }

        public int getViewIndex() {
            return mViewIndex;
        }

        public Type getType() {
            return mType;
        }

        public String getDatasetId() {
            return mDatasetId;
        }

        public String getFilter() {
            return (mFilterDetail != null ? mFilterDetail.filter : null);
        }

        public FilterDetail getFilterDetail() {
            return mFilterDetail;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(mViewIndex);
            dest.writeParcelable(mType, 0);
            dest.writeString(mDatasetId);
            dest.writeParcelable(mFilterDetail, 0);
        }

        public String toString() {
            return "{ index: " + mViewIndex +
                   ", type: " + mType +
                   ", dataset: " + mDatasetId +
                   ", filter: " + mFilterDetail +
                   " }";
        }

        public static final Creator<DatasetRequest> CREATOR = new Creator<DatasetRequest>() {
            @Override
            public DatasetRequest createFromParcel(Parcel in) {
                return new DatasetRequest(in);
            }

            @Override
            public DatasetRequest[] newArray(int size) {
                return new DatasetRequest[size];
            }
        };
    }

    



    public interface DatasetHandler {
        



        public void requestDataset(DatasetRequest request);

        




        public void resetDataset(int viewIndex);
    }

    public interface PanelView {
        public void setOnItemOpenListener(OnItemOpenListener listener);
        public void setOnKeyListener(OnKeyListener listener);
        public void setContextMenuInfoFactory(HomeContextMenuInfo.Factory factory);
    }

    public interface FilterManager {
        public FilterDetail getPreviousFilter();
        public boolean canGoBack();
        public void goBack();
    }

    public interface ContextMenuRegistry {
        public void register(View view);
    }

    public PanelLayout(Context context, PanelConfig panelConfig, DatasetHandler datasetHandler,
            OnUrlOpenListener urlOpenListener, ContextMenuRegistry contextMenuRegistry) {
        super(context);
        mViewStates = new SparseArray<ViewState>();
        mPanelConfig = panelConfig;
        mDatasetHandler = datasetHandler;
        mUrlOpenListener = urlOpenListener;
        mContextMenuRegistry = contextMenuRegistry;
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        final int count = mViewStates.size();
        for (int i = 0; i < count; i++) {
            final ViewState viewState = mViewStates.valueAt(i);

            final View view = viewState.getView();
            if (view != null) {
                maybeSetDataset(view, null);
            }
        }
        mViewStates.clear();
    }

    




    public final void deliverDataset(DatasetRequest request, Cursor cursor) {
        Log.d(LOGTAG, "Delivering request: " + request);
        final ViewState viewState = mViewStates.get(request.getViewIndex());
        if (viewState == null) {
            return;
        }

        switch (request.getType()) {
            case FILTER_PUSH:
                viewState.pushFilter(request.getFilterDetail());
                break;
            case FILTER_POP:
                viewState.popFilter();
                break;
        }

        final View activeView = viewState.getActiveView();
        if (activeView == null) {
            throw new IllegalStateException("No active view for view state: " + viewState.getIndex());
        }

        final ViewConfig viewConfig = viewState.getViewConfig();

        final View newView;
        if (cursor == null || cursor.getCount() == 0) {
            newView = createEmptyView(viewConfig);
            maybeSetDataset(activeView, null);
        } else {
            newView = createPanelView(viewConfig);
            maybeSetDataset(newView, cursor);
        }

        if (activeView != newView) {
            replacePanelView(activeView, newView);
        }
    }

    



    public final void releaseDataset(int viewIndex) {
        Log.d(LOGTAG, "Releasing dataset: " + viewIndex);
        final ViewState viewState = mViewStates.get(viewIndex);
        if (viewState == null) {
            return;
        }

        final View view = viewState.getView();
        if (view != null) {
            maybeSetDataset(view, null);
        }
    }

    



    protected final void requestDataset(DatasetRequest request) {
        Log.d(LOGTAG, "Requesting request: " + request);
        if (mViewStates.get(request.getViewIndex()) == null) {
            return;
        }

        mDatasetHandler.requestDataset(request);
    }

    



    protected final void resetDataset(int viewIndex) {
        Log.d(LOGTAG, "Resetting view with index: " + viewIndex);
        if (mViewStates.get(viewIndex) == null) {
            return;
        }

        mDatasetHandler.resetDataset(viewIndex);
    }

    





    protected final View createPanelView(ViewConfig viewConfig) {
        Log.d(LOGTAG, "Creating panel view: " + viewConfig.getType());

        ViewState viewState = mViewStates.get(viewConfig.getIndex());
        if (viewState == null) {
            viewState = new ViewState(viewConfig);
            mViewStates.put(viewConfig.getIndex(), viewState);
        }

        View view = viewState.getView();
        if (view == null) {
            switch(viewConfig.getType()) {
                case LIST:
                    view = new PanelListView(getContext(), viewConfig);
                    break;

                case GRID:
                    view = new PanelRecyclerView(getContext(), viewConfig);
                    break;

                default:
                    throw new IllegalStateException("Unrecognized view type in " + getClass().getSimpleName());
            }

            PanelView panelView = (PanelView) view;
            panelView.setOnItemOpenListener(new PanelOnItemOpenListener(viewState));
            panelView.setOnKeyListener(new PanelKeyListener(viewState));
            panelView.setContextMenuInfoFactory(new HomeContextMenuInfo.Factory() {
                @Override
                public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                    final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                    info.url = cursor.getString(cursor.getColumnIndexOrThrow(HomeItems.URL));
                    info.title = cursor.getString(cursor.getColumnIndexOrThrow(HomeItems.TITLE));
                    return info;
                }
            });

            mContextMenuRegistry.register(view);

            if (view instanceof DatasetBacked) {
                DatasetBacked datasetBacked = (DatasetBacked) view;
                datasetBacked.setFilterManager(new PanelFilterManager(viewState));

                if (viewConfig.isRefreshEnabled()) {
                    view = new PanelRefreshLayout(getContext(), view,
                                                  mPanelConfig.getId(), viewConfig.getIndex());
                }
            }

            viewState.setView(view);
        }

        return view;
    }

    



    protected final void disposePanelView(View view) {
        Log.d(LOGTAG, "Disposing panel view");
        final int count = mViewStates.size();
        for (int i = 0; i < count; i++) {
            final ViewState viewState = mViewStates.valueAt(i);

            if (viewState.getView() == view) {
                maybeSetDataset(view, null);
                mViewStates.remove(viewState.getIndex());
                break;
            }
        }
    }

    private void maybeSetDataset(View view, Cursor cursor) {
        if (view instanceof DatasetBacked) {
            final DatasetBacked dsb = (DatasetBacked) view;
            dsb.setDataset(cursor);
        }
    }

    private View createEmptyView(ViewConfig viewConfig) {
        Log.d(LOGTAG, "Creating empty view: " + viewConfig.getType());

        ViewState viewState = mViewStates.get(viewConfig.getIndex());
        if (viewState == null) {
            throw new IllegalStateException("No view state found for view index: " + viewConfig.getIndex());
        }

        View view = viewState.getEmptyView();
        if (view == null) {
            view = LayoutInflater.from(getContext()).inflate(R.layout.home_empty_panel, null);

            final EmptyViewConfig emptyViewConfig = viewConfig.getEmptyViewConfig();

            
            final String text = (emptyViewConfig == null) ? null : emptyViewConfig.getText();
            final TextView textView = (TextView) view.findViewById(R.id.home_empty_text);
            if (TextUtils.isEmpty(text)) {
                textView.setText(R.string.home_default_empty);
            } else {
                textView.setText(text);
            }

            final String imageUrl = (emptyViewConfig == null) ? null : emptyViewConfig.getImageUrl();
            final ImageView imageView = (ImageView) view.findViewById(R.id.home_empty_image);

            if (TextUtils.isEmpty(imageUrl)) {
                imageView.setImageResource(R.drawable.icon_home_empty_firefox);
            } else {
                ImageLoader.with(getContext())
                           .load(imageUrl)
                           .error(R.drawable.icon_home_empty_firefox)
                           .into(imageView);
            }

            viewState.setEmptyView(view);
        }

        return view;
    }

    private void replacePanelView(View currentView, View newView) {
        final ViewGroup parent = (ViewGroup) currentView.getParent();
        parent.addView(newView, parent.indexOfChild(currentView), currentView.getLayoutParams());
        parent.removeView(currentView);
    }

    




    public abstract void load();

    



    protected class ViewState {
        private final ViewConfig mViewConfig;
        private SoftReference<View> mView;
        private SoftReference<View> mEmptyView;
        private LinkedList<FilterDetail> mFilterStack;

        public ViewState(ViewConfig viewConfig) {
            mViewConfig = viewConfig;
            mView = new SoftReference<View>(null);
            mEmptyView = new SoftReference<View>(null);
        }

        public ViewConfig getViewConfig() {
            return mViewConfig;
        }

        public int getIndex() {
            return mViewConfig.getIndex();
        }

        public View getView() {
            return mView.get();
        }

        public void setView(View view) {
            mView = new SoftReference<View>(view);
        }

        public View getEmptyView() {
            return mEmptyView.get();
        }

        public void setEmptyView(View view) {
            mEmptyView = new SoftReference<View>(view);
        }

        public View getActiveView() {
            final View view = getView();
            if (view != null && view.getParent() != null) {
                return view;
            }

            final View emptyView = getEmptyView();
            if (emptyView != null && emptyView.getParent() != null) {
                return emptyView;
            }

            return null;
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

                
                mFilterStack.push(new FilterDetail(mViewConfig.getFilter(),
                                                   mPanelConfig.getTitle()));
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

    static class FilterDetail implements Parcelable {
        final String filter;
        final String title;

        private FilterDetail(Parcel in) {
            this.filter = in.readString();
            this.title = in.readString();
        }

        public FilterDetail(String filter, String title) {
            this.filter = filter;
            this.title = title;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(filter);
            dest.writeString(title);
        }

        public static final Creator<FilterDetail> CREATOR = new Creator<FilterDetail>() {
            @Override
            public FilterDetail createFromParcel(Parcel in) {
                return new FilterDetail(in);
            }

            @Override
            public FilterDetail[] newArray(int size) {
                return new FilterDetail[size];
            }
        };
    }

    


    private void pushFilterOnView(ViewState viewState, FilterDetail filterDetail) {
        final int index = viewState.getIndex();
        final String datasetId = viewState.getDatasetId();

        mDatasetHandler.requestDataset(new DatasetRequest(index,
                                                          DatasetRequest.Type.FILTER_PUSH,
                                                          datasetId,
                                                          filterDetail));
    }

    




    private boolean popFilterOnView(ViewState viewState) {
        if (viewState.canPopFilter()) {
            final int index = viewState.getIndex();
            final String datasetId = viewState.getDatasetId();
            final FilterDetail filterDetail = viewState.getPreviousFilter();

            mDatasetHandler.requestDataset(new DatasetRequest(index,
                                                              DatasetRequest.Type.FILTER_POP,
                                                              datasetId,
                                                              filterDetail));

            return true;
        } else {
            return false;
        }
    }

    public interface OnItemOpenListener {
        public void onItemOpen(String url, String title);
    }

    private class PanelOnItemOpenListener implements OnItemOpenListener {
        private final ViewState mViewState;

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
        private final ViewState mViewState;

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
