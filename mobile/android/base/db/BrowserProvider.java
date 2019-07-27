




package org.mozilla.gecko.db;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.FaviconColumns;
import org.mozilla.gecko.db.BrowserContract.Favicons;
import org.mozilla.gecko.db.BrowserContract.History;
import org.mozilla.gecko.db.BrowserContract.Schema;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.sync.Utils;

import android.app.SearchManager;
import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.OperationApplicationException;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.MatrixCursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

public class BrowserProvider extends SharedBrowserDatabaseProvider {
    private static final String LOGTAG = "GeckoBrowserProvider";

    
    
    
    
    static final int MAX_POSITION_UPDATES_PER_QUERY = 100;

    
    static final int DEFAULT_EXPIRY_RETAIN_COUNT = 2000;
    static final int AGGRESSIVE_EXPIRY_RETAIN_COUNT = 500;

    
    static final long DEFAULT_EXPIRY_PRESERVE_WINDOW = 1000L * 60L * 60L * 24L * 28L;     
    
    static final int DEFAULT_EXPIRY_THUMBNAIL_COUNT = 15;

    static final String TABLE_BOOKMARKS = Bookmarks.TABLE_NAME;
    static final String TABLE_HISTORY = History.TABLE_NAME;
    static final String TABLE_FAVICONS = Favicons.TABLE_NAME;
    static final String TABLE_THUMBNAILS = Thumbnails.TABLE_NAME;

    static final String VIEW_COMBINED = Combined.VIEW_NAME;
    static final String VIEW_BOOKMARKS_WITH_FAVICONS = Bookmarks.VIEW_WITH_FAVICONS;
    static final String VIEW_HISTORY_WITH_FAVICONS = History.VIEW_WITH_FAVICONS;
    static final String VIEW_COMBINED_WITH_FAVICONS = Combined.VIEW_WITH_FAVICONS;

    static final String VIEW_FLAGS = "flags";

    
    static final int BOOKMARKS = 100;
    static final int BOOKMARKS_ID = 101;
    static final int BOOKMARKS_FOLDER_ID = 102;
    static final int BOOKMARKS_PARENT = 103;
    static final int BOOKMARKS_POSITIONS = 104;

    
    static final int HISTORY = 200;
    static final int HISTORY_ID = 201;
    static final int HISTORY_OLD = 202;

    
    static final int FAVICONS = 300;
    static final int FAVICON_ID = 301;

    
    static final int SCHEMA = 400;

    
    static final int COMBINED = 500;

    
    static final int CONTROL = 600;

    
    static final int SEARCH_SUGGEST = 700;

    
    static final int THUMBNAILS = 800;
    static final int THUMBNAIL_ID = 801;

    static final int FLAGS = 900;

    static final String DEFAULT_BOOKMARKS_SORT_ORDER = Bookmarks.TYPE
            + " ASC, " + Bookmarks.POSITION + " ASC, " + Bookmarks._ID
            + " ASC";

    static final String DEFAULT_HISTORY_SORT_ORDER = History.DATE_LAST_VISITED + " DESC";

    static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);

    static final Map<String, String> BOOKMARKS_PROJECTION_MAP;
    static final Map<String, String> HISTORY_PROJECTION_MAP;
    static final Map<String, String> COMBINED_PROJECTION_MAP;
    static final Map<String, String> SCHEMA_PROJECTION_MAP;
    static final Map<String, String> SEARCH_SUGGEST_PROJECTION_MAP;
    static final Map<String, String> FAVICONS_PROJECTION_MAP;
    static final Map<String, String> THUMBNAILS_PROJECTION_MAP;
    static final Table[] sTables;

    static {
        sTables = new Table[] {
            new URLMetadataTable()
        };
        
        HashMap<String, String> map;

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "bookmarks", BOOKMARKS);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "bookmarks/#", BOOKMARKS_ID);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "bookmarks/parents", BOOKMARKS_PARENT);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "bookmarks/positions", BOOKMARKS_POSITIONS);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "bookmarks/folder/#", BOOKMARKS_FOLDER_ID);

        map = new HashMap<String, String>();
        map.put(Bookmarks._ID, Bookmarks._ID);
        map.put(Bookmarks.TITLE, Bookmarks.TITLE);
        map.put(Bookmarks.URL, Bookmarks.URL);
        map.put(Bookmarks.FAVICON, Bookmarks.FAVICON);
        map.put(Bookmarks.FAVICON_ID, Bookmarks.FAVICON_ID);
        map.put(Bookmarks.FAVICON_URL, Bookmarks.FAVICON_URL);
        map.put(Bookmarks.TYPE, Bookmarks.TYPE);
        map.put(Bookmarks.PARENT, Bookmarks.PARENT);
        map.put(Bookmarks.POSITION, Bookmarks.POSITION);
        map.put(Bookmarks.TAGS, Bookmarks.TAGS);
        map.put(Bookmarks.DESCRIPTION, Bookmarks.DESCRIPTION);
        map.put(Bookmarks.KEYWORD, Bookmarks.KEYWORD);
        map.put(Bookmarks.DATE_CREATED, Bookmarks.DATE_CREATED);
        map.put(Bookmarks.DATE_MODIFIED, Bookmarks.DATE_MODIFIED);
        map.put(Bookmarks.GUID, Bookmarks.GUID);
        map.put(Bookmarks.IS_DELETED, Bookmarks.IS_DELETED);
        BOOKMARKS_PROJECTION_MAP = Collections.unmodifiableMap(map);

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "history", HISTORY);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "history/#", HISTORY_ID);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "history/old", HISTORY_OLD);

        map = new HashMap<String, String>();
        map.put(History._ID, History._ID);
        map.put(History.TITLE, History.TITLE);
        map.put(History.URL, History.URL);
        map.put(History.FAVICON, History.FAVICON);
        map.put(History.FAVICON_ID, History.FAVICON_ID);
        map.put(History.FAVICON_URL, History.FAVICON_URL);
        map.put(History.VISITS, History.VISITS);
        map.put(History.DATE_LAST_VISITED, History.DATE_LAST_VISITED);
        map.put(History.DATE_CREATED, History.DATE_CREATED);
        map.put(History.DATE_MODIFIED, History.DATE_MODIFIED);
        map.put(History.GUID, History.GUID);
        map.put(History.IS_DELETED, History.IS_DELETED);
        HISTORY_PROJECTION_MAP = Collections.unmodifiableMap(map);

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "favicons", FAVICONS);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "favicons/#", FAVICON_ID);

        map = new HashMap<String, String>();
        map.put(Favicons._ID, Favicons._ID);
        map.put(Favicons.URL, Favicons.URL);
        map.put(Favicons.DATA, Favicons.DATA);
        map.put(Favicons.DATE_CREATED, Favicons.DATE_CREATED);
        map.put(Favicons.DATE_MODIFIED, Favicons.DATE_MODIFIED);
        FAVICONS_PROJECTION_MAP = Collections.unmodifiableMap(map);

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "thumbnails", THUMBNAILS);
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "thumbnails/#", THUMBNAIL_ID);

        map = new HashMap<String, String>();
        map.put(Thumbnails._ID, Thumbnails._ID);
        map.put(Thumbnails.URL, Thumbnails.URL);
        map.put(Thumbnails.DATA, Thumbnails.DATA);
        THUMBNAILS_PROJECTION_MAP = Collections.unmodifiableMap(map);

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "combined", COMBINED);

        map = new HashMap<String, String>();
        map.put(Combined._ID, Combined._ID);
        map.put(Combined.BOOKMARK_ID, Combined.BOOKMARK_ID);
        map.put(Combined.HISTORY_ID, Combined.HISTORY_ID);
        map.put(Combined.URL, Combined.URL);
        map.put(Combined.TITLE, Combined.TITLE);
        map.put(Combined.VISITS, Combined.VISITS);
        map.put(Combined.DATE_LAST_VISITED, Combined.DATE_LAST_VISITED);
        map.put(Combined.FAVICON, Combined.FAVICON);
        map.put(Combined.FAVICON_ID, Combined.FAVICON_ID);
        map.put(Combined.FAVICON_URL, Combined.FAVICON_URL);
        COMBINED_PROJECTION_MAP = Collections.unmodifiableMap(map);

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "schema", SCHEMA);

        map = new HashMap<String, String>();
        map.put(Schema.VERSION, Schema.VERSION);
        SCHEMA_PROJECTION_MAP = Collections.unmodifiableMap(map);


        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "control", CONTROL);

        
        URI_MATCHER.addURI(BrowserContract.AUTHORITY, SearchManager.SUGGEST_URI_PATH_QUERY + "/*", SEARCH_SUGGEST);

        URI_MATCHER.addURI(BrowserContract.AUTHORITY, "flags", FLAGS);

        map = new HashMap<String, String>();
        map.put(SearchManager.SUGGEST_COLUMN_TEXT_1,
                Combined.TITLE + " AS " + SearchManager.SUGGEST_COLUMN_TEXT_1);
        map.put(SearchManager.SUGGEST_COLUMN_TEXT_2_URL,
                Combined.URL + " AS " + SearchManager.SUGGEST_COLUMN_TEXT_2_URL);
        map.put(SearchManager.SUGGEST_COLUMN_INTENT_DATA,
                Combined.URL + " AS " + SearchManager.SUGGEST_COLUMN_INTENT_DATA);
        SEARCH_SUGGEST_PROJECTION_MAP = Collections.unmodifiableMap(map);

        for (Table table : sTables) {
            for (Table.ContentProviderInfo type : table.getContentProviderInfo()) {
                URI_MATCHER.addURI(BrowserContract.AUTHORITY, type.name, type.id);
            }
        }
    }

    private static boolean hasFaviconsInProjection(String[] projection) {
        if (projection == null) return true;
        for (int i = 0; i < projection.length; ++i) {
            if (projection[i].equals(FaviconColumns.FAVICON) ||
                projection[i].equals(FaviconColumns.FAVICON_URL))
                return true;
        }

        return false;
    }

    
    
    private static boolean logDebug   = Log.isLoggable(LOGTAG, Log.DEBUG);
    private static boolean logVerbose = Log.isLoggable(LOGTAG, Log.VERBOSE);
    protected static void trace(String message) {
        if (logVerbose) {
            Log.v(LOGTAG, message);
        }
    }

    protected static void debug(String message) {
        if (logDebug) {
            Log.d(LOGTAG, message);
        }
    }

    







    private void expireHistory(final SQLiteDatabase db, final int retain, final long keepAfter) {
        Log.d(LOGTAG, "Expiring history.");
        final long rows = DatabaseUtils.queryNumEntries(db, TABLE_HISTORY);

        if (retain >= rows) {
            debug("Not expiring history: only have " + rows + " rows.");
            return;
        }

        final String sortOrder = BrowserContract.getFrecencySortOrder(false, true);
        final long toRemove = rows - retain;
        debug("Expiring at most " + toRemove + " rows earlier than " + keepAfter + ".");

        final String sql;
        if (keepAfter > 0) {
            sql = "DELETE FROM " + TABLE_HISTORY + " " +
                  "WHERE MAX(" + History.DATE_LAST_VISITED + ", " + History.DATE_MODIFIED +") < " + keepAfter + " " +
                  " AND " + History._ID + " IN ( SELECT " +
                    History._ID + " FROM " + TABLE_HISTORY + " " +
                    "ORDER BY " + sortOrder + " LIMIT " + toRemove +
                  ")";
        } else {
            sql = "DELETE FROM " + TABLE_HISTORY + " WHERE " + History._ID + " " +
                  "IN ( SELECT " + History._ID + " FROM " + TABLE_HISTORY + " " +
                  "ORDER BY " + sortOrder + " LIMIT " + toRemove + ")";
        }
        trace("Deleting using query: " + sql);

        beginWrite(db);
        db.execSQL(sql);
    }

    





    private void expireThumbnails(final SQLiteDatabase db) {
        Log.d(LOGTAG, "Expiring thumbnails.");
        final String sortOrder = BrowserContract.getFrecencySortOrder(true, false);
        final String sql = "DELETE FROM " + TABLE_THUMBNAILS +
                           " WHERE " + Thumbnails.URL + " NOT IN ( " +
                             " SELECT " + Combined.URL +
                             " FROM " + Combined.VIEW_NAME +
                             " ORDER BY " + sortOrder +
                             " LIMIT " + DEFAULT_EXPIRY_THUMBNAIL_COUNT +
                           ") AND " + Thumbnails.URL + " NOT IN ( " +
                             " SELECT " + Bookmarks.URL +
                             " FROM " + TABLE_BOOKMARKS +
                             " WHERE " + Bookmarks.PARENT + " = " + Bookmarks.FIXED_PINNED_LIST_ID +
                           ")";
        trace("Clear thumbs using query: " + sql);
        db.execSQL(sql);
    }

    private boolean shouldIncrementVisits(Uri uri) {
        String incrementVisits = uri.getQueryParameter(BrowserContract.PARAM_INCREMENT_VISITS);
        return Boolean.parseBoolean(incrementVisits);
    }

    @Override
    public String getType(Uri uri) {
        final int match = URI_MATCHER.match(uri);

        trace("Getting URI type: " + uri);

        switch (match) {
            case BOOKMARKS:
                trace("URI is BOOKMARKS: " + uri);
                return Bookmarks.CONTENT_TYPE;
            case BOOKMARKS_ID:
                trace("URI is BOOKMARKS_ID: " + uri);
                return Bookmarks.CONTENT_ITEM_TYPE;
            case HISTORY:
                trace("URI is HISTORY: " + uri);
                return History.CONTENT_TYPE;
            case HISTORY_ID:
                trace("URI is HISTORY_ID: " + uri);
                return History.CONTENT_ITEM_TYPE;
            case SEARCH_SUGGEST:
                trace("URI is SEARCH_SUGGEST: " + uri);
                return SearchManager.SUGGEST_MIME_TYPE;
            case FLAGS:
                trace("URI is FLAGS.");
                return Bookmarks.CONTENT_ITEM_TYPE;
            default:
                String type = getContentItemType(match);
                if (type != null) {
                    trace("URI is " + type);
                    return type;
                }

                debug("URI has unrecognized type: " + uri);
                return null;
        }
    }

    @SuppressWarnings("fallthrough")
    @Override
    public int deleteInTransaction(Uri uri, String selection, String[] selectionArgs) {
        trace("Calling delete in transaction on URI: " + uri);
        final SQLiteDatabase db = getWritableDatabase(uri);

        final int match = URI_MATCHER.match(uri);
        int deleted = 0;

        switch (match) {
            case BOOKMARKS_ID:
                trace("Delete on BOOKMARKS_ID: " + uri);

                selection = DBUtils.concatenateWhere(selection, TABLE_BOOKMARKS + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case BOOKMARKS: {
                trace("Deleting bookmarks: " + uri);
                deleted = deleteBookmarks(uri, selection, selectionArgs);
                deleteUnusedImages(uri);
                break;
            }

            case HISTORY_ID:
                trace("Delete on HISTORY_ID: " + uri);

                selection = DBUtils.concatenateWhere(selection, TABLE_HISTORY + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case HISTORY: {
                trace("Deleting history: " + uri);
                beginWrite(db);
                deleted = deleteHistory(uri, selection, selectionArgs);
                deleteUnusedImages(uri);
                break;
            }

            case HISTORY_OLD: {
                String priority = uri.getQueryParameter(BrowserContract.PARAM_EXPIRE_PRIORITY);
                long keepAfter = System.currentTimeMillis() - DEFAULT_EXPIRY_PRESERVE_WINDOW;
                int retainCount = DEFAULT_EXPIRY_RETAIN_COUNT;

                if (BrowserContract.ExpirePriority.AGGRESSIVE.toString().equals(priority)) {
                    keepAfter = 0;
                    retainCount = AGGRESSIVE_EXPIRY_RETAIN_COUNT;
                }
                expireHistory(db, retainCount, keepAfter);
                expireThumbnails(db);
                deleteUnusedImages(uri);
                break;
            }

            case FAVICON_ID:
                debug("Delete on FAVICON_ID: " + uri);

                selection = DBUtils.concatenateWhere(selection, TABLE_FAVICONS + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case FAVICONS: {
                trace("Deleting favicons: " + uri);
                beginWrite(db);
                deleted = deleteFavicons(uri, selection, selectionArgs);
                break;
            }

            case THUMBNAIL_ID:
                debug("Delete on THUMBNAIL_ID: " + uri);

                selection = DBUtils.concatenateWhere(selection, TABLE_THUMBNAILS + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case THUMBNAILS: {
                trace("Deleting thumbnails: " + uri);
                beginWrite(db);
                deleted = deleteThumbnails(uri, selection, selectionArgs);
                break;
            }

            default: {
                Table table = findTableFor(match);
                if (table == null) {
                    throw new UnsupportedOperationException("Unknown delete URI " + uri);
                }
                trace("Deleting TABLE: " + uri);
                beginWrite(db);
                deleted = table.delete(db, uri, match, selection, selectionArgs);
            }
        }

        debug("Deleted " + deleted + " rows for URI: " + uri);

        return deleted;
    }

    @Override
    public Uri insertInTransaction(Uri uri, ContentValues values) {
        trace("Calling insert in transaction on URI: " + uri);

        int match = URI_MATCHER.match(uri);
        long id = -1;

        switch (match) {
            case BOOKMARKS: {
                trace("Insert on BOOKMARKS: " + uri);
                id = insertBookmark(uri, values);
                break;
            }

            case HISTORY: {
                trace("Insert on HISTORY: " + uri);
                id = insertHistory(uri, values);
                break;
            }

            case FAVICONS: {
                trace("Insert on FAVICONS: " + uri);
                id = insertFavicon(uri, values);
                break;
            }

            case THUMBNAILS: {
                trace("Insert on THUMBNAILS: " + uri);
                id = insertThumbnail(uri, values);
                break;
            }

            default: {
                Table table = findTableFor(match);
                if (table == null) {
                    throw new UnsupportedOperationException("Unknown insert URI " + uri);
                }

                trace("Insert on TABLE: " + uri);
                final SQLiteDatabase db = getWritableDatabase(uri);
                beginWrite(db);
                id = table.insert(db, uri, match, values);
            }
        }

        debug("Inserted ID in database: " + id);

        if (id >= 0)
            return ContentUris.withAppendedId(uri, id);

        return null;
    }

    @SuppressWarnings("fallthrough")
    @Override
    public int updateInTransaction(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        trace("Calling update in transaction on URI: " + uri);

        int match = URI_MATCHER.match(uri);
        int updated = 0;

        final SQLiteDatabase db = getWritableDatabase(uri);
        switch (match) {
            
            
            
            
            
            
            
            
            
            case BOOKMARKS_POSITIONS: {
                debug("Update on BOOKMARKS_POSITIONS: " + uri);

                
                updated = updateBookmarkPositions(uri, selectionArgs);
                break;
            }

            case BOOKMARKS_PARENT: {
                debug("Update on BOOKMARKS_PARENT: " + uri);
                beginWrite(db);
                updated = updateBookmarkParents(db, values, selection, selectionArgs);
                break;
            }

            case BOOKMARKS_ID:
                debug("Update on BOOKMARKS_ID: " + uri);

                selection = DBUtils.concatenateWhere(selection, TABLE_BOOKMARKS + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case BOOKMARKS: {
                debug("Updating bookmark: " + uri);
                if (shouldUpdateOrInsert(uri)) {
                    updated = updateOrInsertBookmark(uri, values, selection, selectionArgs);
                } else {
                    updated = updateBookmarks(uri, values, selection, selectionArgs);
                }
                break;
            }

            case HISTORY_ID:
                debug("Update on HISTORY_ID: " + uri);

                selection = DBUtils.concatenateWhere(selection, TABLE_HISTORY + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case HISTORY: {
                debug("Updating history: " + uri);
                if (shouldUpdateOrInsert(uri)) {
                    updated = updateOrInsertHistory(uri, values, selection, selectionArgs);
                } else {
                    updated = updateHistory(uri, values, selection, selectionArgs);
                }
                break;
            }

            case FAVICONS: {
                debug("Update on FAVICONS: " + uri);

                String url = values.getAsString(Favicons.URL);
                String faviconSelection = null;
                String[] faviconSelectionArgs = null;

                if (!TextUtils.isEmpty(url)) {
                    faviconSelection = Favicons.URL + " = ?";
                    faviconSelectionArgs = new String[] { url };
                }

                if (shouldUpdateOrInsert(uri)) {
                    updated = updateOrInsertFavicon(uri, values, faviconSelection, faviconSelectionArgs);
                } else {
                    updated = updateExistingFavicon(uri, values, faviconSelection, faviconSelectionArgs);
                }
                break;
            }

            case THUMBNAILS: {
                debug("Update on THUMBNAILS: " + uri);

                String url = values.getAsString(Thumbnails.URL);

                
                if (TextUtils.isEmpty(values.getAsString(Thumbnails.URL))) {
                    updated = updateExistingThumbnail(uri, values, null, null);
                } else if (shouldUpdateOrInsert(uri)) {
                    updated = updateOrInsertThumbnail(uri, values, Thumbnails.URL + " = ?",
                                                      new String[] { url });
                } else {
                    updated = updateExistingThumbnail(uri, values, Thumbnails.URL + " = ?",
                                                      new String[] { url });
                }
                break;
            }

            default: {
                Table table = findTableFor(match);
                if (table == null) {
                    throw new UnsupportedOperationException("Unknown update URI " + uri);
                }
                trace("Update TABLE: " + uri);

                beginWrite(db);
                updated = table.update(db, uri, match, values, selection, selectionArgs);
                if (shouldUpdateOrInsert(uri) && updated == 0) {
                    trace("No update, inserting for URL: " + uri);
                    table.insert(db, uri, match, values);
                    updated = 1;
                }
            }
        }

        debug("Updated " + updated + " rows for URI: " + uri);
        return updated;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        SQLiteDatabase db = getReadableDatabase(uri);
        final int match = URI_MATCHER.match(uri);

        
        if (match == FLAGS) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            final boolean includeDeleted = shouldShowDeleted(uri);
            final String query = "SELECT COALESCE(SUM(flag), 0) AS flags " +
                "FROM ( SELECT DISTINCT CASE" +
                " WHEN " + Bookmarks.PARENT + " = " + Bookmarks.FIXED_READING_LIST_ID +
                " THEN " + Bookmarks.FLAG_READING +

                " WHEN " + Bookmarks.PARENT + " = " + Bookmarks.FIXED_PINNED_LIST_ID +
                " THEN " + Bookmarks.FLAG_PINNED +

                " ELSE " + Bookmarks.FLAG_BOOKMARK +
                " END flag " +
                "FROM " + TABLE_BOOKMARKS + " WHERE " +
                Bookmarks.URL + " = ? " +
                (includeDeleted ? "" : ("AND " + Bookmarks.IS_DELETED + " = 0")) +
                ")";

            return db.rawQuery(query, selectionArgs);
        }

        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        String limit = uri.getQueryParameter(BrowserContract.PARAM_LIMIT);
        String groupBy = null;

        switch (match) {
            case BOOKMARKS_FOLDER_ID:
            case BOOKMARKS_ID:
            case BOOKMARKS: {
                debug("Query is on bookmarks: " + uri);

                if (match == BOOKMARKS_ID) {
                    selection = DBUtils.concatenateWhere(selection, Bookmarks._ID + " = ?");
                    selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                            new String[] { Long.toString(ContentUris.parseId(uri)) });
                } else if (match == BOOKMARKS_FOLDER_ID) {
                    selection = DBUtils.concatenateWhere(selection, Bookmarks.PARENT + " = ?");
                    selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                            new String[] { Long.toString(ContentUris.parseId(uri)) });
                }

                if (!shouldShowDeleted(uri))
                    selection = DBUtils.concatenateWhere(Bookmarks.IS_DELETED + " = 0", selection);

                if (TextUtils.isEmpty(sortOrder)) {
                    sortOrder = DEFAULT_BOOKMARKS_SORT_ORDER;
                } else {
                    debug("Using sort order " + sortOrder + ".");
                }

                qb.setProjectionMap(BOOKMARKS_PROJECTION_MAP);

                if (hasFaviconsInProjection(projection))
                    qb.setTables(VIEW_BOOKMARKS_WITH_FAVICONS);
                else
                    qb.setTables(TABLE_BOOKMARKS);

                break;
            }

            case HISTORY_ID:
                selection = DBUtils.concatenateWhere(selection, History._ID + " = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case HISTORY: {
                debug("Query is on history: " + uri);

                if (!shouldShowDeleted(uri))
                    selection = DBUtils.concatenateWhere(History.IS_DELETED + " = 0", selection);

                if (TextUtils.isEmpty(sortOrder))
                    sortOrder = DEFAULT_HISTORY_SORT_ORDER;

                qb.setProjectionMap(HISTORY_PROJECTION_MAP);

                if (hasFaviconsInProjection(projection))
                    qb.setTables(VIEW_HISTORY_WITH_FAVICONS);
                else
                    qb.setTables(TABLE_HISTORY);

                break;
            }

            case FAVICON_ID:
                selection = DBUtils.concatenateWhere(selection, Favicons._ID + " = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case FAVICONS: {
                debug("Query is on favicons: " + uri);

                qb.setProjectionMap(FAVICONS_PROJECTION_MAP);
                qb.setTables(TABLE_FAVICONS);

                break;
            }

            case THUMBNAIL_ID:
                selection = DBUtils.concatenateWhere(selection, Thumbnails._ID + " = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });
                
            case THUMBNAILS: {
                debug("Query is on thumbnails: " + uri);

                qb.setProjectionMap(THUMBNAILS_PROJECTION_MAP);
                qb.setTables(TABLE_THUMBNAILS);

                break;
            }

            case SCHEMA: {
                debug("Query is on schema.");
                MatrixCursor schemaCursor = new MatrixCursor(new String[] { Schema.VERSION });
                schemaCursor.newRow().add(BrowserDatabaseHelper.DATABASE_VERSION);

                return schemaCursor;
            }

            case COMBINED: {
                debug("Query is on combined: " + uri);

                if (TextUtils.isEmpty(sortOrder))
                    sortOrder = DEFAULT_HISTORY_SORT_ORDER;

                
                
                groupBy = Combined.URL;

                qb.setProjectionMap(COMBINED_PROJECTION_MAP);

                if (hasFaviconsInProjection(projection))
                    qb.setTables(VIEW_COMBINED_WITH_FAVICONS);
                else
                    qb.setTables(Combined.VIEW_NAME);

                break;
            }

            case SEARCH_SUGGEST: {
                debug("Query is on search suggest: " + uri);
                selection = DBUtils.concatenateWhere(selection, "(" + Combined.URL + " LIKE ? OR " +
                                                                      Combined.TITLE + " LIKE ?)");

                String keyword = uri.getLastPathSegment();
                if (keyword == null)
                    keyword = "";

                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { "%" + keyword + "%",
                                       "%" + keyword + "%" });

                if (TextUtils.isEmpty(sortOrder))
                    sortOrder = DEFAULT_HISTORY_SORT_ORDER;

                qb.setProjectionMap(SEARCH_SUGGEST_PROJECTION_MAP);
                qb.setTables(VIEW_COMBINED_WITH_FAVICONS);

                break;
            }

            default: {
                Table table = findTableFor(match);
                if (table == null) {
                    throw new UnsupportedOperationException("Unknown query URI " + uri);
                }
                trace("Update TABLE: " + uri);
                return table.query(db, uri, match, projection, selection, selectionArgs, sortOrder, groupBy, limit);
            }
        }

        trace("Running built query.");
        Cursor cursor = qb.query(db, projection, selection, selectionArgs, groupBy,
                null, sortOrder, limit);
        cursor.setNotificationUri(getContext().getContentResolver(),
                BrowserContract.AUTHORITY_URI);

        return cursor;
    }

    






    private int updateBookmarkPositions(Uri uri, String[] guids) {
        if (guids == null) {
            return 0;
        }

        int guidsCount = guids.length;
        if (guidsCount == 0) {
            return 0;
        }

        int offset = 0;
        int updated = 0;

        final SQLiteDatabase db = getWritableDatabase(uri);
        db.beginTransaction();

        while (offset < guidsCount) {
            try {
                updated += updateBookmarkPositionsInTransaction(db, guids, offset,
                                                                MAX_POSITION_UPDATES_PER_QUERY);
            } catch (SQLException e) {
                Log.e(LOGTAG, "Got SQLite exception updating bookmark positions at offset " + offset, e);

                
                
                
                
                db.setTransactionSuccessful();
                db.endTransaction();

                db.beginTransaction();
            }

            offset += MAX_POSITION_UPDATES_PER_QUERY;
        }

        db.setTransactionSuccessful();
        db.endTransaction();

        return updated;
    }

    



    private static int updateBookmarkPositionsInTransaction(final SQLiteDatabase db, final String[] guids,
                                                            final int offset, final int max) {
        int guidsCount = guids.length;
        int processCount = Math.min(max, guidsCount - offset);

        
        String[] args = new String[processCount * 2];
        System.arraycopy(guids, offset, args, 0, processCount);
        System.arraycopy(guids, offset, args, processCount, processCount);

        StringBuilder b = new StringBuilder("UPDATE " + TABLE_BOOKMARKS +
                                            " SET " + Bookmarks.POSITION +
                                            " = CASE guid");

        
        
        final int end = offset + processCount;
        int i = offset;
        for (; i < end; ++i) {
            if (guids[i] == null) {
                
                debug("updateBookmarkPositions called with null GUID at index " + i);
                return 0;
            }
            b.append(" WHEN ? THEN " + i);
        }

        b.append(" END WHERE " + DBUtils.computeSQLInClause(processCount, Bookmarks.GUID));
        db.execSQL(b.toString(), args);

        
        return processCount;
    }

    



    private int updateBookmarkParents(SQLiteDatabase db, ContentValues values, String selection, String[] selectionArgs) {
        trace("Updating bookmark parents of " + selection + " (" + selectionArgs[0] + ")");
        String where = Bookmarks._ID + " IN (" +
                       " SELECT DISTINCT " + Bookmarks.PARENT +
                       " FROM " + TABLE_BOOKMARKS +
                       " WHERE " + selection + " )";
        return db.update(TABLE_BOOKMARKS, values, where, selectionArgs);
    }

    private long insertBookmark(Uri uri, ContentValues values) {
        
        
        long now = System.currentTimeMillis();
        if (!values.containsKey(Bookmarks.DATE_CREATED)) {
            values.put(Bookmarks.DATE_CREATED, now);
        }

        if (!values.containsKey(Bookmarks.DATE_MODIFIED)) {
            values.put(Bookmarks.DATE_MODIFIED, now);
        }

        if (!values.containsKey(Bookmarks.GUID)) {
            values.put(Bookmarks.GUID, Utils.generateGuid());
        }

        if (!values.containsKey(Bookmarks.POSITION)) {
            debug("Inserting bookmark with no position for URI");
            values.put(Bookmarks.POSITION,
                       Long.toString(BrowserContract.Bookmarks.DEFAULT_POSITION));
        }

        if (!values.containsKey(Bookmarks.TITLE)) {
            
            
            values.put(Bookmarks.TITLE, "");
        }

        String url = values.getAsString(Bookmarks.URL);

        debug("Inserting bookmark in database with URL: " + url);
        final SQLiteDatabase db = getWritableDatabase(uri);
        beginWrite(db);
        return db.insertOrThrow(TABLE_BOOKMARKS, Bookmarks.TITLE, values);
    }


    private int updateOrInsertBookmark(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        int updated = updateBookmarks(uri, values, selection, selectionArgs);
        if (updated > 0) {
            return updated;
        }

        
        if (0 <= insertBookmark(uri, values)) {
            
            return 1;
        }

        
        return 0;
    }

    private int updateBookmarks(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        trace("Updating bookmarks on URI: " + uri);

        final String[] bookmarksProjection = new String[] {
                Bookmarks._ID, 
        };

        if (!values.containsKey(Bookmarks.DATE_MODIFIED)) {
            values.put(Bookmarks.DATE_MODIFIED, System.currentTimeMillis());
        }

        trace("Querying bookmarks to update on URI: " + uri);
        final SQLiteDatabase db = getWritableDatabase(uri);

        
        final Cursor cursor = db.query(TABLE_BOOKMARKS, bookmarksProjection,
                                       selection, selectionArgs, null, null, null);

        
        final String inClause;
        try {
            inClause = DBUtils.computeSQLInClauseFromLongs(cursor, Bookmarks._ID);
        } finally {
            cursor.close();
        }

        beginWrite(db);
        return db.update(TABLE_BOOKMARKS, values, inClause, null);
    }

    private long insertHistory(Uri uri, ContentValues values) {
        final long now = System.currentTimeMillis();
        values.put(History.DATE_CREATED, now);
        values.put(History.DATE_MODIFIED, now);

        
        if (!values.containsKey(History.GUID)) {
          values.put(History.GUID, Utils.generateGuid());
        }

        String url = values.getAsString(History.URL);

        debug("Inserting history in database with URL: " + url);
        final SQLiteDatabase db = getWritableDatabase(uri);
        beginWrite(db);
        return db.insertOrThrow(TABLE_HISTORY, History.VISITS, values);
    }

    private int updateOrInsertHistory(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        final int updated = updateHistory(uri, values, selection, selectionArgs);
        if (updated > 0) {
            return updated;
        }

        
        if (!values.containsKey(History.VISITS)) {
            values.put(History.VISITS, 1);
        }
        if (!values.containsKey(History.TITLE)) {
            values.put(History.TITLE, values.getAsString(History.URL));
        }

        if (0 <= insertHistory(uri, values)) {
            return 1;
        }

        return 0;
    }

    private int updateHistory(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        trace("Updating history on URI: " + uri);

        int updated = 0;

        final String[] historyProjection = new String[] {
            History._ID,   
            History.URL,   
            History.VISITS 
        };

        final SQLiteDatabase db = getWritableDatabase(uri);
        final Cursor cursor = db.query(TABLE_HISTORY, historyProjection, selection,
                                       selectionArgs, null, null, null);

        try {
            if (!values.containsKey(Bookmarks.DATE_MODIFIED)) {
                values.put(Bookmarks.DATE_MODIFIED,  System.currentTimeMillis());
            }

            while (cursor.moveToNext()) {
                long id = cursor.getLong(0);

                trace("Updating history entry with ID: " + id);

                if (shouldIncrementVisits(uri)) {
                    long existing = cursor.getLong(2);
                    Long additional = values.getAsLong(History.VISITS);

                    
                    values.put(History.VISITS, existing + ((additional != null) ? additional.longValue() : 1));
                }

                updated += db.update(TABLE_HISTORY, values, "_id = ?",
                                     new String[] { Long.toString(id) });
            }
        } finally {
            cursor.close();
        }

        return updated;
    }

    private void updateFaviconIdsForUrl(SQLiteDatabase db, String pageUrl, Long faviconId) {
        ContentValues updateValues = new ContentValues(1);
        updateValues.put(FaviconColumns.FAVICON_ID, faviconId);
        db.update(TABLE_HISTORY,
                  updateValues,
                  History.URL + " = ?",
                  new String[] { pageUrl });
        db.update(TABLE_BOOKMARKS,
                  updateValues,
                  Bookmarks.URL + " = ?",
                  new String[] { pageUrl });
    }

    private long insertFavicon(Uri uri, ContentValues values) {
        return insertFavicon(getWritableDatabase(uri), values);
    }

    private long insertFavicon(SQLiteDatabase db, ContentValues values) {
        String faviconUrl = values.getAsString(Favicons.URL);
        String pageUrl = null;

        trace("Inserting favicon for URL: " + faviconUrl);

        DBUtils.stripEmptyByteArray(values, Favicons.DATA);

        
        if (values.containsKey(Favicons.PAGE_URL)) {
            pageUrl = values.getAsString(Favicons.PAGE_URL);
            values.remove(Favicons.PAGE_URL);
        }

        
        if (TextUtils.isEmpty(faviconUrl) && !TextUtils.isEmpty(pageUrl)) {
            values.put(Favicons.URL, org.mozilla.gecko.favicons.Favicons.guessDefaultFaviconURL(pageUrl));
        }

        final long now = System.currentTimeMillis();
        values.put(Favicons.DATE_CREATED, now);
        values.put(Favicons.DATE_MODIFIED, now);

        beginWrite(db);
        final long faviconId = db.insertOrThrow(TABLE_FAVICONS, null, values);

        if (pageUrl != null) {
            updateFaviconIdsForUrl(db, pageUrl, faviconId);
        }
        return faviconId;
    }

    private int updateOrInsertFavicon(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        return updateFavicon(uri, values, selection, selectionArgs,
                true );
    }

    private int updateExistingFavicon(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        return updateFavicon(uri, values, selection, selectionArgs,
                false );
    }

    private int updateFavicon(Uri uri, ContentValues values, String selection,
            String[] selectionArgs, boolean insertIfNeeded) {
        String faviconUrl = values.getAsString(Favicons.URL);
        String pageUrl = null;
        int updated = 0;
        Long faviconId = null;
        long now = System.currentTimeMillis();

        trace("Updating favicon for URL: " + faviconUrl);

        DBUtils.stripEmptyByteArray(values, Favicons.DATA);

        
        if (values.containsKey(Favicons.PAGE_URL)) {
            pageUrl = values.getAsString(Favicons.PAGE_URL);
            values.remove(Favicons.PAGE_URL);
        }

        values.put(Favicons.DATE_MODIFIED, now);

        final SQLiteDatabase db = getWritableDatabase(uri);

        
        
        
        if (!(insertIfNeeded && (faviconUrl == null))) {
            updated = db.update(TABLE_FAVICONS, values, selection, selectionArgs);
        }

        if (updated > 0) {
            if ((faviconUrl != null) && (pageUrl != null)) {
                final Cursor cursor = db.query(TABLE_FAVICONS,
                                               new String[] { Favicons._ID },
                                               Favicons.URL + " = ?",
                                               new String[] { faviconUrl },
                                               null, null, null);
                try {
                    if (cursor.moveToFirst()) {
                        faviconId = cursor.getLong(cursor.getColumnIndexOrThrow(Favicons._ID));
                    }
                } finally {
                    cursor.close();
                }
            }
            if (pageUrl != null) {
                beginWrite(db);
            }
        } else if (insertIfNeeded) {
            values.put(Favicons.DATE_CREATED, now);

            trace("No update, inserting favicon for URL: " + faviconUrl);
            beginWrite(db);
            faviconId = db.insert(TABLE_FAVICONS, null, values);
            updated = 1;
        }

        if (pageUrl != null) {
            updateFaviconIdsForUrl(db, pageUrl, faviconId);
        }

        return updated;
    }

    private long insertThumbnail(Uri uri, ContentValues values) {
        final String url = values.getAsString(Thumbnails.URL);

        trace("Inserting thumbnail for URL: " + url);

        DBUtils.stripEmptyByteArray(values, Thumbnails.DATA);

        final SQLiteDatabase db = getWritableDatabase(uri);
        beginWrite(db);
        return db.insertOrThrow(TABLE_THUMBNAILS, null, values);
    }

    private int updateOrInsertThumbnail(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        return updateThumbnail(uri, values, selection, selectionArgs,
                true );
    }

    private int updateExistingThumbnail(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        return updateThumbnail(uri, values, selection, selectionArgs,
                false );
    }

    private int updateThumbnail(Uri uri, ContentValues values, String selection,
            String[] selectionArgs, boolean insertIfNeeded) {
        final String url = values.getAsString(Thumbnails.URL);
        DBUtils.stripEmptyByteArray(values, Thumbnails.DATA);

        trace("Updating thumbnail for URL: " + url);

        final SQLiteDatabase db = getWritableDatabase(uri);
        beginWrite(db);
        int updated = db.update(TABLE_THUMBNAILS, values, selection, selectionArgs);

        if (updated == 0 && insertIfNeeded) {
            trace("No update, inserting thumbnail for URL: " + url);
            db.insert(TABLE_THUMBNAILS, null, values);
            updated = 1;
        }

        return updated;
    }

    





    private int deleteHistory(Uri uri, String selection, String[] selectionArgs) {
        debug("Deleting history entry for URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);

        if (isCallerSync(uri)) {
            return db.delete(TABLE_HISTORY, selection, selectionArgs);
        }

        debug("Marking history entry as deleted for URI: " + uri);

        ContentValues values = new ContentValues();
        values.put(History.IS_DELETED, 1);

        
        values.putNull(History.TITLE);
        values.put(History.URL, "");          
        values.put(History.DATE_CREATED, 0);
        values.put(History.DATE_LAST_VISITED, 0);
        values.put(History.VISITS, 0);
        values.put(History.DATE_MODIFIED, System.currentTimeMillis());

        
        
        
        
        
        
        final int updated = db.update(TABLE_HISTORY, values, selection, selectionArgs);
        try {
            cleanUpSomeDeletedRecords(uri, TABLE_HISTORY);
        } catch (Exception e) {
            
            Log.e(LOGTAG, "Unable to clean up deleted history records: ", e);
        }
        return updated;
    }

    private int deleteBookmarks(Uri uri, String selection, String[] selectionArgs) {
        debug("Deleting bookmarks for URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);

        if (isCallerSync(uri)) {
            beginWrite(db);
            return db.delete(TABLE_BOOKMARKS, selection, selectionArgs);
        }

        debug("Marking bookmarks as deleted for URI: " + uri);

        ContentValues values = new ContentValues();
        values.put(Bookmarks.IS_DELETED, 1);

        
        
        
        
        final int updated = updateBookmarks(uri, values, selection, selectionArgs);
        try {
            cleanUpSomeDeletedRecords(uri, TABLE_BOOKMARKS);
        } catch (Exception e) {
            
            Log.e(LOGTAG, "Unable to clean up deleted bookmark records: ", e);
        }
        return updated;
    }

    private int deleteFavicons(Uri uri, String selection, String[] selectionArgs) {
        debug("Deleting favicons for URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);

        return db.delete(TABLE_FAVICONS, selection, selectionArgs);
    }

    private int deleteThumbnails(Uri uri, String selection, String[] selectionArgs) {
        debug("Deleting thumbnails for URI: " + uri);

        final SQLiteDatabase db = getWritableDatabase(uri);

        return db.delete(TABLE_THUMBNAILS, selection, selectionArgs);
    }

    private int deleteUnusedImages(Uri uri) {
        debug("Deleting all unused favicons and thumbnails for URI: " + uri);

        String faviconSelection = Favicons._ID + " NOT IN "
                + "(SELECT " + History.FAVICON_ID
                + " FROM " + TABLE_HISTORY
                + " WHERE " + History.IS_DELETED + " = 0"
                + " AND " + History.FAVICON_ID + " IS NOT NULL"
                + " UNION ALL SELECT " + Bookmarks.FAVICON_ID
                + " FROM " + TABLE_BOOKMARKS
                + " WHERE " + Bookmarks.IS_DELETED + " = 0"
                + " AND " + Bookmarks.FAVICON_ID + " IS NOT NULL)";

        String thumbnailSelection = Thumbnails.URL + " NOT IN "
                + "(SELECT " + History.URL
                + " FROM " + TABLE_HISTORY
                + " WHERE " + History.IS_DELETED + " = 0"
                + " AND " + History.URL + " IS NOT NULL"
                + " UNION ALL SELECT " + Bookmarks.URL
                + " FROM " + TABLE_BOOKMARKS
                + " WHERE " + Bookmarks.IS_DELETED + " = 0"
                + " AND " + Bookmarks.URL + " IS NOT NULL)";

        return deleteFavicons(uri, faviconSelection, null) +
               deleteThumbnails(uri, thumbnailSelection, null) +
               URLMetadata.deleteUnused(getContext().getContentResolver());
    }

    @Override
    public ContentProviderResult[] applyBatch (ArrayList<ContentProviderOperation> operations)
        throws OperationApplicationException {
        final int numOperations = operations.size();
        final ContentProviderResult[] results = new ContentProviderResult[numOperations];

        if (numOperations < 1) {
            debug("applyBatch: no operations; returning immediately.");
            
            
            return results;
        }

        boolean failures = false;

        
        SQLiteDatabase db = getWritableDatabase(operations.get(0).getUri());

        
        
        
        
        
        
        
        
        
        beginBatch(db);

        for (int i = 0; i < numOperations; i++) {
            try {
                final ContentProviderOperation operation = operations.get(i);
                results[i] = operation.apply(this, results, i);
            } catch (SQLException e) {
                Log.w(LOGTAG, "SQLite Exception during applyBatch.", e);
                
                
                
                
                
                
                
                
                results[i] = new ContentProviderResult(0);
                failures = true;
                
                
                
                
                
                
                
                
                db.setTransactionSuccessful();
                db.endTransaction();
                db.beginTransaction();
            } catch (OperationApplicationException e) {
                
                results[i] = new ContentProviderResult(0);
                failures = true;
                db.setTransactionSuccessful();
                db.endTransaction();
                db.beginTransaction();
            }
        }

        trace("Flushing DB applyBatch...");
        markBatchSuccessful(db);
        endBatch(db);

        if (failures) {
            throw new OperationApplicationException();
        }

        return results;
    }

    private static Table findTableFor(int id) {
        for (Table table : sTables) {
            for (Table.ContentProviderInfo type : table.getContentProviderInfo()) {
                if (type.id == id) {
                    return table;
                }
            }
        }
        return null;
    }

    private static void addTablesToMatcher(Table[] tables, final UriMatcher matcher) {
    }

    private static String getContentItemType(final int match) {
        for (Table table : sTables) {
            for (Table.ContentProviderInfo type : table.getContentProviderInfo()) {
                if (type.id == match) {
                    return "vnd.android.cursor.item/" + type.name;
                }
            }
        }

        return null;
    }
}
