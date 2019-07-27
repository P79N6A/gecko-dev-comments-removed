



package org.mozilla.gecko.reading;

import android.database.Cursor;

public interface ReadingListStorage {
  Cursor getModified();
  Cursor getStatusChanges();
  Cursor getNew();
  Cursor getAll();
  ReadingListChangeAccumulator getChangeAccumulator();
}
