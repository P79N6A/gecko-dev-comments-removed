



package org.mozilla.gecko.db;

import java.io.File;
import java.util.HashMap;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.sqlite.SQLiteBridge;
import org.mozilla.gecko.sqlite.SQLiteBridgeException;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;












public abstract class SQLiteBridgeContentProvider extends ContentProvider {
    private static final String ERROR_MESSAGE_DATABASE_IS_LOCKED = "Can't step statement: (5) database is locked";

    private HashMap<String, SQLiteBridge> mDatabasePerProfile;
    protected Context mContext;
    private final String mLogTag;

    protected SQLiteBridgeContentProvider(String logTag) {
        mLogTag = logTag;
    }

    





    protected abstract String getTelemetryPrefix();

    









    private static enum TelemetryErrorOp {
        BULKINSERT (0),
        DELETE     (1),
        INSERT     (2),
        QUERY      (3),
        UPDATE     (4);

        private final int bucket;

        TelemetryErrorOp(final int bucket) {
            this.bucket = bucket;
        }

        public int getBucket() {
            return bucket;
        }
    }

    @Override
    public void shutdown() {
        if (mDatabasePerProfile == null) {
            return;
        }

        synchronized (this) {
            for (SQLiteBridge bridge : mDatabasePerProfile.values()) {
                if (bridge != null) {
                    try {
                        bridge.close();
                    } catch (Exception ex) { }
                }
            }
            mDatabasePerProfile = null;
        }

        if (AppConstants.Versions.feature11Plus) {
            super.shutdown();
        }
    }

    @Override
    public void finalize() {
        shutdown();
    }

    



    public static boolean isCallerSync(Uri uri) {
        String isSync = uri.getQueryParameter(BrowserContract.PARAM_IS_SYNC);
        return !TextUtils.isEmpty(isSync);
    }

    private SQLiteBridge getDB(Context context, final String databasePath) {
        SQLiteBridge bridge = null;

        boolean dbNeedsSetup = true;
        try {
            String resourcePath = context.getPackageResourcePath();
            GeckoLoader.loadSQLiteLibs(context, resourcePath);
            GeckoLoader.loadNSSLibs(context, resourcePath);
            bridge = SQLiteBridge.openDatabase(databasePath, null, 0);
            int version = bridge.getVersion();
            dbNeedsSetup = version != getDBVersion();
        } catch (SQLiteBridgeException ex) {
            
            if (bridge != null) {
                bridge.close();
            }

            
            
            dbNeedsSetup = true;
            Log.e(mLogTag, "Error getting version ", ex);

            
            
            if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
                Log.e(mLogTag, "Can not set up database. Gecko is not running");
                return null;
            }
        }

        
        
        
        if (dbNeedsSetup) {
            bridge = null;
            initGecko();
        }
        return bridge;
    }
    
    








    private String getDatabasePathForProfile(String profile, String dbName) {
        
        File profileDir = GeckoProfile.get(mContext, profile).getDir();
        if (profileDir == null) {
            return null;
        }

        String databasePath = new File(profileDir, dbName).getAbsolutePath();
        return databasePath;
    }

    







    private SQLiteBridge getDatabaseForProfile(String profile) {
        if (TextUtils.isEmpty(profile)) {
            profile = GeckoProfile.get(mContext).getName();
            Log.d(mLogTag, "No profile provided, using '" + profile + "'");
        }

        final String dbName = getDBName();
        String mapKey = profile + "/" + dbName;

        SQLiteBridge db = null;
        synchronized (this) {
            db = mDatabasePerProfile.get(mapKey);
            if (db != null) {
                return db;
            }
            final String dbPath = getDatabasePathForProfile(profile, dbName);
            if (dbPath == null) {   
                Log.e(mLogTag, "Failed to get a valid db path for profile '" + profile + "'' dbName '" + dbName + "'");
                return null;
            }
            db = getDB(mContext, dbPath);
            if (db != null) {
                mDatabasePerProfile.put(mapKey, db);
            }
        }
        return db;
    }

    







    private SQLiteBridge getDatabaseForProfilePath(String profilePath) {
        File profileDir = new File(profilePath, getDBName());
        final String dbPath = profileDir.getPath();
        return getDatabaseForDBPath(dbPath);
    }

    







    private SQLiteBridge getDatabaseForDBPath(String dbPath) {
        SQLiteBridge db = null;
        synchronized (this) {
            db = mDatabasePerProfile.get(dbPath);
            if (db != null) {
                return db;
            }
            db = getDB(mContext, dbPath);
            if (db != null) {
                mDatabasePerProfile.put(dbPath, db);
            }
        }
        return db;
    }

    







    private SQLiteBridge getDatabase(Uri uri) {
        String profile = null;
        String profilePath = null;

        profile = uri.getQueryParameter(BrowserContract.PARAM_PROFILE);
        profilePath = uri.getQueryParameter(BrowserContract.PARAM_PROFILE_PATH);

        
        if (profilePath != null) {
            return getDatabaseForProfilePath(profilePath);
        }
        return getDatabaseForProfile(profile);
    }

    @Override
    public boolean onCreate() {
        mContext = getContext();
        synchronized (this) {
            mDatabasePerProfile = new HashMap<String, SQLiteBridge>();
        }
        return true;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        int deleted = 0;
        final SQLiteBridge db = getDatabase(uri);
        if (db == null) {
            return deleted;
        }

        try {
            deleted = db.delete(getTable(uri), selection, selectionArgs);
        } catch (SQLiteBridgeException ex) {
            reportError(ex, TelemetryErrorOp.DELETE);
            throw ex;
        }

        return deleted;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        long id = -1;
        final SQLiteBridge db = getDatabase(uri);

        
        
        
        if (db == null) {
            return null;
        }

        setupDefaults(uri, values);

        boolean useTransaction = !db.inTransaction();
        try {
            if (useTransaction) {
                db.beginTransaction();
            }

            
            
            onPreInsert(values, uri, db);
            id = db.insert(getTable(uri), null, values);

            if (useTransaction) {
                db.setTransactionSuccessful();
            }
        } catch (SQLiteBridgeException ex) {
            reportError(ex, TelemetryErrorOp.INSERT);
            throw ex;
        } finally {
            if (useTransaction) {
                db.endTransaction();
            }
        }

        return ContentUris.withAppendedId(uri, id);
    }

    @Override
    public int bulkInsert(Uri uri, ContentValues[] allValues) {
        final SQLiteBridge db = getDatabase(uri);
        
        
        
        if (db == null) {
            return 0;
        }

        int rowsAdded = 0;

        String table = getTable(uri);

        try {
            db.beginTransaction();
            for (ContentValues initialValues : allValues) {
                ContentValues values = new ContentValues(initialValues);
                setupDefaults(uri, values);
                onPreInsert(values, uri, db);
                db.insert(table, null, values);
                rowsAdded++;
            }
            db.setTransactionSuccessful();
        } catch (SQLiteBridgeException ex) {
            reportError(ex, TelemetryErrorOp.BULKINSERT);
            throw ex;
        } finally {
            db.endTransaction();
        }

        if (rowsAdded > 0) {
            final boolean shouldSyncToNetwork = !isCallerSync(uri);
            mContext.getContentResolver().notifyChange(uri, null, shouldSyncToNetwork);
        }

        return rowsAdded;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        int updated = 0;
        final SQLiteBridge db = getDatabase(uri);

        
        
        
        if (db == null) {
            return updated;
        }

        onPreUpdate(values, uri, db);

        try {
            updated = db.update(getTable(uri), values, selection, selectionArgs);
        } catch (SQLiteBridgeException ex) {
            reportError(ex, TelemetryErrorOp.UPDATE);
            throw ex;
        }

        return updated;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        Cursor cursor = null;
        final SQLiteBridge db = getDatabase(uri);

        
        
        
        if (db == null) {
            return cursor;
        }

        sortOrder = getSortOrder(uri, sortOrder);

        try {
            cursor = db.query(getTable(uri), projection, selection, selectionArgs, null, null, sortOrder, null);
            onPostQuery(cursor, uri, db);
        } catch (SQLiteBridgeException ex) {
            reportError(ex, TelemetryErrorOp.QUERY);
            throw ex;
        }

        return cursor;
    }

    private String getHistogram(SQLiteBridgeException e) {
        
        
        if (ERROR_MESSAGE_DATABASE_IS_LOCKED.equals(e.getMessage())) {
            return getTelemetryPrefix() + "_LOCKED";
        }
        return null;
    }

    protected void reportError(SQLiteBridgeException e, TelemetryErrorOp op) {
        Log.e(mLogTag, "Error in database " + op.name(), e);
        final String histogram = getHistogram(e);
        if (histogram == null) {
            return;
        }

        Telemetry.addToHistogram(histogram, op.getBucket());
    }

    protected abstract String getDBName();

    protected abstract int getDBVersion();

    protected abstract String getTable(Uri uri);

    protected abstract String getSortOrder(Uri uri, String aRequested);

    protected abstract void setupDefaults(Uri uri, ContentValues values);

    protected abstract void initGecko();

    protected abstract void onPreInsert(ContentValues values, Uri uri, SQLiteBridge db);

    protected abstract void onPreUpdate(ContentValues values, Uri uri, SQLiteBridge db);

    protected abstract void onPostQuery(Cursor cursor, Uri uri, SQLiteBridge db);
}
