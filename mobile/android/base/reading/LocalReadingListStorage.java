



package org.mozilla.gecko.reading;

import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.SYNC_CHANGE_FAVORITE_CHANGED;
import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.SYNC_CHANGE_FLAGS;
import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.SYNC_CHANGE_UNREAD_CHANGED;
import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.SYNC_STATUS;
import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.SYNC_STATUS_MODIFIED;
import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.SYNC_STATUS_NEW;

import java.util.ArrayList;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;

import android.content.ContentProviderClient;
import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentValues;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.net.Uri;
import android.os.RemoteException;

public class LocalReadingListStorage implements ReadingListStorage {

  private static final String WHERE_STATUS_NEW = "(" + SYNC_STATUS + " = " + SYNC_STATUS_NEW + ")";

  final class LocalReadingListChangeAccumulator implements ReadingListChangeAccumulator {
    private static final String LOG_TAG = "RLChanges";

    



    private final Queue<ClientReadingListRecord> changes;

    




    private final Queue<ClientReadingListRecord> deletions;
    private final Queue<String> deletedGUIDs;

    








    private final Queue<ServerReadingListRecord> additionsOrChanges;

    LocalReadingListChangeAccumulator() {
      this.changes = new ConcurrentLinkedQueue<>();
      this.deletions = new ConcurrentLinkedQueue<>();
      this.deletedGUIDs = new ConcurrentLinkedQueue<>();
      this.additionsOrChanges = new ConcurrentLinkedQueue<>();
    }

    public boolean flushDeletions() throws RemoteException {
      if (deletions.isEmpty() && deletedGUIDs.isEmpty()) {
        return true;
      }

      long[] ids = new long[deletions.size()];
      String[] guids = new String[deletions.size() + deletedGUIDs.size()];
      int iID = 0;
      int iGUID = 0;
      for (ClientReadingListRecord record : deletions) {
        if (record.clientMetadata.id > -1L) {
          ids[iID++] = record.clientMetadata.id;
        } else {
          final String guid = record.getGUID();
          if (guid == null) {
            continue;
          }
          guids[iGUID++] = guid;
        }
      }
      for (String guid : deletedGUIDs) {
        guids[iGUID++] = guid;
      }

      if (iID > 0) {
        client.delete(URI_WITH_DELETED, RepoUtils.computeSQLLongInClause(ids, ReadingListItems._ID), null);
      }

      if (iGUID > 0) {
        client.delete(URI_WITH_DELETED, RepoUtils.computeSQLInClause(iGUID, ReadingListItems.GUID), guids);
      }

      deletions.clear();
      deletedGUIDs.clear();
      return true;
    }

    public boolean flushRecordChanges() throws RemoteException {
      if (changes.isEmpty() && additionsOrChanges.isEmpty()) {
        return true;
      }

      
      
      
      
      
      ArrayList<ContentProviderOperation> operations = new ArrayList<>(changes.size() + additionsOrChanges.size());
      for (ClientReadingListRecord rec : changes) {
        operations.add(makeUpdateOp(rec));
      }

      for (ServerReadingListRecord rec : additionsOrChanges) {
        
        
        
        
        
        operations.add(makeUpdateOrInsertOp(rec));
      }

      
      try {
        Logger.debug(LOG_TAG, "Applying " + operations.size() + " operations.");
        ContentProviderResult[] results = client.applyBatch(operations);
      } catch (OperationApplicationException e) {
        
        Logger.warn(LOG_TAG, "Applying operations failed.", e);
        return false;
      }
      return true;
    }

    private ContentProviderOperation makeUpdateOrInsertOp(ServerReadingListRecord rec) throws RemoteException {
      final ClientReadingListRecord clientRec = new ClientReadingListRecord(rec.serverMetadata, null, rec.fields);

      
      if (hasGUID(rec.serverMetadata.guid)) {
        return makeUpdateOp(clientRec);
      }

      final ContentValues values = ReadingListClientContentValuesFactory.fromClientRecord(clientRec);
      return ContentProviderOperation.newInsert(URI_WITHOUT_DELETED)
                                     .withValues(values)
                                     .build();
    }

    private ContentProviderOperation makeUpdateOp(ClientReadingListRecord rec) {
      
      
      final StringBuilder selection = new StringBuilder();
      final String[] selectionArgs;

      
      
      
      long serverLastModified = rec.getServerLastModified();
      if (serverLastModified != -1L) {
        
        selection.append("(" + ReadingListItems.SERVER_LAST_MODIFIED + " IS NOT ");
        selection.append(serverLastModified);
        selection.append(") AND ");
      }

      if (rec.clientMetadata.id > -1L) {
        selection.append("(");
        selection.append(ReadingListItems._ID + " = ");
        selection.append(rec.clientMetadata.id);
        selection.append(")");
        selectionArgs = null;
      } else if (rec.serverMetadata.guid != null) {
        selection.append("(" + ReadingListItems.GUID + " = ?)");
        selectionArgs = new String[] { rec.serverMetadata.guid };
      } else {
        final String url = rec.fields.getString("url");
        final String resolvedURL = rec.fields.getString("resolved_url");

        if (url == null && resolvedURL == null) {
          
          return null;
        }

        selection.append("((" + ReadingListItems.URL + " = ?) OR (" + ReadingListItems.RESOLVED_URL + " = ?))");
        if (url != null && resolvedURL != null) {
          selectionArgs = new String[] { url, resolvedURL };
        } else {
          final String arg = url == null ? resolvedURL : url;
          selectionArgs = new String[] { arg, arg };
        }
      }

      final ContentValues values = ReadingListClientContentValuesFactory.fromClientRecord(rec);
      return ContentProviderOperation.newUpdate(URI_WITHOUT_DELETED)
                                     .withSelection(selection.toString(), selectionArgs)
                                     .withValues(values)
                                     .build();
    }

    @Override
    public void finish() throws Exception {
      flushDeletions();
      flushRecordChanges();
    }

    @Override
    public void addDeletion(ClientReadingListRecord record) {
      deletions.add(record);
    }

    @Override
    public void addDeletion(String guid) {
      deletedGUIDs.add(guid);
    }

    @Override
    public void addChangedRecord(ClientReadingListRecord record) {
      changes.add(record);
    }

    @Override
    public void addDownloadedRecord(ServerReadingListRecord down) {
      final Boolean deleted = down.fields.getBoolean("deleted");
      if (deleted != null && deleted.booleanValue()) {
        addDeletion(down.getGUID());
      } else {
        additionsOrChanges.add(down);
      }
    }
  }

  private final ContentProviderClient client;
  private final Uri URI_WITHOUT_DELETED = BrowserContract.READING_LIST_AUTHORITY_URI
      .buildUpon()
      .appendPath("items")
      .appendQueryParameter(BrowserContract.PARAM_IS_SYNC, "1")
      .appendQueryParameter(BrowserContract.PARAM_SHOW_DELETED, "0")
      .build();

  private final Uri URI_WITH_DELETED = BrowserContract.READING_LIST_AUTHORITY_URI
      .buildUpon()
      .appendPath("items")
      .appendQueryParameter(BrowserContract.PARAM_IS_SYNC, "1")
      .appendQueryParameter(BrowserContract.PARAM_SHOW_DELETED, "1")
      .build();

  public LocalReadingListStorage(final ContentProviderClient client) {
    this.client = client;
  }

  public boolean hasGUID(String guid) throws RemoteException {
    final String[] projection = new String[] { ReadingListItems.GUID };
    final String selection = ReadingListItems.GUID + " = ?";
    final String[] selectionArgs = new String[] { guid };
    final Cursor cursor = this.client.query(URI_WITHOUT_DELETED, projection, selection, selectionArgs, null);
    try {
      return cursor.moveToFirst();
    } finally {
      cursor.close();
    }
  }

  public Cursor getModifiedWithSelection(final String selection) {
    final String[] projection = new String[] {
      ReadingListItems.GUID,
      ReadingListItems.IS_FAVORITE,
      ReadingListItems.RESOLVED_TITLE,
      ReadingListItems.RESOLVED_URL,
      ReadingListItems.EXCERPT,
      
      
    };

    try {
      return client.query(URI_WITHOUT_DELETED, projection, selection, null, null);
    } catch (RemoteException e) {
      throw new IllegalStateException(e);
    }
  }

  @Override
  public Cursor getModified() {
    final String selection = ReadingListItems.SYNC_STATUS + " = " + ReadingListItems.SYNC_STATUS_MODIFIED;
    return getModifiedWithSelection(selection);
  }

  
  
  
  public Cursor getNonStatusModified() {
    final String selection = ReadingListItems.SYNC_STATUS + " = " + ReadingListItems.SYNC_STATUS_MODIFIED +
                             " AND ((" + ReadingListItems.SYNC_CHANGE_FLAGS + " & " + ReadingListItems.SYNC_CHANGE_RESOLVED + ") > 0)";

    return getModifiedWithSelection(selection);
  }

  
  
  
  @Override
  public Cursor getStatusChanges() {
    final String[] projection = new String[] {
      ReadingListItems.GUID,
      ReadingListItems.IS_FAVORITE,
      ReadingListItems.IS_UNREAD,
      ReadingListItems.MARKED_READ_BY,
      ReadingListItems.MARKED_READ_ON,
      ReadingListItems.SYNC_CHANGE_FLAGS,
    };

    final String selection =
        SYNC_STATUS + " = " + SYNC_STATUS_MODIFIED + " AND " +
        "((" + SYNC_CHANGE_FLAGS + " & (" + SYNC_CHANGE_UNREAD_CHANGED + " | " + SYNC_CHANGE_FAVORITE_CHANGED + ")) > 0)";

    try {
      return client.query(URI_WITHOUT_DELETED, projection, selection, null, null);
    } catch (RemoteException e) {
      throw new IllegalStateException(e);
    }
  }

  @Override
  public Cursor getDeletedItems() {
    final String[] projection = new String[] {
      ReadingListItems.GUID,
    };

    final String selection = "(" + ReadingListItems.IS_DELETED + " = 1) AND (" + ReadingListItems.GUID + " IS NOT NULL)";
    try {
      return client.query(URI_WITH_DELETED, projection, selection, null, null);
    } catch (RemoteException e) {
      throw new IllegalStateException(e);
    }
  }

  @Override
  public Cursor getNew() {
    
    
    final String selection = WHERE_STATUS_NEW + " OR (" + ReadingListItems.GUID + " IS NULL)";

    try {
      return client.query(URI_WITHOUT_DELETED, null, selection, null, null);
    } catch (RemoteException e) {
      throw new IllegalStateException(e);
    }
  }

  @Override
  public Cursor getAll() {
    try {
      return client.query(URI_WITHOUT_DELETED, null, null, null, null);
    } catch (RemoteException e) {
      throw new IllegalStateException(e);
    }
  }

  private ContentProviderOperation updateAddedByNames(final String local) {
    String[] selectionArgs = new String[] {"$local"};
    String selection = WHERE_STATUS_NEW + " AND (" + ReadingListItems.ADDED_BY + " = ?)";
    return ContentProviderOperation.newUpdate(URI_WITHOUT_DELETED)
                                   .withValue(ReadingListItems.ADDED_BY, local)
                                   .withSelection(selection, selectionArgs)
                                   .build();
  }

  private ContentProviderOperation updateMarkedReadByNames(final String local) {
    String[] selectionArgs = new String[] {"$local"};
    String selection = ReadingListItems.MARKED_READ_BY + " = ?";
    return ContentProviderOperation.newUpdate(URI_WITHOUT_DELETED)
                                   .withValue(ReadingListItems.MARKED_READ_BY, local)
                                   .withSelection(selection, selectionArgs)
                                   .build();
  }

  














  public void updateLocalNames(final String local) {
    ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>(2);
    ops.add(updateAddedByNames(local));
    ops.add(updateMarkedReadByNames(local));

    try {
      client.applyBatch(ops);
    } catch (RemoteException e) {
      return;
    } catch (OperationApplicationException e) {
      return;
    }
  }

  @Override
  public ReadingListChangeAccumulator getChangeAccumulator() {
    return new LocalReadingListChangeAccumulator();
  }

  


  













}
