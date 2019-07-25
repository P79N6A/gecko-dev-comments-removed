




































package org.mozilla.gecko.db;

import android.net.Uri;

public class BrowserContract {
    public static final String AUTHORITY = "org.mozilla.gecko.providers.browser";

    public static final Uri AUTHORITY_URI = Uri.parse("content://" + AUTHORITY);

    public static final String DEFAULT_PROFILE = "default";

    public static final String PARAM_PROFILE = "profile";

    public static final String PARAM_LIMIT = "limit";

    interface SyncColumns {
        public static final String GUID = "guid";

        public static final String DATE_CREATED = "created";

        public static final String DATE_MODIFIED = "modified";
    }

    interface CommonColumns {
        public static final String _ID = "_id";

        public static final String URL = "url";

        public static final String TITLE = "title";
    }

    interface ImageColumns {
        public static final String FAVICON = "favicon";

        public static final String THUMBNAIL = "thumbnail";
    }

    public static final class Images implements ImageColumns, SyncColumns {
        private Images() {}

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "images");

        public static final String URL = "url_key";

        public static final String FAVICON_URL = "favicon_url";
    }

    public static final class Bookmarks implements CommonColumns, ImageColumns, SyncColumns {
        private Bookmarks() {}

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "bookmarks");

        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/bookmark";

        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/bookmark";

        public static final String IS_FOLDER = "folder";

        public static final String PARENT = "parent";

        public static final String POSITION = "position";
    }

    public static final class History implements CommonColumns, ImageColumns, SyncColumns {
        private History() {}

        public static final Uri CONTENT_URI = Uri.withAppendedPath(AUTHORITY_URI, "history");

        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/browser-history";

        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/browser-history";

        public static final String DATE_LAST_VISITED = "date";

        public static final String VISITS = "visits";
    }
}
