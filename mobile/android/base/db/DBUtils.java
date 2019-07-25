



package org.mozilla.gecko.db;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.sync.Utils;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Build;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;
import java.util.UUID;

import java.util.Random;

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
        try {
            dbHelper.getWritableDatabase();
        } catch (Exception e) {
            Log.d(LOGTAG, "Database is locked, trying to forcefully unlock the database file: " + databasePath);

            
            GeckoAppShell.unlockDatabaseFile(databasePath);

            
            
            dbHelper.getWritableDatabase();

            
            
        }
    }
}
