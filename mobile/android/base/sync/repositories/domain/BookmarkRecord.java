



package org.mozilla.gecko.sync.repositories.domain;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Map;

import org.json.simple.JSONArray;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;






public class BookmarkRecord extends Record {
  public static final String PLACES_URI_PREFIX = "places:";

  private static final String LOG_TAG = "BookmarkRecord";

  public static final String COLLECTION_NAME = "bookmarks";
  public static final long BOOKMARKS_TTL = -1; 

  public BookmarkRecord(String guid, String collection, long lastModified, boolean deleted) {
    super(guid, collection, lastModified, deleted);
    this.ttl = BOOKMARKS_TTL;
  }
  public BookmarkRecord(String guid, String collection, long lastModified) {
    this(guid, collection, lastModified, false);
  }
  public BookmarkRecord(String guid, String collection) {
    this(guid, collection, 0, false);
  }
  public BookmarkRecord(String guid) {
    this(guid, COLLECTION_NAME, 0, false);
  }
  public BookmarkRecord() {
    this(Utils.generateGuid(), COLLECTION_NAME, 0, false);
  }

  
  
  public String  title;
  public String  bookmarkURI;
  public String  description;
  public String  keyword;
  public String  parentID;
  public String  parentName;
  public long    androidParentID;
  public String  type;
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
    out.ttl       = this.ttl;

    
    out.title           = this.title;
    out.bookmarkURI     = this.bookmarkURI;
    out.description     = this.description;
    out.keyword         = this.keyword;
    out.parentID        = this.parentID;
    out.parentName      = this.parentName;
    out.androidParentID = this.androidParentID;
    out.type            = this.type;
    out.androidPosition = this.androidPosition;

    out.children        = this.copyChildren();
    out.tags            = this.copyTags();

    return out;
  }

  public boolean isBookmark() {
    if (type == null) {
      return false;
    }
    return type.equals("bookmark");
  }

  public boolean isFolder() {
    if (type == null) {
      return false;
    }
    return type.equals("folder");
  }

  public boolean isLivemark() {
    if (type == null) {
      return false;
    }
    return type.equals("livemark");
  }

  public boolean isSeparator() {
    if (type == null) {
      return false;
    }
    return type.equals("separator");
  }

  public boolean isMicrosummary() {
    if (type == null) {
      return false;
    }
    return type.equals("microsummary");
  }

  public boolean isQuery() {
    if (type == null) {
      return false;
    }
    return type.equals("query");
  }

  



  private boolean isBookmarkIsh() {
    if (type == null) {
      return false;
    }
    return type.equals("bookmark") ||
           type.equals("microsummary") ||
           type.equals("query");
  }

  @Override
  protected void initFromPayload(ExtendedJSONObject payload) {
    this.type        = payload.getString("type");
    this.title       = payload.getString("title");
    this.description = payload.getString("description");
    this.parentID    = payload.getString("parentid");
    this.parentName  = payload.getString("parentName");

    if (isFolder()) {
      try {
        this.children = payload.getArray("children");
      } catch (NonArrayJSONException e) {
        Logger.error(LOG_TAG, "Got non-array children in bookmark record " + this.guid, e);
        
        this.children = new JSONArray();
      }
      return;
    }

    final String bmkUri = payload.getString("bmkUri");

    
    if (isBookmarkIsh()) {
      this.keyword = payload.getString("keyword");
      try {
        this.tags = payload.getArray("tags");
      } catch (NonArrayJSONException e) {
        Logger.warn(LOG_TAG, "Got non-array tags in bookmark record " + this.guid, e);
        this.tags = new JSONArray();
      }
    }

    if (isBookmark()) {
      this.bookmarkURI = bmkUri;
      return;
    }

    if (isLivemark()) {
      String siteUri = payload.getString("siteUri");
      String feedUri = payload.getString("feedUri");
      this.bookmarkURI = encodeUnsupportedTypeURI(bmkUri,
                                                  "siteUri", siteUri,
                                                  "feedUri", feedUri);
      return;
    }
    if (isQuery()) {
      String queryId = payload.getString("queryId");
      String folderName = payload.getString("folderName");
      this.bookmarkURI = encodeUnsupportedTypeURI(bmkUri,
                                                  "queryId", queryId,
                                                  "folderName", folderName);
      return;
    }
    if (isMicrosummary()) {
      String generatorUri = payload.getString("generatorUri");
      String staticTitle = payload.getString("staticTitle");
      this.bookmarkURI = encodeUnsupportedTypeURI(bmkUri,
                                                  "generatorUri", generatorUri,
                                                  "staticTitle", staticTitle);
      return;
    }
    if (isSeparator()) {
      Object p = payload.get("pos");
      if (p instanceof Long) {
        this.androidPosition = (Long) p;
      } else if (p instanceof String) {
        try {
          this.androidPosition = Long.parseLong((String) p, 10);
        } catch (NumberFormatException e) {
          return;
        }
      } else {
        Logger.warn(LOG_TAG, "Unsupported position value " + p);
        return;
      }
      String pos = String.valueOf(this.androidPosition);
      this.bookmarkURI = encodeUnsupportedTypeURI(null, "pos", pos, null, null);
      return;
    }
  }

  @Override
  protected void populatePayload(ExtendedJSONObject payload) {
    putPayload(payload, "type", this.type);
    putPayload(payload, "title", this.title);
    putPayload(payload, "description", this.description);
    putPayload(payload, "parentid", this.parentID);
    putPayload(payload, "parentName", this.parentName);
    putPayload(payload, "keyword", this.keyword);

    if (isFolder()) {
      payload.put("children", this.children);
      return;
    }

    
    if (isBookmarkIsh()) {
      if (isBookmark()) {
        payload.put("bmkUri", bookmarkURI);
      }

      if (isQuery()) {
        Map<String, String> parts = Utils.extractURIComponents(PLACES_URI_PREFIX, this.bookmarkURI);
        putPayload(payload, "queryId", parts.get("queryId"), true);
        putPayload(payload, "folderName", parts.get("folderName"), true);
        putPayload(payload, "bmkUri", parts.get("uri"));
        return;
      }

      if (this.tags != null) {
        payload.put("tags", this.tags);
      }

      putPayload(payload, "keyword", this.keyword);
      return;
    }

    if (isLivemark()) {
      Map<String, String> parts = Utils.extractURIComponents(PLACES_URI_PREFIX, this.bookmarkURI);
      putPayload(payload, "siteUri", parts.get("siteUri"));
      putPayload(payload, "feedUri", parts.get("feedUri"));
      return;
    }
    if (isMicrosummary()) {
      Map<String, String> parts = Utils.extractURIComponents(PLACES_URI_PREFIX, this.bookmarkURI);
      putPayload(payload, "generatorUri", parts.get("generatorUri"));
      putPayload(payload, "staticTitle", parts.get("staticTitle"));
      return;
    }
    if (isSeparator()) {
      Map<String, String> parts = Utils.extractURIComponents(PLACES_URI_PREFIX, this.bookmarkURI);
      String pos = parts.get("pos");
      if (pos == null) {
        return;
      }
      try {
        payload.put("pos", Long.parseLong(pos, 10));
      } catch (NumberFormatException e) {
        return;
      }
      return;
    }
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

    if (!RepoUtils.stringsEqual(this.type, other.type)) {
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

  






  protected static String encode(String in) {
    if (in == null) {
      return "";
    }
    try {
      return URLEncoder.encode(in, "UTF-8");
    } catch (UnsupportedEncodingException e) {
      
      return null;
    }
  }

  






  protected static String encodeUnsupportedTypeURI(String originalURI, String p1, String v1, String p2, String v2) {
    StringBuilder b = new StringBuilder(PLACES_URI_PREFIX);
    boolean previous = false;
    if (originalURI != null) {
      b.append("uri=");
      b.append(encode(originalURI));
      previous = true;
    }
    if (p1 != null && v1 != null) {
      if (previous) {
        b.append("&");
      }
      b.append(p1);
      b.append("=");
      b.append(encode(v1));
      previous = true;
    }
    if (p2 != null && v2 != null) {
      if (previous) {
        b.append("&");
      }
      b.append(p2);
      b.append("=");
      b.append(encode(v2));
      previous = true;
    }
    return b.toString();
  }
}












































