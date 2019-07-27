



package org.mozilla.gecko.db;

import org.mozilla.gecko.GeckoAppShell;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteOpenHelper;
import android.text.TextUtils;
import android.util.Log;

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

    public static void ensureDatabaseIsNotLocked(SQLiteOpenHelper dbHelper, String databasePath) {
        for (int retries = 0; retries < 5; retries++) {
            try {
                
                dbHelper.getWritableDatabase();
                return;
            } catch (Exception e) {
                
                Log.d(LOGTAG, "Database is locked, trying to kill any zombie processes: " + databasePath);
                GeckoAppShell.killAnyZombies();
                try {
                    Thread.sleep(retries * 100);
                } catch (InterruptedException ie) { }
            }
        }
        Log.d(LOGTAG, "Failed to unlock database");
        GeckoAppShell.listOfOpenFiles();
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
}
