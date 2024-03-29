



package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.db.Tab;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.Clients;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.NoContentProviderException;
import org.mozilla.gecko.sync.repositories.NoStoreDelegateException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;
import org.mozilla.gecko.sync.repositories.domain.TabsRecord;

import android.content.ContentProviderClient;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.RemoteException;

public class FennecTabsRepository extends Repository {
  private static final String LOG_TAG = "FennecTabsRepository";

  protected final ClientsDataDelegate clientsDataDelegate;

  public FennecTabsRepository(ClientsDataDelegate clientsDataDelegate) {
    this.clientsDataDelegate = clientsDataDelegate;
  }

  







  public class FennecTabsRepositorySession extends RepositorySession {
    protected static final String LOG_TAG = "FennecTabsSession";

    private final ContentProviderClient tabsProvider;
    private final ContentProviderClient clientsProvider;

    protected final RepoUtils.QueryHelper tabsHelper;

    protected final ClientsDatabaseAccessor clientsDatabase;

    protected ContentProviderClient getContentProvider(final Context context, final Uri uri) throws NoContentProviderException {
      ContentProviderClient client = context.getContentResolver().acquireContentProviderClient(uri);
      if (client == null) {
        throw new NoContentProviderException(uri);
      }
      return client;
    }

    protected void releaseProviders() {
      try {
        clientsProvider.release();
      } catch (Exception e) {}
      try {
        tabsProvider.release();
      } catch (Exception e) {}
      clientsDatabase.close();
    }

    public FennecTabsRepositorySession(Repository repository, Context context) throws NoContentProviderException {
      super(repository);
      clientsProvider = getContentProvider(context, BrowserContractHelpers.CLIENTS_CONTENT_URI);
      try {
        tabsProvider = getContentProvider(context, BrowserContractHelpers.TABS_CONTENT_URI);
      } catch (NoContentProviderException e) {
        clientsProvider.release();
        throw e;
      } catch (Exception e) {
        clientsProvider.release();
        
        throw new RuntimeException(e);
      }

      tabsHelper = new RepoUtils.QueryHelper(context, BrowserContractHelpers.TABS_CONTENT_URI, LOG_TAG);
      clientsDatabase = new ClientsDatabaseAccessor(context);
    }

    @Override
    public void abort() {
      releaseProviders();
      super.abort();
    }

    @Override
    public void finish(final RepositorySessionFinishDelegate delegate) throws InactiveSessionException {
      releaseProviders();
      super.finish(delegate);
    }

    
    
    protected String localClientSelection() {
      return BrowserContract.Tabs.CLIENT_GUID + " IS NULL";
    }

    protected String[] localClientSelectionArgs() {
      return null;
    }

    @Override
    public void guidsSince(final long timestamp,
                           final RepositorySessionGuidsSinceDelegate delegate) {
      
      
      Logger.warn(LOG_TAG, "Not returning anything from guidsSince.");
      delegateQueue.execute(new Runnable() {
        @Override
        public void run() {
          delegate.onGuidsSinceSucceeded(new String[] {});
        }
      });
    }

    @Override
    public void fetchSince(final long timestamp,
                           final RepositorySessionFetchRecordsDelegate delegate) {
      if (tabsProvider == null) {
        throw new IllegalArgumentException("tabsProvider was null.");
      }
      if (tabsHelper == null) {
        throw new IllegalArgumentException("tabsHelper was null.");
      }

      final String positionAscending = BrowserContract.Tabs.POSITION + " ASC";

      final String localClientSelection = localClientSelection();
      final String[] localClientSelectionArgs = localClientSelectionArgs();

      final Runnable command = new Runnable() {
        @Override
        public void run() {
          
          
          
          try {
            final Cursor cursor = tabsHelper.safeQuery(tabsProvider, ".fetchSince()", null,
                localClientSelection, localClientSelectionArgs, positionAscending);
            try {
              final String localClientGuid = clientsDataDelegate.getAccountGUID();
              final String localClientName = clientsDataDelegate.getClientName();
              final TabsRecord tabsRecord = FennecTabsRepository.tabsRecordFromCursor(cursor, localClientGuid, localClientName);

              if (tabsRecord.lastModified >= timestamp ||
                  clientsDataDelegate.getLastModifiedTimestamp() >= timestamp) {
                delegate.onFetchedRecord(tabsRecord);
              }
            } finally {
              cursor.close();
            }
          } catch (Exception e) {
            delegate.onFetchFailed(e, null);
            return;
          }
          delegate.onFetchCompleted(now());
        }
      };

      delegateQueue.execute(command);
    }

    @Override
    public void fetch(final String[] guids,
                      final RepositorySessionFetchRecordsDelegate delegate) {
      
      
      Logger.warn(LOG_TAG, "Not returning anything from fetch");
      delegateQueue.execute(new Runnable() {
        @Override
        public void run() {
          delegate.onFetchCompleted(now());
        }
      });
    }

    @Override
    public void fetchAll(final RepositorySessionFetchRecordsDelegate delegate) {
      fetchSince(0, delegate);
    }

    private static final String TABS_CLIENT_GUID_IS = BrowserContract.Tabs.CLIENT_GUID + " = ?";
    private static final String CLIENT_GUID_IS = BrowserContract.Clients.GUID + " = ?";

    @Override
    public void store(final Record record) throws NoStoreDelegateException {
      if (delegate == null) {
        Logger.warn(LOG_TAG, "No store delegate.");
        throw new NoStoreDelegateException();
      }
      if (record == null) {
        Logger.error(LOG_TAG, "Record sent to store was null");
        throw new IllegalArgumentException("Null record passed to FennecTabsRepositorySession.store().");
      }
      if (!(record instanceof TabsRecord)) {
        Logger.error(LOG_TAG, "Can't store anything but a TabsRecord");
        throw new IllegalArgumentException("Non-TabsRecord passed to FennecTabsRepositorySession.store().");
      }
      final TabsRecord tabsRecord = (TabsRecord) record;

      Runnable command = new Runnable() {
        @Override
        public void run() {
          Logger.debug(LOG_TAG, "Storing tabs for client " + tabsRecord.guid);
          if (!isActive()) {
            delegate.onRecordStoreFailed(new InactiveSessionException(null), record.guid);
            return;
          }
          if (tabsRecord.guid == null) {
            delegate.onRecordStoreFailed(new RuntimeException("Can't store record with null GUID."), record.guid);
            return;
          }

          try {
            
            final String[] selectionArgs = new String[] { tabsRecord.guid };
            if (tabsRecord.deleted) {
              try {
                Logger.debug(LOG_TAG, "Clearing entry for client " + tabsRecord.guid);
                clientsProvider.delete(BrowserContractHelpers.CLIENTS_CONTENT_URI,
                                       CLIENT_GUID_IS,
                                       selectionArgs);
                delegate.onRecordStoreSucceeded(record.guid);
              } catch (Exception e) {
                delegate.onRecordStoreFailed(e, record.guid);
              }
              return;
            }

            
            final ContentValues clientsCV = tabsRecord.getClientsContentValues();

            final ClientRecord clientRecord = clientsDatabase.fetchClient(tabsRecord.guid);
            if (null != clientRecord) {
                
                clientsCV.put(Clients.DEVICE_TYPE, clientRecord.type);
            }

            Logger.debug(LOG_TAG, "Updating clients provider.");
            final int updated = clientsProvider.update(BrowserContractHelpers.CLIENTS_CONTENT_URI,
                clientsCV,
                CLIENT_GUID_IS,
                selectionArgs);
            if (0 == updated) {
              clientsProvider.insert(BrowserContractHelpers.CLIENTS_CONTENT_URI, clientsCV);
            }

            
            final ContentValues[] tabsArray = tabsRecord.getTabsContentValues();
            Logger.debug(LOG_TAG, "Inserting " + tabsArray.length + " tabs for client " + tabsRecord.guid);

            tabsProvider.delete(BrowserContractHelpers.TABS_CONTENT_URI, TABS_CLIENT_GUID_IS, selectionArgs);
            final int inserted = tabsProvider.bulkInsert(BrowserContractHelpers.TABS_CONTENT_URI, tabsArray);
            Logger.trace(LOG_TAG, "Inserted: " + inserted);

            delegate.onRecordStoreSucceeded(record.guid);
          } catch (Exception e) {
            Logger.warn(LOG_TAG, "Error storing tabs.", e);
            delegate.onRecordStoreFailed(e, record.guid);
          }
        }
      };

      storeWorkQueue.execute(command);
    }

    @Override
    public void wipe(RepositorySessionWipeDelegate delegate) {
      try {
        tabsProvider.delete(BrowserContractHelpers.TABS_CONTENT_URI, null, null);
        clientsProvider.delete(BrowserContractHelpers.CLIENTS_CONTENT_URI, null, null);
      } catch (RemoteException e) {
        Logger.warn(LOG_TAG, "Got RemoteException in wipe.", e);
        delegate.onWipeFailed(e);
        return;
      }
      delegate.onWipeSucceeded();
    }
  }

  @Override
  public void createSession(RepositorySessionCreationDelegate delegate,
                            Context context) {
    try {
      final FennecTabsRepositorySession session = new FennecTabsRepositorySession(this, context);
      delegate.onSessionCreated(session);
    } catch (Exception e) {
      delegate.onSessionCreateFailed(e);
    }
  }

  















  public static TabsRecord tabsRecordFromCursor(final Cursor cursor, final String clientGuid, final String clientName) {
    final String collection = "tabs";
    final TabsRecord record = new TabsRecord(clientGuid, collection, 0, false);
    record.tabs = new ArrayList<Tab>();
    record.clientName = clientName;

    record.androidID = -1;
    record.deleted = false;

    record.lastModified = 0;

    int position = cursor.getPosition();
    try {
      cursor.moveToFirst();
      while (!cursor.isAfterLast()) {
        final Tab tab = Tab.fromCursor(cursor);
        record.tabs.add(tab);

        if (tab.lastUsed > record.lastModified) {
          record.lastModified = tab.lastUsed;
        }

        cursor.moveToNext();
      }
    } finally {
      cursor.moveToPosition(position);
    }

    return record;
  }

  






  public static void deleteNonLocalClientsAndTabs(Context context) {
    final String nonLocalTabsSelection = BrowserContract.Tabs.CLIENT_GUID + " IS NOT NULL";

    ContentProviderClient tabsProvider = context.getContentResolver()
            .acquireContentProviderClient(BrowserContractHelpers.TABS_CONTENT_URI);
    if (tabsProvider == null) {
        Logger.warn(LOG_TAG, "Unable to create tabsProvider!");
        return;
    }

    try {
      Logger.info(LOG_TAG, "Clearing all non-local tabs for default profile.");
      tabsProvider.delete(BrowserContractHelpers.TABS_CONTENT_URI, nonLocalTabsSelection, null);
    } catch (RemoteException e) {
      Logger.warn(LOG_TAG, "Error while deleting", e);
    } finally {
      try {
        tabsProvider.release();
      } catch (Exception e) {
        Logger.warn(LOG_TAG, "Got exception releasing tabsProvider!", e);
      }
    }
  }
}
