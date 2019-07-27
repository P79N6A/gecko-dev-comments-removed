



package org.mozilla.gecko.db;

import org.mozilla.gecko.AppConstants.Versions;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

































@SuppressWarnings("javadoc")
public abstract class AbstractTransactionalProvider extends ContentProvider {
    private static final String LOGTAG = "GeckoTransProvider";

    private static boolean logDebug = Log.isLoggable(LOGTAG, Log.DEBUG);
    private static boolean logVerbose = Log.isLoggable(LOGTAG, Log.VERBOSE);

    protected abstract SQLiteDatabase getReadableDatabase(Uri uri);
    protected abstract SQLiteDatabase getWritableDatabase(Uri uri);

    public abstract SQLiteDatabase getWritableDatabaseForTesting(Uri uri);

    protected abstract Uri insertInTransaction(Uri uri, ContentValues values);
    protected abstract int deleteInTransaction(Uri uri, String selection, String[] selectionArgs);
    protected abstract int updateInTransaction(Uri uri, ContentValues values, String selection, String[] selectionArgs);

    
























    final ThreadLocal<Boolean> isInBatchOperation = new ThreadLocal<Boolean>();

    



    @SuppressWarnings("static-method")
    protected boolean shouldUseTransactions() {
        return Versions.feature11Plus;
    }

    private boolean isInBatch() {
        final Boolean isInBatch = isInBatchOperation.get();
        if (isInBatch == null) {
            return false;
        }
        return isInBatch.booleanValue();
    }

    


    protected void beginWrite(final SQLiteDatabase db) {
        if (isInBatch()) {
            trace("Not bothering with an intermediate write transaction: inside batch operation.");
            return;
        }

        if (shouldUseTransactions() && !db.inTransaction()) {
            trace("beginWrite: beginning transaction.");
            db.beginTransaction();
        }
    }

    



    protected void markWriteSuccessful(final SQLiteDatabase db) {
        if (isInBatch()) {
            trace("Not marking write successful: inside batch operation.");
            return;
        }

        if (shouldUseTransactions() && db.inTransaction()) {
            trace("Marking write transaction successful.");
            db.setTransactionSuccessful();
        }
    }

    





    protected void endWrite(final SQLiteDatabase db) {
        if (isInBatch()) {
            trace("Not ending write: inside batch operation.");
            return;
        }

        if (shouldUseTransactions() && db.inTransaction()) {
            trace("endWrite: ending transaction.");
            db.endTransaction();
        }
    }

    protected void beginBatch(final SQLiteDatabase db) {
        trace("Beginning batch.");
        isInBatchOperation.set(Boolean.TRUE);
        db.beginTransaction();
    }

    protected void markBatchSuccessful(final SQLiteDatabase db) {
        if (isInBatch()) {
            trace("Marking batch successful.");
            db.setTransactionSuccessful();
            return;
        }
        Log.w(LOGTAG, "Unexpectedly asked to mark batch successful, but not in batch!");
        throw new IllegalStateException("Not in batch.");
    }

    protected void endBatch(final SQLiteDatabase db) {
        trace("Ending batch.");
        db.endTransaction();
        isInBatchOperation.set(Boolean.FALSE);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        trace("Calling delete on URI: " + uri + ", " + selection + ", " + selectionArgs);

        final SQLiteDatabase db = getWritableDatabase(uri);
        int deleted = 0;

        try {
            deleted = deleteInTransaction(uri, selection, selectionArgs);
            markWriteSuccessful(db);
        } finally {
            endWrite(db);
        }

        if (deleted > 0) {
            final boolean shouldSyncToNetwork = !isCallerSync(uri);
            getContext().getContentResolver().notifyChange(uri, null, shouldSyncToNetwork);
        }

        return deleted;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        trace("Calling insert on URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);
        Uri result = null;
        try {
            result = insertInTransaction(uri, values);
            markWriteSuccessful(db);
        } catch (SQLException sqle) {
            Log.e(LOGTAG, "exception in DB operation", sqle);
        } catch (UnsupportedOperationException uoe) {
            Log.e(LOGTAG, "don't know how to perform that insert", uoe);
        } finally {
            endWrite(db);
        }

        if (result != null) {
            final boolean shouldSyncToNetwork = !isCallerSync(uri);
            getContext().getContentResolver().notifyChange(uri, null, shouldSyncToNetwork);
        }

        return result;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        trace("Calling update on URI: " + uri + ", " + selection + ", " + selectionArgs);

        final SQLiteDatabase db = getWritableDatabase(uri);
        int updated = 0;

        try {
            updated = updateInTransaction(uri, values, selection,
                                          selectionArgs);
            markWriteSuccessful(db);
        } finally {
            endWrite(db);
        }

        if (updated > 0) {
            final boolean shouldSyncToNetwork = !isCallerSync(uri);
            getContext().getContentResolver().notifyChange(uri, null, shouldSyncToNetwork);
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

        debug("bulkInsert: explicitly starting transaction.");
        beginBatch(db);

        try {
            for (int i = 0; i < numValues; i++) {
                insertInTransaction(uri, values[i]);
                successes++;
            }
            trace("Flushing DB bulkinsert...");
            markBatchSuccessful(db);
        } finally {
            debug("bulkInsert: explicitly ending transaction.");
            endBatch(db);
        }

        if (successes > 0) {
            final boolean shouldSyncToNetwork = !isCallerSync(uri);
            getContext().getContentResolver().notifyChange(uri, null, shouldSyncToNetwork);
        }

        return successes;
    }

    




    protected static boolean shouldShowDeleted(Uri uri) {
        String showDeleted = uri.getQueryParameter(BrowserContract.PARAM_SHOW_DELETED);
        return !TextUtils.isEmpty(showDeleted);
    }

    




    protected static boolean shouldUpdateOrInsert(Uri uri) {
        String insertIfNeeded = uri.getQueryParameter(BrowserContract.PARAM_INSERT_IF_NEEDED);
        return Boolean.parseBoolean(insertIfNeeded);
    }

    



    protected static boolean isTest(Uri uri) {
        if (uri == null) {
            return false;
        }
        String isTest = uri.getQueryParameter(BrowserContract.PARAM_IS_TEST);
        return !TextUtils.isEmpty(isTest);
    }

    



    protected static boolean isCallerSync(Uri uri) {
        String isSync = uri.getQueryParameter(BrowserContract.PARAM_IS_SYNC);
        return !TextUtils.isEmpty(isSync);
    }

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