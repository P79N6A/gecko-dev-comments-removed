



package org.mozilla.gecko.tests;

import java.util.HashSet;
import java.util.Random;
import java.util.concurrent.Callable;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.db.ReadingListProvider;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;

public class testReadingListProvider extends ContentProviderTest {

    private static final String DB_NAME = "browser.db";

    
    private final TestCase[] TESTS_TO_RUN = { new TestInsertItems(),
                                              new TestDeleteItems(),
                                              new TestUpdateItems(),
                                              new TestBatchOperations(),
                                              new TestBrowserProviderNotifications() };

    
    final String[] TEST_COLUMNS = { ReadingListItems.TITLE,
                                    ReadingListItems.URL,
                                    ReadingListItems.EXCERPT,
                                    ReadingListItems.LENGTH,
                                    ReadingListItems.DATE_CREATED };

    
    
    private boolean mContentProviderInsertTested = false;

    
    
    private boolean mContentProviderUpdateTested = false;

    





    private static Callable<ContentProvider> sProviderFactory = new Callable<ContentProvider>() {
        @Override
        public ContentProvider call() {
            return new ReadingListProvider();
        }
    };

    @Override
    public void setUp() throws Exception {
        super.setUp(sProviderFactory, BrowserContract.READING_LIST_AUTHORITY, DB_NAME);
        for (TestCase test: TESTS_TO_RUN) {
            mTests.add(test);
        }
    }

    public void testReadingListProviderTests() throws Exception {
        for (Runnable test : mTests) {
            setTestName(test.getClass().getSimpleName());
            ensureEmptyDatabase();
            test.run();
        }

        
        
        blockForGeckoReady();
    }

    


    private class TestInsertItems extends TestCase {
        @Override
        public void test() throws Exception {
            ContentValues b = createFillerReadingListItem();
            long id = ContentUris.parseId(mProvider.insert(ReadingListItems.CONTENT_URI, b));
            Cursor c = getItemById(id);

            try {
                mAsserter.ok(c.moveToFirst(), "Inserted item found", "");
                assertRowEqualsContentValues(c, b);
            } finally {
                c.close();
            }

            testInsertWithNullCol(ReadingListItems.GUID);
            mContentProviderInsertTested = true;
        }

        



        private void testInsertWithNullCol(String colName) {
            ContentValues b = createFillerReadingListItem();
            b.putNull(colName);

            try {
                ContentUris.parseId(mProvider.insert(ReadingListItems.CONTENT_URI, b));
                
                mAsserter.ok(false, "Insertion did not succeed with " + colName + " == null", "");
            } catch (NullPointerException e) {
                
            }
        }
    }

    


    private class TestDeleteItems extends TestCase {

        @Override
        public void test() throws Exception {
            long id = insertAnItemWithAssertion();
            
            
            testNonFirefoxSyncDelete(id);

            
            testFirefoxSyncDelete(id);

            id = insertAnItemWithAssertion();
            
            testDeleteWithItemURI(id);
        }

        






        private void testNonFirefoxSyncDelete(long id) {
            final int deleted = mProvider.delete(ReadingListItems.CONTENT_URI,
                    ReadingListItems._ID + " = ?",
                    new String[] { String.valueOf(id) });

            mAsserter.is(deleted, 1, "Inserted item was deleted");

            
            
            Uri uri = appendUriParam(ReadingListItems.CONTENT_URI, BrowserContract.PARAM_SHOW_DELETED, "1");
            assertItemExistsByID(uri, id, "Deleted item was only marked as deleted");

            
            
            assertItemDoesNotExistByID(id, "Inserted item can't be found after deletion");
        }

        





        private void testFirefoxSyncDelete(long id) {
            final int deleted = mProvider.delete(appendUriParam(ReadingListItems.CONTENT_URI, BrowserContract.PARAM_IS_SYNC, "1"),
                    ReadingListItems._ID + " = ?",
                    new String[] { String.valueOf(id) });

            mAsserter.is(deleted, 1, "Inserted item was deleted");

            Uri uri = appendUriParam(ReadingListItems.CONTENT_URI, BrowserContract.PARAM_SHOW_DELETED, "1");
            assertItemDoesNotExistByID(uri, id, "Inserted item is now actually deleted");
        }

        





        private void testDeleteWithItemURI(long id) {
            final int deleted = mProvider.delete(ContentUris.withAppendedId(ReadingListItems.CONTENT_URI, id), null, null);
            mAsserter.is(deleted, 1, "Inserted item was deleted using URI with id");
        }
    }

    


    private class TestUpdateItems extends TestCase {

        @Override
        public void test() throws Exception {
            
            ensureCanInsert();

            ContentValues original = createFillerReadingListItem();
            long id = ContentUris.parseId(mProvider.insert(ReadingListItems.CONTENT_URI, original));
            int updated = 0;
            Long originalDateCreated = null;
            Long originalDateModified = null;
            ContentValues updates = new ContentValues();
            Cursor c = getItemById(id);
            try {
                mAsserter.ok(c.moveToFirst(), "Inserted item found", "");

                originalDateCreated = c.getLong(c.getColumnIndex(ReadingListItems.DATE_CREATED));
                originalDateModified = c.getLong(c.getColumnIndex(ReadingListItems.DATE_MODIFIED));

                updates.put(ReadingListItems.TITLE, original.getAsString(ReadingListItems.TITLE) + "CHANGED");
                updates.put(ReadingListItems.URL, original.getAsString(ReadingListItems.URL) + "/more/stuff");
                updates.put(ReadingListItems.EXCERPT, original.getAsString(ReadingListItems.EXCERPT) + "CHANGED");

                updated = mProvider.update(ReadingListItems.CONTENT_URI, updates,
                                               ReadingListItems._ID + " = ?",
                                               new String[] { String.valueOf(id) });

                mAsserter.is(updated, 1, "Inserted item was updated");
            } finally {
                c.close();
            }

            
            
            ContentValues expectedValues = updates;
            c = getItemById(id);
            try {
                mAsserter.ok(c.moveToFirst(), "Updated item found", "");
                mAsserter.isnot(c.getLong(c.getColumnIndex(ReadingListItems.DATE_MODIFIED)),
                originalDateModified,
                "Date modified should have changed");

                
                expectedValues.put(ReadingListItems.DATE_CREATED, originalDateCreated);
                expectedValues.put(ReadingListItems.LENGTH, original.getAsString(ReadingListItems.LENGTH));
                assertRowEqualsContentValues(c, expectedValues,  false);
            } finally {
                c.close();
            }

            
            testUpdateWithInvalidID();

            
            testUpdateWithNullCol(id, ReadingListItems.GUID);

            mContentProviderUpdateTested = true;
        }

        





        private void testUpdateWithInvalidID() {
            ensureEmptyDatabase();
            final ContentValues b = createFillerReadingListItem();
            final long id = ContentUris.parseId(mProvider.insert(ReadingListItems.CONTENT_URI, b));
            final long INVALID_ID = id + 1;
            final ContentValues updates = new ContentValues();
            updates.put(ReadingListItems.TITLE, b.getAsString(ReadingListItems.TITLE) + "CHANGED");
            final int updated = mProvider.update(ReadingListItems.CONTENT_URI, updates,
                                               ReadingListItems._ID + " = ?",
                                               new String[] { String.valueOf(INVALID_ID) });
            mAsserter.is(updated, 0, "Should not be able to update item with an invalid GUID");
        }

        


        private int testUpdateWithNullCol(long id, String colName) {
            ContentValues updates = new ContentValues();
            updates.putNull(colName);

            int updated = mProvider.update(ReadingListItems.CONTENT_URI, updates,
                                           ReadingListItems._ID + " = ?",
                                           new String[] { String.valueOf(id) });

            mAsserter.is(updated, 0, "Should not be able to update item with " + colName + " == null ");
            return updated;
        }
    }

    private class TestBatchOperations extends TestCase {
        private static final int ITEM_COUNT = 10;

        



        private void testBulkInsert() {
            ensureEmptyDatabase();
            final ContentValues allVals[] = new ContentValues[ITEM_COUNT];
            final HashSet<String> urls = new HashSet<String>();
            for (int i = 0; i < ITEM_COUNT; i++) {
                final String url =  "http://www.test.org/" + i;
                allVals[i] = new ContentValues();
                allVals[i].put(ReadingListItems.TITLE, "Test" + i);
                allVals[i].put(ReadingListItems.URL, url);
                allVals[i].put(ReadingListItems.EXCERPT, "EXCERPT" + i);
                allVals[i].put(ReadingListItems.LENGTH, i);
                urls.add(url);
            }

            int inserts = mProvider.bulkInsert(ReadingListItems.CONTENT_URI, allVals);
            mAsserter.is(inserts, ITEM_COUNT, "Excepted number of inserts matches");

            Cursor c = mProvider.query(ReadingListItems.CONTENT_URI, null,
                               null,
                               null,
                               null);
            try {
                while (c.moveToNext()) {
                    final String url = c.getString(c.getColumnIndex(ReadingListItems.URL));
                    mAsserter.ok(urls.contains(url), "Bulk inserted item with url == " + url + " was found in the DB", "");
                    
                    urls.remove(url);
                }
            } finally {
                c.close();
            }
        }

        @Override
        public void test() {
            testBulkInsert();
        }
    }

    





    private class TestBrowserProviderNotifications extends TestCase {

        @Override
        public void test() {
            
            ensureCanInsert();
            
            ensureCanUpdate();

            final String CONTENT_URI = ReadingListItems.CONTENT_URI.toString();

            mResolver.notifyChangeList.clear();

            
            final ContentValues h = createFillerReadingListItem();
            long id = ContentUris.parseId(mProvider.insert(ReadingListItems.CONTENT_URI, h));

            mAsserter.isnot(id,
                            -1L,
                            "Inserted item has valid id");

            ensureOnlyChangeNotifiedStartsWith(CONTENT_URI, "insert");

            
            mResolver.notifyChangeList.clear();
            h.put(ReadingListItems.TITLE, "http://newexample.com");

            long numUpdated = mProvider.update(ReadingListItems.CONTENT_URI, h,
                                               ReadingListItems._ID + " = ?",
                                               new String[] { String.valueOf(id) });

            mAsserter.is(numUpdated,
                         1L,
                         "Correct number of items are updated");

            ensureOnlyChangeNotifiedStartsWith(CONTENT_URI, "update");

            
            mResolver.notifyChangeList.clear();
            long numDeleted = mProvider.delete(ReadingListItems.CONTENT_URI, null, null);

            mAsserter.is(numDeleted,
                         1L,
                         "Correct number of items are deleted");

            ensureOnlyChangeNotifiedStartsWith(CONTENT_URI, "delete");

            
            mResolver.notifyChangeList.clear();
            final ContentValues[] hs = { createFillerReadingListItem(),
                                         createFillerReadingListItem(),
                                         createFillerReadingListItem() };

            long numBulkInserted = mProvider.bulkInsert(ReadingListItems.CONTENT_URI, hs);

            mAsserter.is(numBulkInserted,
                         3L,
                         "Correct number of items are bulkInserted");

            ensureOnlyChangeNotifiedStartsWith(CONTENT_URI, "bulkInsert");
        }

        protected void ensureOnlyChangeNotifiedStartsWith(String expectedUri, String operation) {
            mAsserter.is(Long.valueOf(mResolver.notifyChangeList.size()),
                         1L,
                         "Content observer was notified exactly once by " + operation);

            final Uri uri = mResolver.notifyChangeList.poll();

            mAsserter.isnot(uri,
                            null,
                            "Notification from " + operation + " was valid");

            mAsserter.ok(uri.toString().startsWith(expectedUri),
                         "Content observer was notified exactly once by " + operation,
                         "");
        }
    }

    


    private void ensureEmptyDatabase() {
        Uri uri = appendUriParam(ReadingListItems.CONTENT_URI, BrowserContract.PARAM_IS_SYNC, "1");
        getWritableDatabase(uri).delete(ReadingListItems.TABLE_NAME, null, null);
    }


    private SQLiteDatabase getWritableDatabase(Uri uri) {
        Uri testUri = appendUriParam(uri, BrowserContract.PARAM_IS_TEST, "1");
        DelegatingTestContentProvider delegateProvider = (DelegatingTestContentProvider) mProvider;
        ReadingListProvider readingListProvider = (ReadingListProvider) delegateProvider.getTargetProvider();
        return readingListProvider.getWritableDatabaseForTesting(testUri);
    }

    






    private void assertRowEqualsContentValues(Cursor cursorWithActual, ContentValues expectedValues, boolean compareDateModified) {
        for (String column: TEST_COLUMNS) {
            String expected = expectedValues.getAsString(column);
            String actual = cursorWithActual.getString(cursorWithActual.getColumnIndex(column));
            mAsserter.is(actual, expected, "Item has correct " + column);
        }

        if (compareDateModified) {
            String expected = expectedValues.getAsString(ReadingListItems.DATE_MODIFIED);
            String actual = cursorWithActual.getString(cursorWithActual.getColumnIndex(ReadingListItems.DATE_MODIFIED));
            mAsserter.is(actual, expected, "Item has correct " + ReadingListItems.DATE_MODIFIED);
        }
    }

    private void assertRowEqualsContentValues(Cursor cursorWithActual, ContentValues expectedValues) {
        assertRowEqualsContentValues(cursorWithActual, expectedValues, true);
    }

    private ContentValues fillContentValues(String title, String url, String excerpt) {
        ContentValues values = new ContentValues();

        values.put(ReadingListItems.TITLE, title);
        values.put(ReadingListItems.URL, url);
        values.put(ReadingListItems.EXCERPT, excerpt);
        values.put(ReadingListItems.LENGTH, excerpt.length());

        return values;
    }

    private ContentValues createFillerReadingListItem() {
        Random rand = new Random();
        return fillContentValues("Example", "http://example.com/?num=" + rand.nextInt(), "foo bar");
    }

    private Cursor getItemById(Uri uri, long id, String[] projection) {
        return mProvider.query(uri, projection,
                               ReadingListItems._ID + " = ?",
                               new String[] { String.valueOf(id) },
                               null);
    }

    private Cursor getItemById(long id) {
        return getItemById(ReadingListItems.CONTENT_URI, id, null);
    }

    private Cursor getItemById(Uri uri, long id) {
        return getItemById(uri, id, null);
    }

    


    private void ensureCanInsert() {
        if (!mContentProviderInsertTested) {
            mAsserter.ok(false, "ContentProvider insertions have not been tested yet.", "");
        }
    }

    


    private void ensureCanUpdate() {
        if (!mContentProviderUpdateTested) {
            mAsserter.ok(false, "ContentProvider updates have not been tested yet.", "");
        }
    }

    private long insertAnItemWithAssertion() {
        
        ensureCanInsert();

        ContentValues v = createFillerReadingListItem();
        long id = ContentUris.parseId(mProvider.insert(ReadingListItems.CONTENT_URI, v));

        assertItemExistsByID(id, "Inserted item found");
        return id;
    }

    private void assertItemExistsByID(Uri uri, long id, String msg) {
        Cursor c = getItemById(uri, id);
        try {
            mAsserter.ok(c.moveToFirst(), msg, "");
        } finally {
            c.close();
        }
    }

    private void assertItemExistsByID(long id, String msg) {
        Cursor c = getItemById(id);
        try {
            mAsserter.ok(c.moveToFirst(), msg, "");
        } finally {
            c.close();
        }
    }

    private void assertItemDoesNotExistByID(long id, String msg) {
        Cursor c = getItemById(id);
        try {
            mAsserter.ok(!c.moveToFirst(), msg, "");
        } finally {
            c.close();
        }
    }

    private void assertItemDoesNotExistByID(Uri uri, long id, String msg) {
        Cursor c = getItemById(uri, id);
        try {
            mAsserter.ok(!c.moveToFirst(), msg, "");
        } finally {
            c.close();
        }
    }
}
