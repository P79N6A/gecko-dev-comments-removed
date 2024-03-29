



package org.mozilla.gecko.sqlite;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.text.TextUtils;
import android.util.Log;

import org.mozilla.gecko.mozglue.RobocopTarget;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map.Entry;






public class SQLiteBridge {
    private static final String LOGTAG = "SQLiteBridge";

    
    private final String mDb;

    
    protected volatile long mDbPointer;

    
    private long[] mQueryResults;

    private boolean mTransactionSuccess;
    private boolean mInTransaction;

    private static final int RESULT_INSERT_ROW_ID = 0;
    private static final int RESULT_ROWS_CHANGED = 1;

    
    private static final int DEFAULT_PAGE_SIZE_BYTES = 32768;

    
    private static final int MAX_WAL_SIZE_BYTES = 524288;

    
    private static native MatrixBlobCursor sqliteCall(String aDb, String aQuery,
                                                      String[] aParams,
                                                      long[] aUpdateResult)
        throws SQLiteBridgeException;
    private static native MatrixBlobCursor sqliteCallWithDb(long aDb, String aQuery,
                                                            String[] aParams,
                                                            long[] aUpdateResult)
        throws SQLiteBridgeException;
    private static native long openDatabase(String aDb)
        throws SQLiteBridgeException;
    private static native void closeDatabase(long aDb);

    
    @RobocopTarget
    public SQLiteBridge(String aDb) throws SQLiteBridgeException {
        mDb = aDb;
    }

    
    public void execSQL(String sql)
                throws SQLiteBridgeException {
        Cursor cursor = internalQuery(sql, null);
        cursor.close();
    }

    
    public void execSQL(String sql, String[] bindArgs)
                throws SQLiteBridgeException {
        Cursor cursor = internalQuery(sql, bindArgs);
        cursor.close();
    }

    
    public int delete(String table, String whereClause, String[] whereArgs)
               throws SQLiteBridgeException {
        StringBuilder sb = new StringBuilder("DELETE from ");
        sb.append(table);
        if (whereClause != null) {
            sb.append(" WHERE " + whereClause);
        }

        execSQL(sb.toString(), whereArgs);
        return (int)mQueryResults[RESULT_ROWS_CHANGED];
    }

    public Cursor query(String table,
                        String[] columns,
                        String selection,
                        String[] selectionArgs,
                        String groupBy,
                        String having,
                        String orderBy,
                        String limit)
               throws SQLiteBridgeException {
        StringBuilder sb = new StringBuilder("SELECT ");
        if (columns != null)
            sb.append(TextUtils.join(", ", columns));
        else
            sb.append(" * ");

        sb.append(" FROM ");
        sb.append(table);

        if (selection != null) {
            sb.append(" WHERE " + selection);
        }

        if (groupBy != null) {
            sb.append(" GROUP BY " + groupBy);
        }

        if (having != null) {
            sb.append(" HAVING " + having);
        }

        if (orderBy != null) {
            sb.append(" ORDER BY " + orderBy);
        }

        if (limit != null) {
            sb.append(" " + limit);
        }

        return rawQuery(sb.toString(), selectionArgs);
    }

    @RobocopTarget
    public Cursor rawQuery(String sql, String[] selectionArgs)
        throws SQLiteBridgeException {
        return internalQuery(sql, selectionArgs);
    }

    public long insert(String table, String nullColumnHack, ContentValues values)
               throws SQLiteBridgeException {
        if (values == null)
            return 0;

        ArrayList<String> valueNames = new ArrayList<String>();
        ArrayList<String> valueBinds = new ArrayList<String>();
        ArrayList<String> keyNames = new ArrayList<String>();

        for (Entry<String, Object> value : values.valueSet()) {
            keyNames.add(value.getKey());

            Object val = value.getValue();
            if (val == null) {
                valueNames.add("NULL");
            } else {
                valueNames.add("?");
                valueBinds.add(val.toString());
            }
        }

        StringBuilder sb = new StringBuilder("INSERT into ");
        sb.append(table);

        sb.append(" (");
        sb.append(TextUtils.join(", ", keyNames));
        sb.append(")");

        
        sb.append(" VALUES (");
        sb.append(TextUtils.join(", ", valueNames));
        sb.append(") ");

        String[] binds = new String[valueBinds.size()];
        valueBinds.toArray(binds);
        execSQL(sb.toString(), binds);
        return mQueryResults[RESULT_INSERT_ROW_ID];
    }

    public int update(String table, ContentValues values, String whereClause, String[] whereArgs)
               throws SQLiteBridgeException {
        if (values == null)
            return 0;

        ArrayList<String> valueNames = new ArrayList<String>();

        StringBuilder sb = new StringBuilder("UPDATE ");
        sb.append(table);
        sb.append(" SET ");

        boolean isFirst = true;

        for (Entry<String, Object> value : values.valueSet()) {
            if (isFirst)
                isFirst = false;
            else
                sb.append(", ");

            sb.append(value.getKey());

            Object val = value.getValue();
            if (val == null) {
                sb.append(" = NULL");
            } else {
                sb.append(" = ?");
                valueNames.add(val.toString());
            }
        }

        if (!TextUtils.isEmpty(whereClause)) {
            sb.append(" WHERE ");
            sb.append(whereClause);
            valueNames.addAll(Arrays.asList(whereArgs));
        }

        String[] binds = new String[valueNames.size()];
        valueNames.toArray(binds);

        execSQL(sb.toString(), binds);
        return (int)mQueryResults[RESULT_ROWS_CHANGED];
    }

    public int getVersion()
               throws SQLiteBridgeException {
        Cursor cursor = internalQuery("PRAGMA user_version", null);
        int ret = -1;
        if (cursor != null) {
            cursor.moveToFirst();
            String version = cursor.getString(0);
            ret = Integer.parseInt(version);
            cursor.close();
        }
        return ret;
    }

    
    
    
    private Cursor internalQuery(String aQuery, String[] aParams)
        throws SQLiteBridgeException {

        mQueryResults = new long[2];
        if (isOpen()) {
            return sqliteCallWithDb(mDbPointer, aQuery, aParams, mQueryResults);
        }
        return sqliteCall(mDb, aQuery, aParams, mQueryResults);
    }

    



    public static SQLiteBridge openDatabase(String path, SQLiteDatabase.CursorFactory factory, int flags)
        throws SQLiteException {
        if (factory != null) {
            throw new RuntimeException("factory not supported.");
        }
        if (flags != 0) {
            throw new RuntimeException("flags not supported.");
        }

        SQLiteBridge bridge = null;
        try {
            bridge = new SQLiteBridge(path);
            bridge.mDbPointer = SQLiteBridge.openDatabase(path);
        } catch (SQLiteBridgeException ex) {
            
            throw new SQLiteException(ex.getMessage());
        }

        prepareWAL(bridge);

        return bridge;
    }

    public void close() {
        if (isOpen()) {
          closeDatabase(mDbPointer);
        }
        mDbPointer = 0L;
    }

    public boolean isOpen() {
        return mDbPointer != 0;
    }

    public void beginTransaction() throws SQLiteBridgeException {
        if (inTransaction()) {
            throw new SQLiteBridgeException("Nested transactions are not supported");
        }
        execSQL("BEGIN EXCLUSIVE");
        mTransactionSuccess = false;
        mInTransaction = true;
    }

    public void beginTransactionNonExclusive() throws SQLiteBridgeException {
        if (inTransaction()) {
            throw new SQLiteBridgeException("Nested transactions are not supported");
        }
        execSQL("BEGIN IMMEDIATE");
        mTransactionSuccess = false;
        mInTransaction = true;
    }

    public void endTransaction() {
        if (!inTransaction())
            return;

        try {
          if (mTransactionSuccess) {
              execSQL("COMMIT TRANSACTION");
          } else {
              execSQL("ROLLBACK TRANSACTION");
          }
        } catch(SQLiteBridgeException ex) {
            Log.e(LOGTAG, "Error ending transaction", ex);
        }
        mInTransaction = false;
        mTransactionSuccess = false;
    }

    public void setTransactionSuccessful() throws SQLiteBridgeException {
        if (!inTransaction()) {
            throw new SQLiteBridgeException("setTransactionSuccessful called outside a transaction");
        }
        mTransactionSuccess = true;
    }

    public boolean inTransaction() {
        return mInTransaction;
    }

    @Override
    public void finalize() {
        if (isOpen()) {
            Log.e(LOGTAG, "Bridge finalized without closing the database");
            close();
        }
    }

    private static void prepareWAL(final SQLiteBridge bridge) {
        
        
        
        final Cursor cursor = bridge.internalQuery("PRAGMA journal_mode=WAL", null);
        try {
            if (cursor.moveToFirst()) {
                String journalMode = cursor.getString(0);
                Log.d(LOGTAG, "Journal mode: " + journalMode);
                if ("wal".equals(journalMode)) {
                    
                    final int pageSizeBytes = bridge.getPageSizeBytes();
                    final int checkpointPageCount = MAX_WAL_SIZE_BYTES / pageSizeBytes;
                    bridge.execSQL("PRAGMA wal_autocheckpoint=" + checkpointPageCount);
                } else {
                    if (!"truncate".equals(journalMode)) {
                        Log.w(LOGTAG, "Unable to activate WAL journal mode. Using truncate instead.");
                        bridge.execSQL("PRAGMA journal_mode=TRUNCATE");
                    }
                    Log.w(LOGTAG, "Not using WAL mode: using synchronous=FULL instead.");
                    bridge.execSQL("PRAGMA synchronous=FULL");
                }
            }
        } finally {
            cursor.close();
        }
    }

    private int getPageSizeBytes() {
        if (!isOpen()) {
            throw new IllegalStateException("Database not open.");
        }

        final Cursor cursor = internalQuery("PRAGMA page_size", null);
        try {
            if (!cursor.moveToFirst()) {
                Log.w(LOGTAG, "Unable to retrieve page size.");
                return DEFAULT_PAGE_SIZE_BYTES;
            }

            return cursor.getInt(0);
        } finally {
            cursor.close();
        }
    }
}
