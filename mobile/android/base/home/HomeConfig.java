




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

public final class HomeConfig {
    





    public static enum PanelType implements Parcelable {
        TOP_SITES("top_sites", TopSitesPanel.class),
        BOOKMARKS("bookmarks", BookmarksPanel.class),
        HISTORY("history", HistoryPanel.class),
        READING_LIST("reading_list", ReadingListPanel.class),
        DYNAMIC("dynamic", DynamicPanel.class);

        private final String mId;
        private final Class<?> mPanelClass;

        PanelType(String id, Class<?> panelClass) {
            mId = id;
            mPanelClass = panelClass;
        }

        public static PanelType fromId(String id) {
            if (id == null) {
                throw new IllegalArgumentException("Could not convert null String to PanelType");
            }

            for (PanelType panelType : PanelType.values()) {
                if (TextUtils.equals(panelType.mId, id.toLowerCase())) {
                    return panelType;
                }
            }

            throw new IllegalArgumentException("Could not convert String id to PanelType");
        }

        @Override
        public String toString() {
            return mId;
        }

        public Class<?> getPanelClass() {
            return mPanelClass;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<PanelType> CREATOR = new Creator<PanelType>() {
            @Override
            public PanelType createFromParcel(final Parcel source) {
                return PanelType.values()[source.readInt()];
            }

            @Override
            public PanelType[] newArray(final int size) {
                return new PanelType[size];
            }
        };
    }

    public static class PanelConfig implements Parcelable {
        private final PanelType mType;
        private final String mTitle;
        private final String mId;
        private final LayoutType mLayoutType;
        private final List<ViewConfig> mViews;
        private final EnumSet<Flags> mFlags;

        private static final String JSON_KEY_TYPE = "type";
        private static final String JSON_KEY_TITLE = "title";
        private static final String JSON_KEY_ID = "id";
        private static final String JSON_KEY_LAYOUT = "layout";
        private static final String JSON_KEY_VIEWS = "views";
        private static final String JSON_KEY_DEFAULT = "default";
        private static final String JSON_KEY_DISABLED = "disabled";

        public enum Flags {
            DEFAULT_PANEL,
            DISABLED_PANEL
        }

        public PanelConfig(JSONObject json) throws JSONException, IllegalArgumentException {
            final String panelType = json.optString(JSON_KEY_TYPE, null);
            if (TextUtils.isEmpty(panelType)) {
                mType = PanelType.DYNAMIC;
            } else {
                mType = PanelType.fromId(panelType);
            }

            mTitle = json.getString(JSON_KEY_TITLE);
            mId = json.getString(JSON_KEY_ID);

            final String layoutTypeId = json.optString(JSON_KEY_LAYOUT, null);
            if (layoutTypeId != null) {
                mLayoutType = LayoutType.fromId(layoutTypeId);
            } else {
                mLayoutType = null;
            }

            final JSONArray jsonViews = json.optJSONArray(JSON_KEY_VIEWS);
            if (jsonViews != null) {
                mViews = new ArrayList<ViewConfig>();

                final int viewCount = jsonViews.length();
                for (int i = 0; i < viewCount; i++) {
                    final JSONObject jsonViewConfig = (JSONObject) jsonViews.get(i);
                    final ViewConfig viewConfig = new ViewConfig(jsonViewConfig);
                    mViews.add(viewConfig);
                }
            } else {
                mViews = null;
            }

            mFlags = EnumSet.noneOf(Flags.class);

            if (json.optBoolean(JSON_KEY_DEFAULT, false)) {
                mFlags.add(Flags.DEFAULT_PANEL);
            }

            if (json.optBoolean(JSON_KEY_DISABLED, false)) {
                mFlags.add(Flags.DISABLED_PANEL);
            }

            validate();
        }

        @SuppressWarnings("unchecked")
        public PanelConfig(Parcel in) {
            mType = (PanelType) in.readParcelable(getClass().getClassLoader());
            mTitle = in.readString();
            mId = in.readString();
            mLayoutType = (LayoutType) in.readParcelable(getClass().getClassLoader());

            mViews = new ArrayList<ViewConfig>();
            in.readTypedList(mViews, ViewConfig.CREATOR);

            mFlags = (EnumSet<Flags>) in.readSerializable();

            validate();
        }

        public PanelConfig(PanelConfig panelConfig) {
            mType = panelConfig.mType;
            mTitle = panelConfig.mTitle;
            mId = panelConfig.mId;
            mLayoutType = panelConfig.mLayoutType;

            mViews = new ArrayList<ViewConfig>();
            List<ViewConfig> viewConfigs = panelConfig.mViews;
            if (viewConfigs != null) {
                for (ViewConfig viewConfig : viewConfigs) {
                    mViews.add(new ViewConfig(viewConfig));
                }
            }
            mFlags = panelConfig.mFlags.clone();

            validate();
        }

        public PanelConfig(PanelType type, String title, String id) {
            this(type, title, id, EnumSet.noneOf(Flags.class));
        }

        public PanelConfig(PanelType type, String title, String id, EnumSet<Flags> flags) {
            this(type, title, id, null, null, flags);
        }

        public PanelConfig(PanelType type, String title, String id, LayoutType layoutType,
                List<ViewConfig> views, EnumSet<Flags> flags) {
            mType = type;
            mTitle = title;
            mId = id;
            mLayoutType = layoutType;
            mViews = views;
            mFlags = flags;

            validate();
        }

        private void validate() {
            if (mType == null) {
                throw new IllegalArgumentException("Can't create PanelConfig with null type");
            }

            if (TextUtils.isEmpty(mTitle)) {
                throw new IllegalArgumentException("Can't create PanelConfig with empty title");
            }

            if (TextUtils.isEmpty(mId)) {
                throw new IllegalArgumentException("Can't create PanelConfig with empty id");
            }

            if (mLayoutType == null && mType == PanelType.DYNAMIC) {
                throw new IllegalArgumentException("Can't create a dynamic PanelConfig with null layout type");
            }

            if ((mViews == null || mViews.size() == 0) && mType == PanelType.DYNAMIC) {
                throw new IllegalArgumentException("Can't create a dynamic PanelConfig with no views");
            }

            if (mFlags == null) {
                throw new IllegalArgumentException("Can't create PanelConfig with null flags");
            }
        }

        public PanelType getType() {
            return mType;
        }

        public String getTitle() {
            return mTitle;
        }

        public String getId() {
            return mId;
        }

        public LayoutType getLayoutType() {
            return mLayoutType;
        }

        public int getViewCount() {
            return (mViews != null ? mViews.size() : 0);
        }

        public ViewConfig getViewAt(int index) {
            return (mViews != null ? mViews.get(index) : null);
        }

        public boolean isDynamic() {
            return (mType == PanelType.DYNAMIC);
        }

        public boolean isDefault() {
            return mFlags.contains(Flags.DEFAULT_PANEL);
        }

        public void setIsDefault(boolean isDefault) {
            if (isDefault) {
                mFlags.add(Flags.DEFAULT_PANEL);
            } else {
                mFlags.remove(Flags.DEFAULT_PANEL);
            }
        }

        public boolean isDisabled() {
            return mFlags.contains(Flags.DISABLED_PANEL);
        }

        public void setIsDisabled(boolean isDisabled) {
            if (isDisabled) {
                mFlags.add(Flags.DISABLED_PANEL);
            } else {
                mFlags.remove(Flags.DISABLED_PANEL);
            }
        }

        public JSONObject toJSON() throws JSONException {
            final JSONObject json = new JSONObject();

            json.put(JSON_KEY_TYPE, mType.toString());
            json.put(JSON_KEY_TITLE, mTitle);
            json.put(JSON_KEY_ID, mId);

            if (mLayoutType != null) {
                json.put(JSON_KEY_LAYOUT, mLayoutType.toString());
            }

            if (mViews != null) {
                final JSONArray jsonViews = new JSONArray();

                final int viewCount = mViews.size();
                for (int i = 0; i < viewCount; i++) {
                    final ViewConfig viewConfig = mViews.get(i);
                    final JSONObject jsonViewConfig = viewConfig.toJSON();
                    jsonViews.put(jsonViewConfig);
                }

                json.put(JSON_KEY_VIEWS, jsonViews);
            }

            if (mFlags.contains(Flags.DEFAULT_PANEL)) {
                json.put(JSON_KEY_DEFAULT, true);
            }

            if (mFlags.contains(Flags.DISABLED_PANEL)) {
                json.put(JSON_KEY_DISABLED, true);
            }

            return json;
        }

        @Override
        public boolean equals(Object o) {
            if (o == null) {
                return false;
            }

            if (this == o) {
                return true;
            }

            if (!(o instanceof PanelConfig)) {
                return false;
            }

            final PanelConfig other = (PanelConfig) o;
            return mId.equals(other.mId);
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeParcelable(mType, 0);
            dest.writeString(mTitle);
            dest.writeString(mId);
            dest.writeParcelable(mLayoutType, 0);
            dest.writeTypedList(mViews);
            dest.writeSerializable(mFlags);
        }

        public static final Creator<PanelConfig> CREATOR = new Creator<PanelConfig>() {
            @Override
            public PanelConfig createFromParcel(final Parcel in) {
                return new PanelConfig(in);
            }

            @Override
            public PanelConfig[] newArray(final int size) {
                return new PanelConfig[size];
            }
        };
    }

    public static enum LayoutType implements Parcelable {
        FRAME("frame");

        private final String mId;

        LayoutType(String id) {
            mId = id;
        }

        public static LayoutType fromId(String id) {
            if (id == null) {
                throw new IllegalArgumentException("Could not convert null String to LayoutType");
            }

            for (LayoutType layoutType : LayoutType.values()) {
                if (TextUtils.equals(layoutType.mId, id.toLowerCase())) {
                    return layoutType;
                }
            }

            throw new IllegalArgumentException("Could not convert String id to LayoutType");
        }

        @Override
        public String toString() {
            return mId;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<LayoutType> CREATOR = new Creator<LayoutType>() {
            @Override
            public LayoutType createFromParcel(final Parcel source) {
                return LayoutType.values()[source.readInt()];
            }

            @Override
            public LayoutType[] newArray(final int size) {
                return new LayoutType[size];
            }
        };
    }

    public static enum ViewType implements Parcelable {
        LIST("list"),
        GRID("grid");

        private final String mId;

        ViewType(String id) {
            mId = id;
        }

        public static ViewType fromId(String id) {
            if (id == null) {
                throw new IllegalArgumentException("Could not convert null String to ViewType");
            }

            for (ViewType viewType : ViewType.values()) {
                if (TextUtils.equals(viewType.mId, id.toLowerCase())) {
                    return viewType;
                }
            }

            throw new IllegalArgumentException("Could not convert String id to ViewType");
        }

        @Override
        public String toString() {
            return mId;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<ViewType> CREATOR = new Creator<ViewType>() {
            @Override
            public ViewType createFromParcel(final Parcel source) {
                return ViewType.values()[source.readInt()];
            }

            @Override
            public ViewType[] newArray(final int size) {
                return new ViewType[size];
            }
        };
    }

    public static enum ItemType implements Parcelable {
        ARTICLE("article"),
        IMAGE("image");

        private final String mId;

        ItemType(String id) {
            mId = id;
        }

        public static ItemType fromId(String id) {
            if (id == null) {
                throw new IllegalArgumentException("Could not convert null String to ItemType");
            }

            for (ItemType itemType : ItemType.values()) {
                if (TextUtils.equals(itemType.mId, id.toLowerCase())) {
                    return itemType;
                }
            }

            throw new IllegalArgumentException("Could not convert String id to ItemType");
        }

        @Override
        public String toString() {
            return mId;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<ItemType> CREATOR = new Creator<ItemType>() {
            @Override
            public ItemType createFromParcel(final Parcel source) {
                return ItemType.values()[source.readInt()];
            }

            @Override
            public ItemType[] newArray(final int size) {
                return new ItemType[size];
            }
        };
    }

    public static enum ItemHandler implements Parcelable {
        BROWSER("browser"),
        INTENT("intent");

        private final String mId;

        ItemHandler(String id) {
            mId = id;
        }

        public static ItemHandler fromId(String id) {
            if (id == null) {
                throw new IllegalArgumentException("Could not convert null String to ItemHandler");
            }

            for (ItemHandler itemHandler : ItemHandler.values()) {
                if (TextUtils.equals(itemHandler.mId, id.toLowerCase())) {
                    return itemHandler;
                }
            }

            throw new IllegalArgumentException("Could not convert String id to ItemHandler");
        }

        @Override
        public String toString() {
            return mId;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<ItemHandler> CREATOR = new Creator<ItemHandler>() {
            @Override
            public ItemHandler createFromParcel(final Parcel source) {
                return ItemHandler.values()[source.readInt()];
            }

            @Override
            public ItemHandler[] newArray(final int size) {
                return new ItemHandler[size];
            }
        };
    }

    public static class ViewConfig implements Parcelable {
        private final ViewType mType;
        private final String mDatasetId;
        private final ItemType mItemType;
        private final ItemHandler mItemHandler;

        private static final String JSON_KEY_TYPE = "type";
        private static final String JSON_KEY_DATASET = "dataset";
        private static final String JSON_KEY_ITEM_TYPE = "itemType";
        private static final String JSON_KEY_ITEM_HANDLER = "itemHandler";

        public ViewConfig(JSONObject json) throws JSONException, IllegalArgumentException {
            mType = ViewType.fromId(json.getString(JSON_KEY_TYPE));
            mDatasetId = json.getString(JSON_KEY_DATASET);
            mItemType = ItemType.fromId(json.getString(JSON_KEY_ITEM_TYPE));
            mItemHandler = ItemHandler.fromId(json.getString(JSON_KEY_ITEM_HANDLER));

            validate();
        }

        @SuppressWarnings("unchecked")
        public ViewConfig(Parcel in) {
            mType = (ViewType) in.readParcelable(getClass().getClassLoader());
            mDatasetId = in.readString();
            mItemType = (ItemType) in.readParcelable(getClass().getClassLoader());
            mItemHandler = (ItemHandler) in.readParcelable(getClass().getClassLoader());

            validate();
        }

        public ViewConfig(ViewConfig viewConfig) {
            mType = viewConfig.mType;
            mDatasetId = viewConfig.mDatasetId;
            mItemType = viewConfig.mItemType;
            mItemHandler = viewConfig.mItemHandler;

            validate();
        }

        public ViewConfig(ViewType type, String datasetId, ItemType itemType, ItemHandler itemHandler) {
            mType = type;
            mDatasetId = datasetId;
            mItemType = itemType;
            mItemHandler = itemHandler;

            validate();
        }

        private void validate() {
            if (mType == null) {
                throw new IllegalArgumentException("Can't create ViewConfig with null type");
            }

            if (TextUtils.isEmpty(mDatasetId)) {
                throw new IllegalArgumentException("Can't create ViewConfig with empty dataset ID");
            }

            if (mItemType == null) {
                throw new IllegalArgumentException("Can't create ViewConfig with null item type");
            }

            if (mItemHandler == null) {
                throw new IllegalArgumentException("Can't create ViewConfig with null item handler");
            }
        }

        public ViewType getType() {
            return mType;
        }

        public String getDatasetId() {
            return mDatasetId;
        }

        public ItemType getItemType() {
            return mItemType;
        }

        public ItemHandler getItemHandler() {
            return mItemHandler;
        }

        public JSONObject toJSON() throws JSONException {
            final JSONObject json = new JSONObject();

            json.put(JSON_KEY_TYPE, mType.toString());
            json.put(JSON_KEY_DATASET, mDatasetId);
            json.put(JSON_KEY_ITEM_TYPE, mItemType.toString());
            json.put(JSON_KEY_ITEM_HANDLER, mItemHandler.toString());

            return json;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeParcelable(mType, 0);
            dest.writeString(mDatasetId);
            dest.writeParcelable(mItemType, 0);
            dest.writeParcelable(mItemHandler, 0);
        }

        public static final Creator<ViewConfig> CREATOR = new Creator<ViewConfig>() {
            @Override
            public ViewConfig createFromParcel(final Parcel in) {
                return new ViewConfig(in);
            }

            @Override
            public ViewConfig[] newArray(final int size) {
                return new ViewConfig[size];
            }
        };
    }

    public interface OnChangeListener {
        public void onChange();
    }

    public interface HomeConfigBackend {
        public List<PanelConfig> load();
        public void save(List<PanelConfig> entries);
        public String getLocale();
        public void setOnChangeListener(OnChangeListener listener);
    }

    
    private static final String TOP_SITES_PANEL_ID = "4becc86b-41eb-429a-a042-88fe8b5a094e";
    private static final String BOOKMARKS_PANEL_ID = "7f6d419a-cd6c-4e34-b26f-f68b1b551907";
    private static final String READING_LIST_PANEL_ID = "20f4549a-64ad-4c32-93e4-1dcef792733b";
    private static final String HISTORY_PANEL_ID = "f134bf20-11f7-4867-ab8b-e8e705d7fbe8";

    private final HomeConfigBackend mBackend;

    public HomeConfig(HomeConfigBackend backend) {
        mBackend = backend;
    }

    public List<PanelConfig> load() {
        return mBackend.load();
    }

    public String getLocale() {
        return mBackend.getLocale();
    }

    public void save(List<PanelConfig> panelConfigs) {
        mBackend.save(panelConfigs);
    }

    public void setOnChangeListener(OnChangeListener listener) {
        mBackend.setOnChangeListener(listener);
    }

    public static PanelConfig createBuiltinPanelConfig(Context context, PanelType panelType) {
        return createBuiltinPanelConfig(context, panelType, EnumSet.noneOf(PanelConfig.Flags.class));
    }

    public static PanelConfig createBuiltinPanelConfig(Context context, PanelType panelType, EnumSet<PanelConfig.Flags> flags) {
        int titleId = 0;
        String id = null;

        switch(panelType) {
            case TOP_SITES:
                titleId = R.string.home_top_sites_title;
                id = TOP_SITES_PANEL_ID;
                break;

            case BOOKMARKS:
                titleId = R.string.bookmarks_title;
                id = BOOKMARKS_PANEL_ID;
                break;

            case HISTORY:
                titleId = R.string.home_history_title;
                id = HISTORY_PANEL_ID;
                break;

            case READING_LIST:
                titleId = R.string.reading_list_title;
                id = READING_LIST_PANEL_ID;
                break;

            case DYNAMIC:
                throw new IllegalArgumentException("createBuiltinPanelConfig() is only for built-in panels");
        }

        return new PanelConfig(panelType, context.getString(titleId), id, flags);
    }

    public static HomeConfig getDefault(Context context) {
        return new HomeConfig(new HomeConfigPrefsBackend(context));
    }
}
