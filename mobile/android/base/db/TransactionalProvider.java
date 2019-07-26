



package org.mozilla.gecko.db;

import org.mozilla.gecko.db.PerProfileDatabases.DatabaseHelperFactory;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;






public abstract class TransactionalProvider<T extends SQLiteOpenHelper> extends ContentProvider {
    private static final String LOGTAG = "GeckoTransProvider";
    protected Context mContext;
    protected PerProfileDatabases<T> mDatabases;

    





    abstract protected String getDatabaseName();

    







    abstract protected T createDatabaseHelper(Context context, String databasePath);

    






    abstract protected Uri insertInTransaction(Uri uri, ContentValues values);

    







    abstract protected int deleteInTransaction(Uri uri, String selection, String[] selectionArgs);

    








    abstract protected int updateInTransaction(Uri uri, ContentValues values, String selection, String[] selectionArgs);

    







    protected SQLiteDatabase getReadableDatabase(Uri uri) {
        String profile = null;
        if (uri != null) {
            profile = uri.getQueryParameter(BrowserContract.PARAM_PROFILE);
        }

        return mDatabases.getDatabaseHelperForProfile(profile, isTest(uri)).getReadableDatabase();
    }

    







    protected SQLiteDatabase getWritableDatabase(Uri uri) {
        String profile = null;
        if (uri != null) {
            profile = uri.getQueryParameter(BrowserContract.PARAM_PROFILE);
        }

        return mDatabases.getDatabaseHelperForProfile(profile, isTest(uri)).getWritableDatabase();
    }

    protected SQLiteDatabase getWritableDatabaseForProfile(String profile, boolean isTest) {
        return mDatabases.getDatabaseHelperForProfile(profile, isTest).getWritableDatabase();
    }

    @Override
    public boolean onCreate() {
        synchronized (this) {
            mContext = getContext();
            mDatabases = new PerProfileDatabases<T>(
                getContext(), getDatabaseName(), new DatabaseHelperFactory<T>() {
                    @Override
                    public T makeDatabaseHelper(Context context, String databasePath) {
                        return createDatabaseHelper(context, databasePath);
                    }
                });
        }

        return true;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        trace("Calling delete on URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);
        int deleted = 0;

        if (Build.VERSION.SDK_INT >= 11) {
            trace("Beginning delete transaction: " + uri);
            db.beginTransaction();
            try {
                deleted = deleteInTransaction(uri, selection, selectionArgs);
                db.setTransactionSuccessful();
                trace("Successful delete transaction: " + uri);
            } finally {
                db.endTransaction();
            }
        } else {
            deleted = deleteInTransaction(uri, selection, selectionArgs);
        }

        if (deleted > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
        }

        return deleted;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        trace("Calling insert on URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);
        Uri result = null;
        try {
            if (Build.VERSION.SDK_INT >= 11) {
                trace("Beginning insert transaction: " + uri);
                db.beginTransaction();
                try {
                    result = insertInTransaction(uri, values);
                    db.setTransactionSuccessful();
                    trace("Successful insert transaction: " + uri);
                } finally {
                    db.endTransaction();
                }
            } else {
                result = insertInTransaction(uri, values);
            }
        } catch (SQLException sqle) {
            Log.e(LOGTAG, "exception in DB operation", sqle);
        } catch (UnsupportedOperationException uoe) {
            Log.e(LOGTAG, "don't know how to perform that insert", uoe);
        }

        if (result != null) {
            getContext().getContentResolver().notifyChange(uri, null);
        }

        return result;
    }


    @Override
    public int update(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        trace("Calling update on URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);
        int updated = 0;

        if (Build.VERSION.SDK_INT >= 11) {
            trace("Beginning update transaction: " + uri);
            db.beginTransaction();
            try {
                updated = updateInTransaction(uri, values, selection, selectionArgs);
                db.setTransactionSuccessful();
                trace("Successful update transaction: " + uri);
            } finally {
                db.endTransaction();
            }
        } else {
            updated = updateInTransaction(uri, values, selection, selectionArgs);
        }

        if (updated > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
        }

        return updated;
    }

    @Override
    public int bulkInsert(Uri uri, ContentValues[] values) {
        if (values == null) {
            return 0;
        }

        int numValues = values.length;
        int successes = 0;

        final SQLiteDatabase db = getWritableDatabase(uri);

        db.beginTransaction();

        try {
            for (int i = 0; i < numValues; i++) {
                insertInTransaction(uri, values[i]);
                successes++;
            }
            trace("Flushing DB bulkinsert...");
            db.setTransactionSuccessful();
        } finally {
            db.endTransaction();
        }

        if (successes > 0) {
            mContext.getContentResolver().notifyChange(uri, null);
        }

        return successes;
    }

    protected boolean isTest(Uri uri) {
        String isTest = uri.getQueryParameter(BrowserContract.PARAM_IS_TEST);
        return !TextUtils.isEmpty(isTest);
    }

    
    
    private static boolean logDebug  = Log.isLoggable(LOGTAG, Log.DEBUG);
    private static boolean logVerbose = Log.isLoggable(LOGTAG, Log.VERBOSE);
    protected static void trace(String message) {
        if (logVerbose) {
            Log.v(LOGTAG, message);
        }
    }

    protected static void debug(String message) {
        if (logDebug) {
            Log.d(LOGTAG, message);
        }
    }
}
