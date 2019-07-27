



package org.mozilla.gecko.db;

import android.annotation.TargetApi;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteStatement;
import android.os.Build;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoProfile;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.mozglue.RobocopTarget;

import java.util.Map;

public class DBUtils {
    private static final String LOGTAG = "GeckoDBUtils";

    public static final String qualifyColumn(String table, String column) {
        return table + "." + column;
    }

    
    
    public static String concatenateWhere(String a, String b) {
        if (TextUtils.isEmpty(a)) {
            return b;
        }

        if (TextUtils.isEmpty(b)) {
            return a;
        }

        return "(" + a + ") AND (" + b + ")";
    }

    
    
    public static String[] appendSelectionArgs(String[] originalValues, String[] newValues) {
        if (originalValues == null || originalValues.length == 0) {
            return newValues;
        }

        if (newValues == null || newValues.length == 0) {
            return originalValues;
        }

        String[] result = new String[originalValues.length + newValues.length];
        System.arraycopy(originalValues, 0, result, 0, originalValues.length);
        System.arraycopy(newValues, 0, result, originalValues.length, newValues.length);

        return result;
    }

    public static void replaceKey(ContentValues aValues, String aOriginalKey,
                                  String aNewKey, String aDefault) {
        String value = aDefault;
        if (aOriginalKey != null && aValues.containsKey(aOriginalKey)) {
            value = aValues.get(aOriginalKey).toString();
            aValues.remove(aOriginalKey);
        }

        if (!aValues.containsKey(aNewKey)) {
            aValues.put(aNewKey, value);
        }
    }

    private static String HISTOGRAM_DATABASE_LOCKED = "DATABASE_LOCKED_EXCEPTION";
    private static String HISTOGRAM_DATABASE_UNLOCKED = "DATABASE_SUCCESSFUL_UNLOCK";
    public static void ensureDatabaseIsNotLocked(SQLiteOpenHelper dbHelper, String databasePath) {
        final int maxAttempts = 5;
        int attempt = 0;
        SQLiteDatabase db = null;
        for (; attempt < maxAttempts; attempt++) {
            try {
                
                db = dbHelper.getWritableDatabase();
                break;
            } catch (Exception e) {
                
                
                Telemetry.addToHistogram(HISTOGRAM_DATABASE_LOCKED, attempt);

                
                Log.d(LOGTAG, "Database is locked, trying to kill any zombie processes: " + databasePath);
                GeckoAppShell.killAnyZombies();
                try {
                    Thread.sleep(attempt * 100);
                } catch (InterruptedException ie) {
                }
            }
        }

        if (db == null) {
            Log.w(LOGTAG, "Failed to unlock database.");
            GeckoAppShell.listOfOpenFiles();
            return;
        }

        
        
        if (attempt > 1) {
            Telemetry.addToHistogram(HISTOGRAM_DATABASE_UNLOCKED, attempt - 1);
        }
    }

    




    public static void stripEmptyByteArray(ContentValues values, String columnName) {
        if (values.containsKey(columnName)) {
            byte[] data = values.getAsByteArray(columnName);
            if (data == null || data.length == 0) {
                Log.w(LOGTAG, "Tried to insert an empty or non-byte-array image. Ignoring.");
                values.putNull(columnName);
            }
        }
    }

    






    public static String computeSQLInClause(int items, String field) {
        final StringBuilder builder = new StringBuilder(field);
        builder.append(" IN (");
        int i = 0;
        for (; i < items - 1; ++i) {
            builder.append("?, ");
        }
        if (i < items) {
            builder.append("?");
        }
        builder.append(")");
        return builder.toString();
    }

    




    public static String computeSQLInClauseFromLongs(final Cursor cursor, String field) {
        final StringBuilder builder = new StringBuilder(field);
        builder.append(" IN (");
        final int commaLimit = cursor.getCount() - 1;
        int i = 0;
        while (cursor.moveToNext()) {
            builder.append(cursor.getLong(0));
            if (i++ < commaLimit) {
                builder.append(", ");
            }
        }
        builder.append(")");
        return builder.toString();
    }

    public static Uri appendProfile(final String profile, final Uri uri) {
        return uri.buildUpon().appendQueryParameter(BrowserContract.PARAM_PROFILE, profile).build();
    }

    public static Uri appendProfileWithDefault(final String profile, final Uri uri) {
        if (TextUtils.isEmpty(profile)) {
            return appendProfile(GeckoProfile.DEFAULT_PROFILE, uri);
        }
        return appendProfile(profile, uri);
    }

    


    private static final int CONFLICT_NONE = 0;
    private static final String[] CONFLICT_VALUES = new String[] {"", " OR ROLLBACK ", " OR ABORT ", " OR FAIL ", " OR IGNORE ", " OR REPLACE "};

    












    @RobocopTarget
    public static int updateArrays(SQLiteDatabase db, String table, ContentValues[] values, UpdateOperation[] ops, String whereClause, String[] whereArgs) {
        return updateArraysWithOnConflict(db, table, values, ops, whereClause, whereArgs, CONFLICT_NONE, true);
    }

    public static void updateArraysBlindly(SQLiteDatabase db, String table, ContentValues[] values, UpdateOperation[] ops, String whereClause, String[] whereArgs) {
        updateArraysWithOnConflict(db, table, values, ops, whereClause, whereArgs, CONFLICT_NONE, false);
    }

    @RobocopTarget
    public enum UpdateOperation {
        




        ASSIGN,

        




        BITWISE_OR,

        







        EXPRESSION,
    }

    





    private static int updateArraysWithOnConflict(SQLiteDatabase db, String table,
                                          ContentValues[] values,
                                          UpdateOperation[] ops,
                                          String whereClause,
                                          String[] whereArgs,
                                          int conflictAlgorithm,
                                          boolean returnChangedRows) {
        if (values == null || values.length == 0) {
            throw new IllegalArgumentException("Empty values");
        }

        if (ops == null || ops.length != values.length) {
            throw new IllegalArgumentException("ops and values don't match");
        }

        StringBuilder sql = new StringBuilder(120);
        sql.append("UPDATE ");
        sql.append(CONFLICT_VALUES[conflictAlgorithm]);
        sql.append(table);
        sql.append(" SET ");

        
        int setValuesSize = 0;
        for (int i = 0; i < values.length; i++) {
            
            if (ops[i] != UpdateOperation.EXPRESSION) {
                setValuesSize += values[i].size();
            }
        }

        int bindArgsSize = (whereArgs == null) ? setValuesSize : (setValuesSize + whereArgs.length);
        Object[] bindArgs = new Object[bindArgsSize];

        int arg = 0;
        for (int i = 0; i < values.length; i++) {
            final ContentValues v = values[i];
            final UpdateOperation op = ops[i];

            
            switch (op) {
                case ASSIGN:
                    for (Map.Entry<String, Object> entry : v.valueSet()) {
                        final String colName = entry.getKey();
                        sql.append((arg > 0) ? "," : "");
                        sql.append(colName);
                        bindArgs[arg++] = entry.getValue();
                        sql.append("= ?");
                    }
                    break;
                case BITWISE_OR:
                    for (Map.Entry<String, Object> entry : v.valueSet()) {
                        final String colName = entry.getKey();
                        sql.append((arg > 0) ? "," : "");
                        sql.append(colName);
                        bindArgs[arg++] = entry.getValue();
                        sql.append("= ? | ");
                        sql.append(colName);
                    }
                    break;
                case EXPRESSION:
                    
                    for (Map.Entry<String, Object> entry : v.valueSet()) {
                        final String colName = entry.getKey();
                        sql.append((arg > 0) ? "," : "");
                        sql.append(colName);
                        sql.append(" = ");
                        sql.append(entry.getValue());
                    }
                    break;
            }
        }

        if (whereArgs != null) {
            for (arg = setValuesSize; arg < bindArgsSize; arg++) {
                bindArgs[arg] = whereArgs[arg - setValuesSize];
            }
        }
        if (!TextUtils.isEmpty(whereClause)) {
            sql.append(" WHERE ");
            sql.append(whereClause);
        }

        
        
        
        final SQLiteStatement statement = db.compileStatement(sql.toString());
        try {
            bindAllArgs(statement, bindArgs);
            if (!returnChangedRows) {
                statement.execute();
                return 0;
            }

            if (AppConstants.Versions.feature11Plus) {
                
                return executeStatementReturningChangedRows(statement);
            } else {
                statement.execute();
                final Cursor cursor = db.rawQuery("SELECT changes()", null);
                try {
                    cursor.moveToFirst();
                    return cursor.getInt(0);
                } finally {
                    cursor.close();
                }

            }
        } finally {
            statement.close();
        }
    }

    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    private static int executeStatementReturningChangedRows(SQLiteStatement statement) {
        return statement.executeUpdateDelete();
    }

    
    private static void bindAllArgs(SQLiteStatement statement, Object[] bindArgs) {
        if (bindArgs == null) {
            return;
        }
        for (int i = bindArgs.length; i != 0; i--) {
            Object v = bindArgs[i - 1];
            if (v == null) {
                statement.bindNull(i);
            } else if (v instanceof String) {
                statement.bindString(i, (String) v);
            } else if (v instanceof Double) {
                statement.bindDouble(i, (Double) v);
            } else if (v instanceof Float) {
                statement.bindDouble(i, (Float) v);
            } else if (v instanceof Long) {
                statement.bindLong(i, (Long) v);
            } else if (v instanceof Integer) {
                statement.bindLong(i, (Integer) v);
            } else if (v instanceof Byte) {
                statement.bindLong(i, (Byte) v);
            } else if (v instanceof byte[]) {
                statement.bindBlob(i, (byte[]) v);
            }
        }
    }
}
