


package org.mozilla.tests.browser.junit3;

import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.RemoteException;
import android.test.InstrumentationTestCase;

import org.mozilla.gecko.background.db.CursorDumper;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.LocalTabsAccessor;
import org.mozilla.gecko.db.RemoteClient;
import org.mozilla.gecko.db.TabsAccessor;
import org.mozilla.gecko.sync.repositories.android.BrowserContractHelpers;

import java.util.List;

public class TestRemoteTabs extends InstrumentationTestCase {
    public void testGetClientsWithoutTabsByRecencyFromCursor() throws Exception {
        final Uri uri = BrowserContractHelpers.CLIENTS_CONTENT_URI;
        final ContentResolver cr = getInstrumentation().getTargetContext().getContentResolver();
        final ContentProviderClient cpc = cr.acquireContentProviderClient(uri);
        final LocalTabsAccessor accessor = new LocalTabsAccessor("test"); 

        try {
            
            cpc.delete(uri, null, null);
            Cursor allClients = cpc.query(uri, null, null, null, null);
            try {
                assertEquals(0, allClients.getCount());
            } finally {
                allClients.close();
            }

            
            final long now = System.currentTimeMillis();
            
            final ContentValues local = new ContentValues();
            local.put(BrowserContract.Clients.NAME, "local");
            local.put(BrowserContract.Clients.LAST_MODIFIED, now + 1);
            
            final ContentValues remote1 = new ContentValues();
            remote1.put(BrowserContract.Clients.GUID, "guid1");
            remote1.put(BrowserContract.Clients.NAME, "remote1");
            remote1.put(BrowserContract.Clients.LAST_MODIFIED, now + 2);

            final ContentValues remote2 = new ContentValues();
            remote2.put(BrowserContract.Clients.GUID, "guid2");
            remote2.put(BrowserContract.Clients.NAME, "remote2");
            remote2.put(BrowserContract.Clients.LAST_MODIFIED, now + 3);

            ContentValues[] values = new ContentValues[]{local, remote1, remote2};
            int inserted = cpc.bulkInsert(uri, values);
            assertEquals(3, inserted);

            allClients = cpc.query(BrowserContract.Clients.CONTENT_RECENCY_URI, null, null, null, null);
            try {
                CursorDumper.dumpCursor(allClients);
                
                assertEquals(3, allClients.getCount());
                final List<RemoteClient> clients = accessor.getClientsWithoutTabsByRecencyFromCursor(allClients);
                assertEquals(3, clients.size());
                for (RemoteClient client : clients) {
                    
                    assertNotNull(client.tabs);
                    assertEquals(0, client.tabs.size());
                }
                
                assertEquals("guid2", clients.get(0).guid);
                assertEquals("guid1", clients.get(1).guid);
                assertEquals(null, clients.get(2).guid);
            } finally {
                allClients.close();
            }

            
            
            final ContentValues remoteTab1 = new ContentValues();
            remoteTab1.put(BrowserContract.Tabs.CLIENT_GUID, "guid1");
            remoteTab1.put(BrowserContract.Tabs.TITLE, "title1");
            remoteTab1.put(BrowserContract.Tabs.URL, "http://test.com/test1");
            remoteTab1.put(BrowserContract.Tabs.HISTORY, "[\"http://test.com/test1\"]");
            remoteTab1.put(BrowserContract.Tabs.LAST_USED, now);
            remoteTab1.put(BrowserContract.Tabs.POSITION, 0);

            final ContentValues remoteTab2 = new ContentValues();
            remoteTab2.put(BrowserContract.Tabs.CLIENT_GUID, "guid1");
            remoteTab2.put(BrowserContract.Tabs.TITLE, "title2");
            remoteTab2.put(BrowserContract.Tabs.URL, "http://test.com/test2");
            remoteTab2.put(BrowserContract.Tabs.HISTORY, "[\"http://test.com/test2\"]");
            remoteTab2.put(BrowserContract.Tabs.LAST_USED, now + 5);
            remoteTab2.put(BrowserContract.Tabs.POSITION, 1);

            values = new ContentValues[]{remoteTab1, remoteTab2};
            inserted = cpc.bulkInsert(BrowserContract.Tabs.CONTENT_URI, values);
            assertEquals(2, inserted);

            allClients = cpc.query(BrowserContract.Clients.CONTENT_RECENCY_URI, null, BrowserContract.Clients.GUID + " IS NOT NULL", null, null);
            try {
                CursorDumper.dumpCursor(allClients);
                
                assertEquals(2, allClients.getCount());
                final List<RemoteClient> clients = accessor.getClientsWithoutTabsByRecencyFromCursor(allClients);
                assertEquals(2, clients.size());
                for (RemoteClient client : clients) {
                    
                    assertNotNull(client.guid);
                    assertNotNull(client.tabs);
                    assertEquals(0, client.tabs.size());
                }
                
                
                assertEquals("guid1", clients.get(0).guid);
                assertEquals("guid2", clients.get(1).guid);
            } finally {
                allClients.close();
            }
        } finally {
            cpc.release();
        }
    }
}
