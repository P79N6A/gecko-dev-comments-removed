




































package org.mozilla.gecko.sqlite;

import org.mozilla.gecko.sqlite.SQLiteBridgeException;
import android.util.Log;

import java.lang.String;
import java.util.ArrayList;
import java.util.HashMap;






public class SQLiteBridge {
    private static final String LOGTAG = "SQLiteBridge";
    
    private String mDb;
    
    private ArrayList<String> mColumns;

    
    private static native void sqliteCall(String aDb, String aQuery,
                                          String[] aParams,
                                          ArrayList<String> aColumns,
                                          ArrayList<Object[]> aRes)
        throws SQLiteBridgeException;

    
    public SQLiteBridge(String aDb) throws SQLiteBridgeException {
        mDb = aDb;
    }

    
    public ArrayList<Object[]> query(String aQuery) throws SQLiteBridgeException {
        String[] params = new String[0];
        return query(aQuery, params);
    }

    
    
    
    
    
    
    
    public ArrayList<Object[]> query(String aQuery, String[] aParams)
        throws SQLiteBridgeException {
        ArrayList<Object[]> result = new ArrayList<Object[]>();
        mColumns = new ArrayList<String>();
        sqliteCall(mDb, aQuery, aParams, mColumns, result);
        return result;
    }

    
    
    public int getColumnIndex(String aColumnName) {
        return mColumns.lastIndexOf(aColumnName);
    }

    public void close() {
        
    }
}