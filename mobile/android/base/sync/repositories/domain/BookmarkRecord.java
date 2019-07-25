





































package org.mozilla.gecko.sync.repositories.domain;

import org.json.simple.JSONArray;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksDataAccessor;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;

import android.util.Log;






public class BookmarkRecord extends Record {
  private static final String LOG_TAG = "BookmarkRecord";

  public static final String COLLECTION_NAME = "bookmarks";

  public BookmarkRecord(String guid, String collection, long lastModified, boolean deleted) {
    super(guid, collection, lastModified, deleted);
  }
  public BookmarkRecord(String guid, String collection, long lastModified) {
    super(guid, collection, lastModified, false);
  }
  public BookmarkRecord(String guid, String collection) {
    super(guid, collection, 0, false);
  }
  public BookmarkRecord(String guid) {
    super(guid, COLLECTION_NAME, 0, false);
  }
  public BookmarkRecord() {
    super(Utils.generateGuid(), COLLECTION_NAME, 0, false);
  }

  
  
  public String  title;
  public String  bookmarkURI;
  public String  description;
  public String  keyword;
  public String  parentID;
  public String  parentName;
  public long    androidParentID;
  public String  type;
  public String  pos;
  public long    androidPosition;

  public JSONArray children;
  public JSONArray tags;

  @Override
  public String toString() {
    return "#<Bookmark " + guid + " (" + androidID + "), parent " +
           parentID + "/" + androidParentID + "/" + parentName + ">";
  }

  
  @SuppressWarnings("unchecked")
  protected JSONArray copyChildren() {
    if (this.children == null) {
      return null;
    }
    JSONArray children = new JSONArray();
    children.addAll(this.children);
    return children;
  }

  @SuppressWarnings("unchecked")
  protected JSONArray copyTags() {
    if (this.tags == null) {
      return null;
    }
    JSONArray tags = new JSONArray();
    tags.addAll(this.tags);
    return tags;
  }

  @Override
  public Record copyWithIDs(String guid, long androidID) {
    BookmarkRecord out = new BookmarkRecord(guid, this.collection, this.lastModified, this.deleted);
    out.androidID = androidID;
    out.sortIndex = this.sortIndex;

    
    out.title           = this.title;
    out.bookmarkURI     = this.bookmarkURI;
    out.description     = this.description;
    out.keyword         = this.keyword;
    out.parentID        = this.parentID;
    out.parentName      = this.parentName;
    out.androidParentID = this.androidParentID;
    out.type            = this.type;
    out.pos             = this.pos;
    out.androidPosition = this.androidPosition;

    out.children        = this.copyChildren();
    out.tags            = this.copyTags();

    return out;
  }

  @Override
  protected void initFromPayload(ExtendedJSONObject payload) {
    this.type        = (String) payload.get("type");
    this.title       = (String) payload.get("title");
    this.description = (String) payload.get("description");
    this.parentID    = (String) payload.get("parentid");
    this.parentName  = (String) payload.get("parentName");

    
    if (isBookmark()) {
      this.bookmarkURI = (String) payload.get("bmkUri");
      this.keyword     = (String) payload.get("keyword");
      try {
        this.tags = payload.getArray("tags");
      } catch (NonArrayJSONException e) {
        Log.e(LOG_TAG, "Got non-array tags in bookmark record " + this.guid, e);
        this.tags = new JSONArray();
      }
    }

    
    if (isFolder()) {
      try {
        this.children = payload.getArray("children");
      } catch (NonArrayJSONException e) {
        Log.e(LOG_TAG, "Got non-array children in bookmark record " + this.guid, e);
        
        this.children = new JSONArray();
      }
    }

    
    
    








  }

  @Override
  protected void populatePayload(ExtendedJSONObject payload) {
    putPayload(payload, "type", this.type);
    putPayload(payload, "title", this.title);
    putPayload(payload, "description", this.description);
    putPayload(payload, "parentid", this.parentID);
    putPayload(payload, "parentName", this.parentName);

    if (isBookmark()) {
      payload.put("bmkUri", bookmarkURI);
      payload.put("keyword", keyword);
      payload.put("tags", this.tags);
    } else if (isFolder()) {
      payload.put("children", this.children);
    }
  }

  public boolean isBookmark() {
    return AndroidBrowserBookmarksDataAccessor.TYPE_BOOKMARK.equalsIgnoreCase(this.type);
  }

  public boolean isFolder() {
    return AndroidBrowserBookmarksDataAccessor.TYPE_FOLDER.equalsIgnoreCase(this.type);
  }

  private void trace(String s) {
    Logger.trace(LOG_TAG, s);
  }

  @Override
  public boolean equalPayloads(Object o) {
    trace("Calling BookmarkRecord.equalPayloads.");
    if (o == null || !(o instanceof BookmarkRecord)) {
      return false;
    }

    BookmarkRecord other = (BookmarkRecord) o;
    if (!super.equalPayloads(other)) {
      return false;
    }

    
    if (isFolder() && (this.children != other.children)) {
      trace("BookmarkRecord.equals: this folder: " + this.title + ", " + this.guid);
      trace("BookmarkRecord.equals: other: " + other.title + ", " + other.guid);
      if (this.children  == null &&
          other.children != null) {
        trace("Records differ: one children array is null.");
        return false;
      }
      if (this.children  != null &&
          other.children == null) {
        trace("Records differ: one children array is null.");
        return false;
      }
      if (this.children.size() != other.children.size()) {
        trace("Records differ: children arrays differ in size (" +
              this.children.size() + " vs. " + other.children.size() + ").");
        return false;
      }

      for (int i = 0; i < this.children.size(); i++) {
        String child = (String) this.children.get(i);
        if (!other.children.contains(child)) {
          trace("Records differ: child " + child + " not found.");
          return false;
        }
      }
    }

    trace("Checking strings.");
    return RepoUtils.stringsEqual(this.title, other.title)
        && RepoUtils.stringsEqual(this.bookmarkURI, other.bookmarkURI)
        && RepoUtils.stringsEqual(this.parentID, other.parentID)
        && RepoUtils.stringsEqual(this.parentName, other.parentName)
        && RepoUtils.stringsEqual(this.type, other.type)
        && RepoUtils.stringsEqual(this.description, other.description)
        && RepoUtils.stringsEqual(this.keyword, other.keyword)
        && jsonArrayStringsEqual(this.tags, other.tags);
  }

  
  @Override
  public boolean congruentWith(Object o) {
    return this.equalPayloads(o) &&
           super.congruentWith(o);
  }

  
  
  
  private boolean jsonArrayStringsEqual(JSONArray a, JSONArray b) {
    
    if (a == b) return true;
    if (a == null && b != null) return false;
    if (a != null && b == null) return false;
    return RepoUtils.stringsEqual(a.toJSONString(), b.toJSONString());
  }
}












































