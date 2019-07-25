



package org.mozilla.gecko.sqlite;

import org.mozilla.gecko.sqlite.SQLiteBridgeException;
import org.mozilla.gecko.sqlite.MatrixBlobCursor;
import android.content.ContentValues;
import android.database.Cursor;
import android.text.TextUtils;
import android.util.Log;

import java.lang.String;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;






public class SQLiteBridge {
    private static final String LOGTAG = "SQLiteBridge";

    
    private String mDb;

    
    private ArrayList<String> mColumns;
    private Long[] mQueryResults;

    
    private int kResultInsertRowId = 0;
    private int kResultRowsChanged = 1;

    
    private static native void sqliteCall(String aDb, String aQuery,
                                          String[] aParams,
                                          ArrayList<String> aColumns,
                                          Long[] aUpdateResult,
                                          ArrayList<Object[]> aRes)
        throws SQLiteBridgeException;

    
    public SQLiteBridge(String aDb) throws SQLiteBridgeException {
        mDb = aDb;
    }

    
    public void execSQL(String sql)
                throws SQLiteBridgeException {
        internalQuery(sql, null);
    }

    
    public void execSQL(String sql, String[] bindArgs)
                throws SQLiteBridgeException {
        internalQuery(sql, bindArgs);
    }

    
    public int delete(String table, String whereClause, String[] whereArgs)
               throws SQLiteBridgeException {
        StringBuilder sb = new StringBuilder("DELETE from ");
        sb.append(table);
        if (whereClause != null) {
            sb.append(" WHERE " + whereClause);
        }

        internalQuery(sb.toString(), whereArgs);
        return mQueryResults[kResultRowsChanged].intValue();
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

    public Cursor rawQuery(String sql, String[] selectionArgs)
        throws SQLiteBridgeException {
        ArrayList<Object[]> results;
        results = internalQuery(sql, selectionArgs);

        MatrixBlobCursor cursor =
            new MatrixBlobCursor(mColumns.toArray(new String[0]));
        try {
            for (Object resultRow: results) {
                Object[] resultColumns = (Object[])resultRow;
                if (resultColumns.length == mColumns.size())
                    cursor.addRow(resultColumns);
            }
        } catch(IllegalArgumentException ex) {
            Log.e(LOGTAG, "Error getting rows", ex);
        }

        return cursor;
    }

    public long insert(String table, String nullColumnHack, ContentValues values)
               throws SQLiteBridgeException {
        Set<Entry<String, Object>> valueSet = values.valueSet();
        Iterator<Entry<String, Object>> valueIterator = valueSet.iterator();
        ArrayList<String> valueNames = new ArrayList<String>();
        ArrayList<String> valueBinds = new ArrayList<String>();
        ArrayList<String> keyNames = new ArrayList<String>();

        while(valueIterator.hasNext()) {
            Entry<String, Object> value = valueIterator.next();
            keyNames.add(value.getKey());
            valueNames.add("?");
            valueBinds.add(value.getValue().toString());
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
        internalQuery(sb.toString(), binds);
        return mQueryResults[kResultInsertRowId];
    }

    public int update(String table, ContentValues values, String whereClause, String[] whereArgs)
               throws SQLiteBridgeException {
        Set<Entry<String, Object>> valueSet = values.valueSet();
        Iterator<Entry<String, Object>> valueIterator = valueSet.iterator();
        ArrayList<String> valueNames = new ArrayList<String>();

        StringBuilder sb = new StringBuilder("UPDATE ");
        sb.append(table);

        sb.append(" SET ");
        while(valueIterator.hasNext()) {
            Entry<String, Object> value = valueIterator.next();
            sb.append(value.getKey() + " = ?");
            valueNames.add(value.getValue().toString());
            if (valueIterator.hasNext())
                sb.append(", ");
            else
                sb.append(" ");
        }

        if (whereClause != null) {
            sb.append(" WHERE ");
            sb.append(whereClause);
            for (int i = 0; i < whereArgs.length; i++) {
                valueNames.add(whereArgs[i]);
            }
        }

        String[] binds = new String[valueNames.size()];
        valueNames.toArray(binds);

        internalQuery(sb.toString(), binds);
        return mQueryResults[kResultRowsChanged].intValue();
    }

    public int getVersion()
               throws SQLiteBridgeException {
        ArrayList<Object[]> results = null;
        results = internalQuery("PRAGMA user_version", null);
        int ret = -1;
        if (results != null) {
            for (Object resultRow: results) {
                Object[] resultColumns = (Object[])resultRow;
                String version = (String)resultColumns[0];
                ret = Integer.parseInt(version);
            }
        }
        return ret;
    }

    
    
    
    
    
    
    
    private ArrayList<Object[]> internalQuery(String aQuery, String[] aParams)
        throws SQLiteBridgeException {
        ArrayList<Object[]> result = new ArrayList<Object[]>();
        mQueryResults = new Long[2];
        mColumns = new ArrayList<String>();

        sqliteCall(mDb, aQuery, aParams, mColumns, mQueryResults, result);

        return result;
    }

    
    public void close() { }
}
