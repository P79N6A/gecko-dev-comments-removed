


package org.mozilla.gecko.background.helpers;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import junit.framework.Assert;

public class DBHelpers {

  


  public static void assertCursorContains(Object[][] expected, Cursor actual) {
    Assert.assertEquals(expected.length, actual.getCount());
    int i = 0, j = 0;
    Object[] row;

    do {
      row = expected[i];
      for (j = 0; j < row.length; ++j) {
        Object atIndex = row[j];
        if (atIndex == null) {
          continue;
        }
        if (atIndex instanceof String) {
          Assert.assertEquals(atIndex, actual.getString(j));
        } else {
          Assert.assertEquals(atIndex, actual.getInt(j));
        }
      }
      ++i;
    } while (actual.moveToPosition(i));
  }

  public static int getRowCount(SQLiteDatabase db, String table) {
    return getRowCount(db, table, null, null);
  }

  public static int getRowCount(SQLiteDatabase db, String table, String selection, String[] selectionArgs) {
    final Cursor c = db.query(table, null, selection, selectionArgs, null, null, null);
    try {
      return c.getCount();
    } finally {
      c.close();
    }
  }

  



  public static int getNonExistentID(SQLiteDatabase db, String table) {
    
    
    final Cursor c = db.rawQuery("SELECT MAX(id) + 1 FROM " + table, null);
    try {
      if (!c.moveToNext()) {
        return 0;
      }
      return c.getInt(0);
    } finally {
      c.close();
    }
  }

  



  public static long getExistentID(SQLiteDatabase db, String table) {
    final Cursor c = db.query(table, new String[] {"id"}, null, null, null, null, null, "1");
    try {
      if (!c.moveToNext()) {
        throw new IllegalStateException("Given table does not contain any entries.");
      }
      return c.getInt(0);
    } finally {
      c.close();
    }
  }

}
