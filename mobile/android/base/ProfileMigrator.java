




































package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.History;
import org.mozilla.gecko.db.BrowserContract.ImageColumns;
import org.mozilla.gecko.db.BrowserContract.Images;
import org.mozilla.gecko.db.BrowserContract.Passwords;
import org.mozilla.gecko.db.BrowserContract.URLColumns;
import org.mozilla.gecko.db.BrowserContract.SyncColumns;
import org.mozilla.gecko.sqlite.SQLiteBridge;
import org.mozilla.gecko.sqlite.SQLiteBridgeException;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;

import android.accounts.Account;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.ContentProviderResult;
import android.content.ContentProviderOperation;
import android.content.Context;
import android.content.OperationApplicationException;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteConstraintException;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.RemoteException;
import android.provider.Browser;
import android.text.TextUtils;
import android.util.Log;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.File;
import java.io.InputStream;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

public class ProfileMigrator {
    private static final String LOGTAG = "ProfileMigrator";
    private static final String PREFS_NAME = "ProfileMigrator";
    private File mProfileDir;
    private ContentResolver mCr;
    private Context mContext;
    private Runnable mLongOperationStartCallback;
    private boolean mLongOperationStartRun;
    private Runnable mLongOperationStopCallback;

    
    private static final int DEFAULT_HISTORY_MIGRATE_COUNT = 2000;

    
    
    private static final int HISTORY_MAX_BATCH = 5000;

    private static final String PREFS_MIGRATE_BOOKMARKS_DONE = "bookmarks_done";
    private static final String PREFS_MIGRATE_HISTORY_DONE = "history_done";
    
    private static final String PREFS_MIGRATE_HISTORY_COUNT = "history_count";
    private static final String PREFS_MIGRATE_SYNC_DONE = "sync_done";

    



    private static final String ROOT_QUERY =
        "SELECT root_name, folder_id FROM moz_bookmarks_roots";
    private static final String ROOT_NAME      = "root_name";
    private static final String ROOT_FOLDER_ID = "folder_id";

    
    private static final String ROOT_TAGS_FOLDER_NAME = "tags";

    private static final String BOOKMARK_QUERY_SELECT =
        "SELECT places.url             AS p_url,"         +
        "       bookmark.guid          AS b_guid,"        +
        "       bookmark.id            AS b_id,"          +
        "       bookmark.title         AS b_title,"       +
        "       bookmark.type          AS b_type,"        +
        "       bookmark.parent        AS b_parent,"      +
        "       bookmark.dateAdded     AS b_added,"       +
        "       bookmark.lastModified  AS b_modified,"    +
        "       bookmark.position      AS b_position,";

    private static final String BOOKMARK_QUERY_TRAILER =
        "FROM ((moz_bookmarks AS bookmark "               +
        "       LEFT OUTER JOIN moz_places AS places "    +
        "       ON places.id = bookmark.fk) "             +
        "       LEFT OUTER JOIN moz_favicons AS favicon " +
        "       ON places.favicon_id = favicon.id) "      +
        
        "WHERE (places.hidden IS NULL "                   +
        "       OR places.hidden <> 1) "                  +
        
        
        "ORDER BY bookmark.id";

    private static final String BOOKMARK_QUERY_GUID =
        BOOKMARK_QUERY_SELECT                              +
        "       favicon.data           AS f_data,"        +
        "       favicon.mime_type      AS f_mime_type,"   +
        "       favicon.url            AS f_url,"         +
        "       favicon.guid           AS f_guid "        +
        BOOKMARK_QUERY_TRAILER;

    private static final String BOOKMARK_QUERY_NO_GUID =
        BOOKMARK_QUERY_SELECT                              +
        "       favicon.data           AS f_data,"        +
        "       favicon.mime_type      AS f_mime_type,"   +
        "       favicon.url            AS f_url "         +
        BOOKMARK_QUERY_TRAILER;

    
    private static final String BOOKMARK_URL      = "p_url";
    private static final String BOOKMARK_TITLE    = "b_title";
    private static final String BOOKMARK_GUID     = "b_guid";
    private static final String BOOKMARK_ID       = "b_id";
    private static final String BOOKMARK_TYPE     = "b_type";
    private static final String BOOKMARK_PARENT   = "b_parent";
    private static final String BOOKMARK_ADDED    = "b_added";
    private static final String BOOKMARK_MODIFIED = "b_modified";
    private static final String BOOKMARK_POSITION = "b_position";
    private static final String FAVICON_DATA      = "f_data";
    private static final String FAVICON_MIME      = "f_mime_type";
    private static final String FAVICON_URL       = "f_url";
    private static final String FAVICON_GUID      = "f_guid";

    
    private static final int PLACES_TYPE_BOOKMARK = 1;
    private static final int PLACES_TYPE_FOLDER   = 2;

    


    private static final String HISTORY_COUNT_QUERY =
        "SELECT COUNT(*) FROM moz_historyvisits";

    





    private static final String HISTORY_QUERY_SELECT =
        "SELECT places.url              AS p_url, "       +
        "       places.title            AS p_title, "     +
        "       places.guid             AS p_guid, "      +
        "       MAX(history.visit_date) AS h_date, "      +
        "       COUNT(*) AS h_visits, "                   +
        
        "       MAX(1, 100 * 225 / (" +
        "          ((MAX(history.visit_date)/1000 - ?) / 86400000) * " +
        "          ((MAX(history.visit_date)/1000 - ?) / 86400000) + 225)) AS a_recent, ";

    private static final String HISTORY_QUERY_TRAILER =
        "FROM (moz_historyvisits AS history "             +
        "      JOIN moz_places AS places "                +
        "      ON places.id = history.place_id "          +
        
        "      LEFT OUTER JOIN moz_favicons AS favicon "  +
        "      ON places.favicon_id = favicon.id) "       +
        "WHERE places.hidden <> 1 "                       +
        "GROUP BY p_url "                                 +
        "ORDER BY h_visits * a_recent "                   +
        "DESC LIMIT ? OFFSET ?";

    private static final String HISTORY_QUERY_GUID =
        HISTORY_QUERY_SELECT                               +
        "       favicon.data            AS f_data, "      +
        "       favicon.mime_type       AS f_mime_type, " +
        "       favicon.url             AS f_url, "       +
        "       favicon.guid            AS f_guid "       +
        HISTORY_QUERY_TRAILER;

    private static final String HISTORY_QUERY_NO_GUID =
        HISTORY_QUERY_SELECT                               +
        "       favicon.data            AS f_data, "      +
        "       favicon.mime_type       AS f_mime_type, " +
        "       favicon.url             AS f_url "        +
        HISTORY_QUERY_TRAILER;

    private static final String HISTORY_URL    = "p_url";
    private static final String HISTORY_TITLE  = "p_title";
    private static final String HISTORY_GUID   = "p_guid";
    private static final String HISTORY_DATE   = "h_date";
    private static final String HISTORY_VISITS = "h_visits";

    


    private static final String[] SYNC_SETTINGS_LIST = new String[] {
        "services.sync.account",
        "services.sync.client.name",
        "services.sync.client.GUID",
        "services.sync.serverURL",
        "services.sync.clusterURL"
    };

    


    private static final String SYNC_HOST_NAME = "chrome://weave";
    private static final String[] SYNC_REALM_LIST = new String[] {
        "Mozilla Services Password",
        "Mozilla Services Encryption Passphrase"
    };


    public ProfileMigrator(Context context, File profileDir) {
        mProfileDir = profileDir;
        mContext = context;
        mCr = mContext.getContentResolver();
        mLongOperationStartCallback = null;
        mLongOperationStopCallback = null;
    }

    
    
    public void setLongOperationCallbacks(Runnable start,
                                          Runnable stop) {
        mLongOperationStartCallback = start;
        mLongOperationStopCallback = stop;
        mLongOperationStartRun = false;
    }

    public void launchPlaces() {
        boolean timeThisRun = false;
        Telemetry.Timer timer = null;
        
        if (!hasMigrationRun()) {
            timeThisRun = true;
            timer = new Telemetry.Timer("BROWSERPROVIDER_XUL_IMPORT_TIME");
        }
        launchPlaces(DEFAULT_HISTORY_MIGRATE_COUNT);
        if (timeThisRun)
            timer.stop();
    }

    public void launchPlaces(int maxEntries) {
        mLongOperationStartRun = false;
        
        
        new PlacesRunnable(maxEntries).run();
    }

    public void launchSyncPrefs() {
        
        new SyncTask().run();
    }

    public boolean areBookmarksMigrated() {
        return getPreferences().getBoolean(PREFS_MIGRATE_BOOKMARKS_DONE, false);
    }

    public boolean isHistoryMigrated() {
        return getPreferences().getBoolean(PREFS_MIGRATE_HISTORY_DONE, false);
    }

    
    public boolean hasSyncMigrated() {
        return getPreferences().getBoolean(PREFS_MIGRATE_SYNC_DONE, false);
    }

    
    protected boolean hasMigrationRun() {
        return areBookmarksMigrated()
            && ((getMigratedHistoryEntries() > 0) || isHistoryMigrated());
    }

    
    protected boolean hasMigrationFinished() {
        return areBookmarksMigrated() && isHistoryMigrated();
    }

    protected SharedPreferences getPreferences() {
        return mContext.getSharedPreferences(PREFS_NAME, 0);
    }

    protected int getMigratedHistoryEntries() {
        return getPreferences().getInt(PREFS_MIGRATE_HISTORY_COUNT, 0);
    }

    protected void setMigratedHistoryEntries(int count) {
        SharedPreferences.Editor editor = getPreferences().edit();
        editor.putInt(PREFS_MIGRATE_HISTORY_COUNT, count);
        editor.commit();
    }

    protected void setMigratedHistory() {
        SharedPreferences.Editor editor = getPreferences().edit();
        editor.putBoolean(PREFS_MIGRATE_HISTORY_DONE, true);
        editor.commit();
    }

    protected void setMigratedBookmarks() {
        SharedPreferences.Editor editor = getPreferences().edit();
        editor.putBoolean(PREFS_MIGRATE_BOOKMARKS_DONE, true);
        editor.commit();
    }

    protected void setMigratedSync() {
        SharedPreferences.Editor editor = getPreferences().edit();
        editor.putBoolean(PREFS_MIGRATE_SYNC_DONE, true);
        editor.commit();
    }

    private class SyncTask implements Runnable, GeckoEventListener {
        private List<String> mSyncSettingsList;
        private Map<String, String> mSyncSettingsMap;

        
        protected void requestValues() {
            mSyncSettingsList = Arrays.asList(SYNC_SETTINGS_LIST);
            mSyncSettingsMap = new HashMap<String, String>();
            JSONArray jsonPrefs = new JSONArray(mSyncSettingsList);
            Log.d(LOGTAG, "Sending: " + jsonPrefs.toString());
            GeckoEvent event =
                GeckoEvent.createBroadcastEvent("Preferences:Get",
                                                jsonPrefs.toString());
            GeckoAppShell.sendEventToGecko(event);
        }

        
        public void handleMessage(String event, JSONObject message) {
            Log.d(LOGTAG, "Received event: " + event);
            try {
                if (event.equals("Preferences:Data")) {
                    
                    
                    
                    JSONArray jsonPrefs = message.getJSONArray("preferences");
                    parsePrefs(jsonPrefs);
                    GeckoAppShell.unregisterGeckoEventListener("Preferences:Data",
                                                               (GeckoEventListener)this);

                    
                    for (String location: SYNC_REALM_LIST) {
                        Log.d(LOGTAG, "Checking: " + location);
                        String passwd = getPassword(location);
                        if (!TextUtils.isEmpty(passwd)) {
                            Log.d(LOGTAG, "Got password");
                            mSyncSettingsMap.put(location, passwd);
                        } else {
                            Log.d(LOGTAG, "No password found");
                            mSyncSettingsMap.put(location, null);
                        }
                    }

                    
                    configureSync();
                }
            } catch (Exception e) {
                Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
            }
        }

        protected String getPassword(String realm) {
            Cursor cursor = null;
            String result = null;
            try {
                cursor = mCr.query(Passwords.CONTENT_URI,
                                   null,
                                   Passwords.HOSTNAME + " = ? AND "
                                   + Passwords.HTTP_REALM + " = ?",
                                   new String[] { SYNC_HOST_NAME, realm },
                                   null);

                if (cursor != null) {
                    final int userCol =
                        cursor.getColumnIndexOrThrow(Passwords.ENCRYPTED_USERNAME);
                    final int passCol =
                        cursor.getColumnIndexOrThrow(Passwords.ENCRYPTED_PASSWORD);

                    if (cursor.moveToFirst()) {
                        String user = cursor.getString(userCol);
                        String pass = cursor.getString(passCol);
                        result = pass;
                    } else {
                        Log.i(LOGTAG, "No password found for realm = " + realm);
                    }
                }
            } finally {
                if (cursor != null)
                    cursor.close();
            }

            return result;
        }

        protected void parsePrefs(JSONArray jsonPrefs) {
            try {
                final int length = jsonPrefs.length();
                for (int i = 0; i < length; i++) {
                    JSONObject jPref = jsonPrefs.getJSONObject(i);
                    final String prefName = jPref.getString("name");
                    final String prefType = jPref.getString("type");
                    if ("bool".equals(prefType)) {
                        final boolean value = jPref.getBoolean("value");
                        mSyncSettingsMap.put(prefName, value ? "1" : "0");
                    } else {
                        final String value = jPref.getString("value");
                        if (!TextUtils.isEmpty(value)) {
                            mSyncSettingsMap.put(prefName, value);
                        } else {
                            Log.w(LOGTAG, "Could not recover setting for = " + prefName);
                            mSyncSettingsMap.put(prefName, null);
                        }
                    }
                }
            } catch (JSONException e) {
                Log.e(LOGTAG, "Exception handling preferences answer: "
                      + e.getMessage());
            }
        }

        protected void configureSync() {
            final String userName = mSyncSettingsMap.get("services.sync.account");
            final String syncKey = mSyncSettingsMap.get("Mozilla Services Password");
            final String syncPass = mSyncSettingsMap.get("Mozilla Services Encryption Passphrase");
            final String serverURL = mSyncSettingsMap.get("services.sync.serverURL");
            final String clusterURL = mSyncSettingsMap.get("services.sync.clusterURL");
            final String clientName = mSyncSettingsMap.get("services.sync.client.name");
            final String clientGuid = mSyncSettingsMap.get("services.sync.client.GUID");

            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    if (userName == null || syncKey == null || syncPass == null) {
                        
                        Log.e(LOGTAG, "Profile has incomplete Sync config. Not migrating.");
                        setMigratedSync();
                        return;
                    }

                    final SyncAccountParameters params =
                        new SyncAccountParameters(mContext, null,
                                                  userName, syncKey,
                                                  syncPass, serverURL, clusterURL,
                                                  clientName, clientGuid);

                    final Account account = SyncAccounts.createSyncAccount(params);
                    if (account == null) {
                        Log.e(LOGTAG, "Failed to migrate Sync account.");
                    } else {
                        Log.i(LOGTAG, "Migrating Sync account succeeded.");
                    }
                    setMigratedSync();
                }
            });
        }

        protected void registerAndRequest() {
            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    GeckoAppShell.registerGeckoEventListener("Preferences:Data",
                                                             SyncTask.this);
                    requestValues();
                }
            });
        }

        @Override
        public void run() {
            
            new SyncAccounts.AccountsExistTask() {
                @Override
                protected void onPostExecute(Boolean result) {
                    if (result.booleanValue()) {
                        GeckoAppShell.getHandler().post(new Runnable() {
                            public void run() {
                                Log.i(LOGTAG, "Sync account already configured, skipping.");
                                setMigratedSync();
                            }
                        });
                    } else {
                        
                        registerAndRequest();
                    }
                }
            }.execute(mContext);
        }
    }

    private class MiscTask implements Runnable {
        protected void cleanupXULLibCache() {
            File cacheFile = GeckoAppShell.getCacheDir(mContext);
            File[] files = cacheFile.listFiles();
            if (files != null) {
                Iterator<File> cacheFiles = Arrays.asList(files).iterator();
                while (cacheFiles.hasNext()) {
                    File libFile = cacheFiles.next();
                    if (libFile.getName().endsWith(".so")) {
                        libFile.delete();
                    }
                }
            }
        }

        @Override
        public void run() {
            
            
        }
    }

    private class PlacesRunnable implements Runnable {
        private Map<Long, Long> mRerootMap;
        private Long mTagsPlacesFolderId;
        private ArrayList<ContentProviderOperation> mOperations;
        private int mMaxEntries;
        
        
        
        private boolean mHasFaviconGUID;

        public PlacesRunnable(int limit) {
            mMaxEntries = limit;
        }

        protected Uri getBookmarksUri() {
            Uri.Builder uriBuilder = Bookmarks.CONTENT_URI.buildUpon()
                .appendQueryParameter(BrowserContract.PARAM_SHOW_DELETED, "1");
            return uriBuilder.build();
        }

        protected Uri getHistoryUri() {
            Uri.Builder uriBuilder = History.CONTENT_URI.buildUpon()
                .appendQueryParameter(BrowserContract.PARAM_SHOW_DELETED, "1");
            return uriBuilder.build();

        }

        protected Uri getImagesUri() {
            Uri.Builder uriBuilder = Images.CONTENT_URI.buildUpon()
                .appendQueryParameter(BrowserContract.PARAM_SHOW_DELETED, "1");
            return uriBuilder.build();
        }

        private long getFolderId(String guid) {
            Cursor c = null;

            try {
                c = mCr.query(getBookmarksUri(),
                              new String[] { Bookmarks._ID },
                              Bookmarks.GUID + " = ?",
                              new String [] { guid },
                              null);
                if (c.moveToFirst())
                    return c.getLong(c.getColumnIndexOrThrow(Bookmarks._ID));
            } finally {
                if (c != null)
                    c.close();
            }
            
            return Bookmarks.FIXED_ROOT_ID;
        }

        
        public boolean checkPlacesSchema(SQLiteBridge db) {
            final int schemaVersion = db.getVersion();
            Log.d(LOGTAG, "Schema version " + schemaVersion);
            if (schemaVersion < 12) {
                Log.e(LOGTAG, "Places DB is too old, not migrating.");
                return false;
            } else if (schemaVersion >= 12 && schemaVersion <= 13) {
                Log.d(LOGTAG, "Not Migrating Favicon GUIDs.");
                mHasFaviconGUID = false;
                return true;
            } else if (schemaVersion <= 20) {
                Log.d(LOGTAG, "Migrating Favicon GUIDs.");
                mHasFaviconGUID = true;
                return true;
            } else {
                Log.e(LOGTAG, "Too new (corrupted?) Places schema.");
                return false;
            }
        }

        
        
        protected void calculateReroot(SQLiteBridge db) {
            mRerootMap = new HashMap<Long, Long>();

            try {
                Cursor cursor = db.rawQuery(ROOT_QUERY, null);
                final int rootCol = cursor.getColumnIndex(ROOT_NAME);
                final int folderCol = cursor.getColumnIndex(ROOT_FOLDER_ID);

                cursor.moveToFirst();
                while (!cursor.isAfterLast()) {
                    String name = cursor.getString(rootCol);
                    long placesFolderId = cursor.getLong(folderCol);
                    mRerootMap.put(placesFolderId, getFolderId(name));
                    Log.v(LOGTAG, "Name: " + name + ", pid=" + placesFolderId
                          + ", nid=" + mRerootMap.get(placesFolderId));

                    
                    
                    if (ROOT_TAGS_FOLDER_NAME.equals(name))
                        mTagsPlacesFolderId = placesFolderId;

                    cursor.moveToNext();
                }
                cursor.close();
            } catch (SQLiteBridgeException e) {
                Log.e(LOGTAG, "Failed to get bookmark roots: ", e);
                
                setMigratedBookmarks();
                return;
            }
        }

        protected void updateBrowserHistory(String url, String title,
                                            long date, int visits) {
            Cursor cursor = null;

            try {
                final String[] projection = new String[] {
                    History._ID,
                    History.VISITS,
                    History.DATE_LAST_VISITED
                };

                cursor = mCr.query(getHistoryUri(),
                                   projection,
                                   History.URL + " = ?",
                                   new String[] { url },
                                   null);

                ContentValues values = new ContentValues();
                ContentProviderOperation.Builder builder = null;
                
                values.put(History.IS_DELETED, 0);

                if (cursor.moveToFirst()) {
                    int visitsCol = cursor.getColumnIndexOrThrow(History.VISITS);
                    int dateCol = cursor.getColumnIndexOrThrow(History.DATE_LAST_VISITED);
                    int oldVisits = cursor.getInt(visitsCol);
                    long oldDate = cursor.getLong(dateCol);

                    values.put(History.VISITS, oldVisits + visits);
                    if (title != null) {
                        values.put(History.TITLE, title);
                    }
                    
                    if (date > oldDate) {
                        values.put(History.DATE_LAST_VISITED, date);
                    }

                    int idCol = cursor.getColumnIndexOrThrow(History._ID);
                    
                    Uri historyUri = ContentUris.withAppendedId(getHistoryUri(),
                                                                cursor.getLong(idCol));

                    
                    builder = ContentProviderOperation.newUpdate(historyUri);
                    
                    builder.withExpectedCount(1);
                    builder.withValues(values);
                } else {
                    values.put(History.URL, url);
                    values.put(History.VISITS, visits);
                    if (title != null) {
                        values.put(History.TITLE, title);
                    } else {
                        values.put(History.TITLE, url);
                    }
                    values.put(History.DATE_LAST_VISITED, date);

                    
                    builder = ContentProviderOperation.newInsert(getHistoryUri());
                    builder.withValues(values);
                }

                
                mOperations.add(builder.build());
            } finally {
                if (cursor != null)
                    cursor.close();
            }
        }

        protected BitmapDrawable decodeImageData(byte[] data) {
            InputStream byteStream = new ByteArrayInputStream(data);
            BitmapDrawable image =
                (BitmapDrawable)Drawable.createFromStream(byteStream, "src");
            return image;
        }

        protected void addFavicon(String url, String faviconUrl, String faviconGuid,
                                  String mime, byte[] data) {
            
            
            if (mime == null || mime.compareTo("image/gif") == 0) {
                return;
            }
            BitmapDrawable image = null;
            
            if (mime.compareTo("image/png") != 0) {
                image = decodeImageData(data);
                
                if (image == null) {
                    Log.i(LOGTAG, "Cannot decode image type " + mime
                          + " for URL=" + url);
                }
            }
            try {
                ContentValues values = new ContentValues();

                
                if (image != null) {
                    Bitmap bitmap = image.getBitmap();
                    ByteArrayOutputStream stream = new ByteArrayOutputStream();
                    bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);
                    values.put(Images.FAVICON, stream.toByteArray());
                } else {
                    
                    
                    values.put(Images.FAVICON, data);
                }

                values.put(Images.URL, url);
                values.put(Images.FAVICON_URL, faviconUrl);
                
                values.put(Images.IS_DELETED, 0);
                if (faviconGuid != null) {
                    values.put(Images.GUID, faviconGuid);
                }

                Cursor cursor = null;
                ContentProviderOperation.Builder builder = null;
                try {
                    cursor = mCr.query(getImagesUri(),
                                       null,
                                       Images.URL + " = ?",
                                       new String[] { url },
                                       null);

                    if (cursor != null && cursor.moveToFirst()) {
                        
                        builder = ContentProviderOperation.newUpdate(getImagesUri());
                        
                        builder.withExpectedCount(1);
                        builder.withValues(values);
                        builder.withSelection(Images.URL + " = ?",
                                              new String[] { url });
                    } else {
                        
                        builder = ContentProviderOperation.newInsert(getImagesUri());
                        builder.withValues(values);
                    }
                } finally {
                    if (cursor != null)
                        cursor.close();
                }

                
                mOperations.add(builder.build());
            } catch (SQLException e) {
                Log.i(LOGTAG, "Migrating favicon failed: " + mime + " URL: " + url
                      + " error:" + e.getMessage());
            }
        }

        protected void doMigrateHistoryBatch(SQLiteBridge db,
                                             int maxEntries, int currentEntries) {
            final ArrayList<String> placesHistory = new ArrayList<String>();
            mOperations = new ArrayList<ContentProviderOperation>();
            int queryResultEntries = 0;

            try {
                Cursor cursor = db.rawQuery(HISTORY_COUNT_QUERY, null);
                cursor.moveToFirst();
                int historyCount = cursor.getInt(0);
                Telemetry.HistogramAdd("BROWSERPROVIDER_XUL_IMPORT_HISTORY",
                                       historyCount);

                final String currentTime = Long.toString(System.currentTimeMillis());
                final String[] queryParams = new String[] {
                    
                    currentTime,
                    currentTime,
                    Integer.toString(maxEntries),
                    Integer.toString(currentEntries)
                };

                if (mHasFaviconGUID) {
                    cursor = db.rawQuery(HISTORY_QUERY_GUID, queryParams);
                } else {
                    cursor = db.rawQuery(HISTORY_QUERY_NO_GUID, queryParams);
                }
                queryResultEntries = cursor.getCount();

                final int urlCol = cursor.getColumnIndex(HISTORY_URL);
                final int titleCol = cursor.getColumnIndex(HISTORY_TITLE);
                final int dateCol = cursor.getColumnIndex(HISTORY_DATE);
                final int visitsCol = cursor.getColumnIndex(HISTORY_VISITS);
                final int faviconMimeCol = cursor.getColumnIndex(FAVICON_MIME);
                final int faviconDataCol = cursor.getColumnIndex(FAVICON_DATA);
                final int faviconUrlCol = cursor.getColumnIndex(FAVICON_URL);
                
                final int faviconGuidCol = cursor.getColumnIndex(FAVICON_GUID);

                cursor.moveToFirst();
                while (!cursor.isAfterLast()) {
                    String url = cursor.getString(urlCol);
                    String title = cursor.getString(titleCol);
                    long date = cursor.getLong(dateCol) / (long)1000;
                    int visits = cursor.getInt(visitsCol);
                    byte[] faviconDataBuff = cursor.getBlob(faviconDataCol);
                    String faviconMime = cursor.getString(faviconMimeCol);
                    String faviconUrl = cursor.getString(faviconUrlCol);
                    String faviconGuid = null;
                    if (mHasFaviconGUID) {
                        faviconGuid = cursor.getString(faviconGuidCol);
                    }

                    try {
                        placesHistory.add(url);
                        addFavicon(url, faviconUrl, faviconGuid,
                                   faviconMime, faviconDataBuff);
                        updateBrowserHistory(url, title, date, visits);
                    } catch (Exception e) {
                        Log.e(LOGTAG, "Error adding history entry: ", e);
                    }
                    cursor.moveToNext();
                }
                cursor.close();
            } catch (SQLiteBridgeException e) {
                Log.e(LOGTAG, "Failed to get history: ", e);
                
                setMigratedHistory();
                return;
            }

            flushBatchOperations();

            int totalEntries = currentEntries + queryResultEntries;
            setMigratedHistoryEntries(totalEntries);

            
            
            if (queryResultEntries < mMaxEntries) {
                setMigratedHistory();
            }

            
            
            GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        for (String url : placesHistory) {
                            GlobalHistory.getInstance().addToGeckoOnly(url);
                        }
                    }
             });
        }

        protected void migrateHistory(SQLiteBridge db) {
            for (int i = 0; i < mMaxEntries; i += HISTORY_MAX_BATCH) {
                int currentEntries = getMigratedHistoryEntries();
                int fetchEntries = Math.min(mMaxEntries, HISTORY_MAX_BATCH);

                Log.i(LOGTAG, "Processed " + currentEntries + " history entries");
                Log.i(LOGTAG, "Fetching " + fetchEntries + " more history entries");

                doMigrateHistoryBatch(db, fetchEntries, currentEntries);
            }
        }

        protected void addBookmark(String url, String title, String guid,
                                   long parent, long added,
                                   long modified, long position,
                                   boolean folder) {
            ContentValues values = new ContentValues();
            if (title == null && url != null) {
                title = url;
            }
            if (title != null) {
                values.put(Bookmarks.TITLE, title);
            }
            if (url != null) {
                values.put(Bookmarks.URL, url);
            }
            if (guid != null) {
                values.put(SyncColumns.GUID, guid);
            }
            values.put(SyncColumns.DATE_CREATED, added);
            values.put(SyncColumns.DATE_MODIFIED, modified);
            values.put(Bookmarks.POSITION, position);
            
            values.put(Bookmarks.IS_DELETED, 0);
            if (mRerootMap.containsKey(parent)) {
                parent = mRerootMap.get(parent);
            }
            values.put(Bookmarks.PARENT, parent);
            values.put(Bookmarks.TYPE, (folder ? Bookmarks.TYPE_FOLDER : Bookmarks.TYPE_BOOKMARK));

            Cursor cursor = null;
            ContentProviderOperation.Builder builder = null;

            if (url != null) {
                try {
                    final String[] projection = new String[] {
                        Bookmarks._ID,
                        Bookmarks.URL
                    };

                    
                    cursor = mCr.query(getBookmarksUri(),
                                       projection,
                                       Bookmarks.URL + " = ?",
                                       new String[] { url },
                                       null);

                    if (cursor.moveToFirst()) {
                        int idCol = cursor.getColumnIndexOrThrow(Bookmarks._ID);
                        
                        Uri bookmarkUri = ContentUris.withAppendedId(getBookmarksUri(),
                                                                     cursor.getLong(idCol));
                        
                        builder = ContentProviderOperation.newUpdate(bookmarkUri);
                        
                        builder.withExpectedCount(1);
                        builder.withValues(values);
                    } else {
                        
                        builder = ContentProviderOperation.newInsert(getBookmarksUri());
                        builder.withValues(values);
                    }
                } finally {
                    if (cursor != null)
                        cursor.close();
                }
            } else {
                
                builder = ContentProviderOperation.newInsert(getBookmarksUri());
                builder.withValues(values);
            }

            
            mOperations.add(builder.build());
        }

        protected void migrateBookmarks(SQLiteBridge db) {
            mOperations = new ArrayList<ContentProviderOperation>();

            try {
                Log.i(LOGTAG, "Fetching bookmarks from places");

                Cursor cursor = null;
                if (mHasFaviconGUID) {
                    cursor = db.rawQuery(BOOKMARK_QUERY_GUID, null);
                } else {
                    cursor = db.rawQuery(BOOKMARK_QUERY_NO_GUID, null);
                }
                final int urlCol = cursor.getColumnIndex(BOOKMARK_URL);
                final int titleCol = cursor.getColumnIndex(BOOKMARK_TITLE);
                final int guidCol = cursor.getColumnIndex(BOOKMARK_GUID);
                final int idCol = cursor.getColumnIndex(BOOKMARK_ID);
                final int typeCol = cursor.getColumnIndex(BOOKMARK_TYPE);
                final int parentCol = cursor.getColumnIndex(BOOKMARK_PARENT);
                final int addedCol = cursor.getColumnIndex(BOOKMARK_ADDED);
                final int modifiedCol = cursor.getColumnIndex(BOOKMARK_MODIFIED);
                final int positionCol = cursor.getColumnIndex(BOOKMARK_POSITION);
                final int faviconMimeCol = cursor.getColumnIndex(FAVICON_MIME);
                final int faviconDataCol = cursor.getColumnIndex(FAVICON_DATA);
                final int faviconUrlCol = cursor.getColumnIndex(FAVICON_URL);
                final int faviconGuidCol = cursor.getColumnIndex(FAVICON_GUID);

                
                int bookmarkCount = cursor.getCount();
                Telemetry.HistogramAdd("BROWSERPROVIDER_XUL_IMPORT_BOOKMARKS",
                                       bookmarkCount);

                
                Set<Long> openFolders = new HashSet<Long>();
                Set<Long> knownFolders = new HashSet<Long>(mRerootMap.keySet());

                
                
                
                
                
                
                Set<Long> processedBookmarks = new HashSet<Long>(mRerootMap.keySet());

                int iterations = 0;
                do {
                    
                    
                    openFolders.clear();

                    int added = 0;
                    int skipped = 0;

                    cursor.moveToFirst();
                    while (!cursor.isAfterLast()) {
                        long id = cursor.getLong(idCol);

                        
                        if (processedBookmarks.contains(id)) {
                            cursor.moveToNext();
                            continue;
                        }

                        int type = cursor.getInt(typeCol);
                        long parent = cursor.getLong(parentCol);

                        
                        
                        if ((id == 1 && parent == 0 && type == PLACES_TYPE_FOLDER) ||
                            parent == mTagsPlacesFolderId) {
                            cursor.moveToNext();
                            continue;
                        }

                        String url = cursor.getString(urlCol);
                        String title = cursor.getString(titleCol);
                        String guid = cursor.getString(guidCol);
                        long dateadded =
                            cursor.getLong(addedCol) / (long)1000;
                        long datemodified =
                            cursor.getLong(modifiedCol) / (long)1000;
                        long position = cursor.getLong(positionCol);
                        byte[] faviconDataBuff = cursor.getBlob(faviconDataCol);
                        String faviconMime = cursor.getString(faviconMimeCol);
                        String faviconUrl = cursor.getString(faviconUrlCol);
                        String faviconGuid = null;
                        if (mHasFaviconGUID) {
                            faviconGuid = cursor.getString(faviconGuidCol);
                        }

                        
                        
                        if (knownFolders.contains(parent)) {
                            try {
                                boolean isFolder = (type == PLACES_TYPE_FOLDER);
                                addBookmark(url, title, guid, parent,
                                            dateadded, datemodified,
                                            position, isFolder);
                                addFavicon(url, faviconUrl, faviconGuid,
                                           faviconMime, faviconDataBuff);
                                if (isFolder) {
                                    
                                    
                                    
                                    
                                    
                                    
                                    flushBatchOperations();
                                    long newFolderId = getFolderId(guid);
                                    
                                    mRerootMap.put(id, newFolderId);
                                    knownFolders.add(id);
                                    Log.d(LOGTAG, "Added folder: " + id);
                                }
                                processedBookmarks.add(id);
                            } catch (Exception e) {
                                Log.e(LOGTAG, "Error adding bookmark: ", e);
                            }
                            added++;
                        } else {
                            
                            openFolders.add(parent);
                            skipped++;
                        }
                        cursor.moveToNext();
                    }

                    
                    
                    
                    
                    boolean changed = openFolders.retainAll(knownFolders);

                    
                    
                    
                    if (openFolders.isEmpty() && changed) {
                        Log.w(LOGTAG, "Orphaned bookmarks found, not imported");
                    }
                    iterations++;
                    Log.i(LOGTAG, "Iteration = " + iterations + ", added " + added +
                          " bookmark(s), skipped " + skipped + " bookmark(s)");
                } while (!openFolders.isEmpty());

                cursor.close();
            } catch (SQLiteBridgeException e) {
                Log.e(LOGTAG, "Failed to get bookmarks: ", e);
                
                setMigratedBookmarks();
                return;
            }

            flushBatchOperations();
        }

        protected void flushBatchOperations() {
            Log.i(LOGTAG, "Flushing " + mOperations.size() + " DB operations");
            try {
                
                mCr.applyBatch(BrowserContract.AUTHORITY, mOperations);
            } catch (RemoteException e) {
                Log.e(LOGTAG, "Remote exception while updating db: ", e);
            } catch (OperationApplicationException e) {
                
                Log.i(LOGTAG, "Error while applying database updates: ", e);
            }
            mOperations.clear();
        }

        protected void migratePlaces(File aFile) {
            
            if (hasMigrationFinished()) {
                Log.i(LOGTAG, "Nothing to migrate, early exit.");
                return;
            }

            String dbPath = aFile.getPath() + "/places.sqlite";
            String dbPathWal = aFile.getPath() + "/places.sqlite-wal";
            String dbPathShm = aFile.getPath() + "/places.sqlite-shm";
            Log.i(LOGTAG, "Opening path: " + dbPath);

            File dbFile = new File(dbPath);
            if (!dbFile.exists()) {
                Log.i(LOGTAG, "No database");
                
                setMigratedBookmarks();
                setMigratedHistory();
                return;
            }
            File dbFileWal = new File(dbPathWal);
            File dbFileShm = new File(dbPathShm);

            SQLiteBridge db = null;
            GeckoAppShell.loadSQLiteLibs(mContext, mContext.getPackageResourcePath());
            try {
                db = new SQLiteBridge(dbPath);
                if (!checkPlacesSchema(db)) {
                    
                    setMigratedBookmarks();
                    setMigratedHistory();
                } else {
                    
                    if (mLongOperationStartCallback != null) {
                        mLongOperationStartCallback.run();
                        mLongOperationStartRun = true;
                    }

                    calculateReroot(db);

                    if (!areBookmarksMigrated()) {
                        migrateBookmarks(db);
                        setMigratedBookmarks();
                    } else {
                        Log.i(LOGTAG, "Bookmarks already migrated. Skipping...");
                    }

                    if (!isHistoryMigrated()) {
                        migrateHistory(db);
                    } else {
                        Log.i(LOGTAG, "History already migrated. Skipping...");
                    }
                }

                db.close();

                
                
                if (isHistoryMigrated()) {
                    Log.i(LOGTAG, "Profile Migration has processed all entries. "
                          +" Purging old DB.");
                    dbFile.delete();
                    dbFileWal.delete();
                    dbFileShm.delete();
                }

                Log.i(LOGTAG, "Profile Migration run finished");
            } catch (SQLiteBridgeException e) {
                if (db != null) {
                    db.close();
                }
                Log.e(LOGTAG, "Error on places database:", e);
            } finally {
                if (mLongOperationStopCallback != null) {
                    if (mLongOperationStartRun) {
                        mLongOperationStopCallback.run();
                    }
                }
            }
        }

        @Override
        public void run() {
            migratePlaces(mProfileDir);
        }
    }
}
