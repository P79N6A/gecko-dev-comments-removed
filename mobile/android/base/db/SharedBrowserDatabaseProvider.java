



package org.mozilla.gecko.db;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.db.BrowserContract.CommonColumns;
import org.mozilla.gecko.db.BrowserContract.SyncColumns;
import org.mozilla.gecko.db.PerProfileDatabases.DatabaseHelperFactory;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.util.Log;














public abstract class SharedBrowserDatabaseProvider extends AbstractPerProfileDatabaseProvider {
    private static final String LOGTAG = SharedBrowserDatabaseProvider.class.getSimpleName();

    private static PerProfileDatabases<BrowserDatabaseHelper> databases;

    @Override
    protected PerProfileDatabases<BrowserDatabaseHelper> getDatabases() {
        return databases;
    }

    @Override
    public boolean onCreate() {
        
        synchronized (SharedBrowserDatabaseProvider.class) {
            if (databases != null) {
                return true;
            }

            final DatabaseHelperFactory<BrowserDatabaseHelper> helperFactory = new DatabaseHelperFactory<BrowserDatabaseHelper>() {
                @Override
                public BrowserDatabaseHelper makeDatabaseHelper(Context context, String databasePath) {
                    final BrowserDatabaseHelper helper = new BrowserDatabaseHelper(context, databasePath);
                    if (Versions.feature16Plus) {
                        helper.setWriteAheadLoggingEnabled(true);
                    }
                    return helper;
                }
            };

            databases = new PerProfileDatabases<BrowserDatabaseHelper>(getContext(), BrowserDatabaseHelper.DATABASE_NAME, helperFactory);
        }

        return true;
    }

    










    protected void cleanUpSomeDeletedRecords(Uri fromUri, String tableName) {
        Log.d(LOGTAG, "Cleaning up deleted records from " + tableName);

        
        
        

        
        
        

        
        final long MAX_AGE_OF_DELETED_RECORDS = 86400000 * 20;

        
        final long DELETED_RECORDS_PURGE_LIMIT = 5;

        
        
        final long now = System.currentTimeMillis();
        final String selection = SyncColumns.IS_DELETED + " = 1 AND " +
                SyncColumns.DATE_MODIFIED + " <= " +
                (now - MAX_AGE_OF_DELETED_RECORDS);

        final String profile = fromUri.getQueryParameter(BrowserContract.PARAM_PROFILE);
        final SQLiteDatabase db = getWritableDatabaseForProfile(profile, isTest(fromUri));
        final String limit = Long.toString(DELETED_RECORDS_PURGE_LIMIT, 10);
        final Cursor cursor = db.query(tableName, new String[] { CommonColumns._ID }, selection, null, null, null, null, limit);
        final String inClause;
        try {
            inClause = DBUtils.computeSQLInClauseFromLongs(cursor, CommonColumns._ID);
        } finally {
            cursor.close();
        }

        db.delete(tableName, inClause, null);
    }
}
