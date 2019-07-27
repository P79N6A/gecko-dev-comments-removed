



package org.mozilla.gecko.reading;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.reading.ReadingListRecord.ServerMetadata;
import org.mozilla.gecko.sync.ExtendedJSONObject;

import android.annotation.TargetApi;
import android.database.AbstractWindowedCursor;
import android.database.Cursor;
import android.database.CursorWindow;
import android.os.Build;













public class ReadingListClientRecordFactory {
  public static final int MAX_SERVER_STRING_CHARS = 1024;

  private final Cursor cursor;

  private final String[] fields;
  private final int[] columns;

  public ReadingListClientRecordFactory(final Cursor cursor, final String[] fields) throws IllegalArgumentException {
    this.cursor = cursor;

    
    final int idIndex = cursor.getColumnIndex(ReadingListItems._ID);
    final int extra = (idIndex != -1) ? 1 : 0;
    final int cols = fields.length + extra;

    this.fields = new String[cols];
    this.columns = new int[cols];

    for (int i = 0; i < fields.length; ++i) {
      final int index = cursor.getColumnIndex(fields[i]);
      if (index == -1) {
        continue;
      }
      this.fields[i] = mapColumn(fields[i]);
      this.columns[i] = index;
    }

    if (idIndex != -1) {
      this.fields[fields.length] = "_id";
      this.columns[fields.length] = idIndex;
    }
  }

  public ReadingListClientRecordFactory(final Cursor cursor) {
    this(cursor, ReadingListItems.ALL_FIELDS);
  }

  private void putNull(ExtendedJSONObject o, String field) {
    o.put(field, null);
  }

  


  private static String mapColumn(final String column) {
    switch (column) {
    case "is_unread":
      return "unread";
    case "is_favorite":
      return "favorite";
    case "is_archived":
      return "archived";
    }
    return column;
  }

  private void put(ExtendedJSONObject o, String field, String value) {
    
    o.put(field, value.length() > MAX_SERVER_STRING_CHARS ? value.substring(0, MAX_SERVER_STRING_CHARS - 1) + "â€¦" : value);
  }

  private void put(ExtendedJSONObject o, String field, long value) {
    
    switch (field) {
    case "unread":
    case "favorite":
    case "archived":
    case "is_article":
      o.put(field, value == 1);
      return;
    }
    o.put(field, value);
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private final void fillHoneycomb(ExtendedJSONObject o, Cursor c, String f, int i) {
    if (f == null) {
      return;
    }
    switch (c.getType(i)) {
    case Cursor.FIELD_TYPE_NULL:
      putNull(o, f);
      return;
    case Cursor.FIELD_TYPE_STRING:
      put(o, f, c.getString(i));
      return;
    case Cursor.FIELD_TYPE_INTEGER:
      put(o, f, c.getLong(i));
      return;
    case Cursor.FIELD_TYPE_FLOAT:
      o.put(f, c.getDouble(i));
      return;
    case Cursor.FIELD_TYPE_BLOB:
      
      o.put(f, c.getBlob(i));
      return;
    default:
      
      return;
    }
  }

  @SuppressWarnings("deprecation")
  private final void fillGingerbread(ExtendedJSONObject o, Cursor c, String f, int i) {
    if (!(c instanceof AbstractWindowedCursor)) {
      throw new IllegalStateException("Unable to handle cursors that don't have a CursorWindow!");
    }

    final AbstractWindowedCursor sqc = (AbstractWindowedCursor) c;
    final CursorWindow w = sqc.getWindow();
    final int pos = c.getPosition();
    if (w.isNull(pos, i)) {
      putNull(o, f);
    } else if (w.isString(pos, i)) {
      put(o, f, c.getString(i));
    } else if (w.isLong(pos, i)) {
      put(o, f, c.getLong(i));
    } else if (w.isFloat(pos, i)) {
      o.put(f, c.getDouble(i));
    } else if (w.isBlob(pos, i)) {
      
      o.put(f, c.getBlob(i));
    }
  }

  


  public ClientReadingListRecord fromCursorRow() {
    final ExtendedJSONObject object = new ExtendedJSONObject();
    for (int i = 0; i < this.fields.length; ++i) {
      final String field = fields[i];
      if (field == null) {
        continue;
      }
      final int column = this.columns[i];
      if (Versions.feature11Plus) {
        fillHoneycomb(object, this.cursor, field, column);
      } else {
        fillGingerbread(object, this.cursor, field, column);
      }
    }

    
    if (object.containsKey("unread") && object.getBoolean("unread")) {
      object.remove("marked_read_by");
      object.remove("marked_read_on");
    }

    
    final long serverLastModified = object.getLong("last_modified", -1L);
    final String guid = object.containsKey("guid") ? object.getString("guid") : null;
    final ServerMetadata sm = new ServerMetadata(guid, serverLastModified);

    final long clientLastModified = object.getLong("client_last_modified", -1L);

    
    final boolean isArchived = object.getBoolean("archived");

    
    final boolean isDeleted = object.getLong("is_deleted", 0L) == 1L;
    final long localID = object.getLong("_id", -1L);
    final ClientMetadata cm = new ClientMetadata(localID, clientLastModified, isDeleted, isArchived);

    
    object.remove("last_modified");
    object.remove("guid");
    object.remove("client_last_modified");
    object.remove("is_deleted");

    
    
    object.remove("stored_on");

    object.remove(ReadingListItems.CONTENT_STATUS);
    object.remove(ReadingListItems.SYNC_STATUS);
    object.remove(ReadingListItems.SYNC_CHANGE_FLAGS);
    object.remove(ReadingListItems.CLIENT_LAST_MODIFIED);

    return new ClientReadingListRecord(sm, cm, object);
  }

  




  public ClientReadingListRecord getNext() {
    if (!cursor.moveToNext()) {
      return null;
    }

    return fromCursorRow();
  }
}