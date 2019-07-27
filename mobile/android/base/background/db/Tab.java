



package org.mozilla.gecko.background.db;

import org.json.simple.JSONArray;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.Tabs;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;

import android.content.ContentValues;
import android.database.Cursor;


public class Tab {
  public final String    title;
  public final String    icon;
  public final JSONArray history;
  public final long      lastUsed;

  public Tab(String title, String icon, JSONArray history, long lastUsed) {
    this.title    = title;
    this.icon     = icon;
    this.history  = history;
    this.lastUsed = lastUsed;
  }

  public ContentValues toContentValues(String clientGUID, int position) {
    ContentValues out = new ContentValues();
    out.put(BrowserContract.Tabs.POSITION,    position);
    out.put(BrowserContract.Tabs.CLIENT_GUID, clientGUID);

    out.put(BrowserContract.Tabs.FAVICON,   this.icon);
    out.put(BrowserContract.Tabs.LAST_USED, this.lastUsed);
    out.put(BrowserContract.Tabs.TITLE,     this.title);
    out.put(BrowserContract.Tabs.URL,       (String) this.history.get(0));
    out.put(BrowserContract.Tabs.HISTORY,   this.history.toJSONString());
    return out;
  }

  @Override
  public boolean equals(Object o) {
    if (!(o instanceof Tab)) {
      return false;
    }
    final Tab other = (Tab) o;

    if (!RepoUtils.stringsEqual(this.title, other.title)) {
      return false;
    }
    if (!RepoUtils.stringsEqual(this.icon, other.icon)) {
      return false;
    }

    if (!(this.lastUsed == other.lastUsed)) {
      return false;
    }

    return Utils.sameArrays(this.history, other.history);
  }

  @Override
  public int hashCode() {
    return super.hashCode();
  }

  








  public static Tab fromCursor(final Cursor cursor) {
    final String title = RepoUtils.getStringFromCursor(cursor, Tabs.TITLE);
    final String icon = RepoUtils.getStringFromCursor(cursor, Tabs.FAVICON);
    final JSONArray history = RepoUtils.getJSONArrayFromCursor(cursor, Tabs.HISTORY);
    final long lastUsed = RepoUtils.getLongFromCursor(cursor, Tabs.LAST_USED);

    return new Tab(title, icon, history, lastUsed);
  }
}
