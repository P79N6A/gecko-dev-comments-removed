




































package org.mozilla.gecko.db;

import android.net.Uri;

public class BrowserContract {
    public static final String AUTHORITY = "org.mozilla.gecko.providers.browser";

    public static final Uri AUTHORITY_URI = Uri.parse("content://" + AUTHORITY);

    public static final String DEFAULT_PROFILE = "default";

    public static final String PARAM_PROFILE = "profile";

    public static final String PARAM_LIMIT = "limit";

    public static final String PARAM_IS_SYNC = "sync";

    public static final String PARAM_SHOW_DELETED = "show_deleted";

    interface CommonColumns {
        public static final String _ID = "_id";
    }

    interface SyncColumns {
        public static final String GUID = "guid";

        public static final String DATE_CREATED = "created";

        public static final String DATE_MODIFIED = "modified";

        public static final String IS_DELETED = "deleted";
    }

    interface URLColumns {
        public static final String URL = "url";

        public static final String TITLE = "title";
    }

    interface ImageColumns {
        public static final String FAVICON = "favicon";

        public static final String THUMBNAIL = "thumbnail";
    }

    public static final class Images implements CommonColumns, ImageColumns, SyncColumns {
        private Images() {}

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "images");

        public static final String URL = "url_key";

        public static final String FAVICON_URL = "favicon_url";
    }

    public static final class Bookmarks implements CommonColumns, URLColumns, ImageColumns, SyncColumns {
        private Bookmarks() {}

        public static final String MOBILE_FOLDER_GUID = "mobile";
        public static final String PLACES_FOLDER_GUID = "places";
        public static final String MENU_FOLDER_GUID = "menu";
        public static final String TAGS_FOLDER_GUID = "tags";
        public static final String TOOLBAR_FOLDER_GUID = "toolbar";
        public static final String UNFILED_FOLDER_GUID = "unfiled";

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "bookmarks");

        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/bookmark";

        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/bookmark";

        public static final String IS_FOLDER = "folder";

        public static final String PARENT = "parent";

        public static final String POSITION = "position";

        public static final String TAGS = "tags";

        public static final String DESCRIPTION = "description";

        public static final String KEYWORD = "keyword";
    }

    public static final class History implements CommonColumns, URLColumns, ImageColumns, SyncColumns {
        private History() {}

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "history");

        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/browser-history";

        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/browser-history";

        public static final String DATE_LAST_VISITED = "date";

        public static final String VISITS = "visits";
    }

    public static final class Schema {
        private Schema() {}

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "schema");

        public static final String VERSION = "version";
    }
}