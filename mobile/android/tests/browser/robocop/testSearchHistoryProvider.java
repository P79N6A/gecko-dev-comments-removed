



package org.mozilla.gecko.tests;

import java.util.concurrent.Callable;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.gecko.db.SearchHistoryProvider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

public class testSearchHistoryProvider extends ContentProviderTest {

    
    private static final String[] testStrings = {
            "An Ríocht Aontaithe", 
            "Angli", 
            "Britanniarum Regnum", 
            "Britio", 
            "Büyük Britanya", 
            "Egyesült Királyság", 
            "Erresuma Batua", 
            "Inggris Raya", 
            "Ir-Renju Unit", 
            "Iso-Britannia", 
            "Jungtinė Karalystė", 
            "Lielbritānija", 
            "Regatul Unit", 
            "Regne Unit", 
            "Regno Unito", 
            "Royaume-Uni", 
            "Spojené království", 
            "Spojené kráľovstvo", 
            "Storbritannia", 
            "Storbritannien", 
            "Suurbritannia", 
            "Ujedinjeno Kraljevstvo", 
            "United Alaeze", 
            "United Kingdom", 
            "Vereinigtes Königreich", 
            "Verenigd Koninkrijk", 
            "Verenigde Koninkryk", 
            "Vương quốc Anh", 
            "Wayòm Ini", 
            "Y Deyrnas Unedig", 
            "Združeno kraljestvo", 
            "Zjednoczone Królestwo", 
            "Ηνωμένο Βασίλειο", 
            "Великобритания", 
            "Нэгдсэн Вант Улс", 
            "Обединетото Кралство", 
            "Уједињено Краљевство", 
            "Միացյալ Թագավորություն", 
            "בריטניה", 
            "פֿאַראייניקטע מלכות", 
            "المملكة المتحدة", 
            "برطانیہ", 
            "پادشاهی متحده", 
            "यूनाइटेड किंगडम", 
            "संयुक्त राज्य", 
            "যুক্তরাজ্য", 
            "યુનાઇટેડ કિંગડમ", 
            "ஐக்கிய ராஜ்யம்", 
            "สหราชอาณาจักร", 
            "ສະ​ຫະ​ປະ​ຊາ​ຊະ​ອາ​ນາ​ຈັກ", 
            "გაერთიანებული სამეფო", 
            "イギリス", 
            "联合王国" 
    };


    private static final String DB_NAME = "searchhistory.db";

    





    private static final Callable<ContentProvider> sProviderFactory =
            new Callable<ContentProvider>() {
                @Override
                public ContentProvider call() {
                    return new SearchHistoryProvider();
                }
            };

    @Override
    public void setUp() throws Exception {
        super.setUp(sProviderFactory, BrowserContract.SEARCH_HISTORY_AUTHORITY, DB_NAME);
        mTests.add(new TestInsert());
        mTests.add(new TestUnicodeQuery());
        mTests.add(new TestTimestamp());
        mTests.add(new TestLimit());
        mTests.add(new TestDelete());
        mTests.add(new TestIncrement());
    }

    public void testSearchHistory() throws Exception {
        for (Runnable test : mTests) {
            String testName = test.getClass().getSimpleName();
            setTestName(testName);
            mAsserter.dumpLog(
                    "testBrowserProvider: Database empty - Starting " + testName + ".");
            
            mProvider.delete(SearchHistory.CONTENT_URI, null, null);
            test.run();
        }
    }

    


    private class TestLimit extends TestCase {
        @Override
        public void test() throws Exception {
            ContentValues cv;
            for (int i = 0; i < testStrings.length; i++) {
                cv = new ContentValues();
                cv.put(SearchHistory.QUERY, testStrings[i]);
                mProvider.insert(SearchHistory.CONTENT_URI, cv);
            }

            final int limit = 5;

            

            Uri uri = SearchHistory.CONTENT_URI
                                   .buildUpon()
                                   .appendQueryParameter(BrowserContract.PARAM_LIMIT, String.valueOf(limit))
                                   .build();

            Cursor c = mProvider.query(uri, null, null, null, null);
            try {
                mAsserter.is(c.getCount(), limit,
                             String.format("Should have %d results", limit));
            } finally {
                c.close();
            }

            

            uri = SearchHistory.CONTENT_URI
                                   .buildUpon()
                                   .appendQueryParameter(BrowserContract.PARAM_LIMIT, "")
                                   .build();

            c = mProvider.query(uri, null, null, null, null);
            try {
                mAsserter.is(c.getCount(), testStrings.length, "Should have all results");
            } finally {
                c.close();
            }

            

            String[] illegalParams = new String[] {"a", "-1"};
            boolean success = true;

            for (String param : illegalParams) {
                success = true;

                uri = SearchHistory.CONTENT_URI
                                   .buildUpon()
                                   .appendQueryParameter(BrowserContract.PARAM_LIMIT, param)
                                   .build();

                try {
                    c = mProvider.query(uri, null, null, null, null);
                    success = false;
                } catch(IllegalArgumentException e) {
                    
                } finally {
                    if (c != null) {
                        c.close();
                    }
                }

                mAsserter.ok(success, "LIMIT", param + " should have been an invalid argument");
            }

        }
    }

    


    private class TestInsert extends TestCase {
        @Override
        public void test() throws Exception {
            ContentValues cv;
            for (int i = 0; i < testStrings.length; i++) {
                cv = new ContentValues();
                cv.put(SearchHistory.QUERY, testStrings[i]);
                mProvider.insert(SearchHistory.CONTENT_URI, cv);
            }

            final Cursor c = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                mAsserter.is(c.getCount(), testStrings.length,
                             "Should have one row for each insert");
            } finally {
                c.close();
            }
        }
    }

    


    private class TestUnicodeQuery extends TestCase {
        @Override
        public void test() throws Exception {
            final String selection = SearchHistory.QUERY + " = ?";

            for (int i = 0; i < testStrings.length; i++) {
                final ContentValues cv = new ContentValues();
                cv.put(SearchHistory.QUERY, testStrings[i]);
                mProvider.insert(SearchHistory.CONTENT_URI, cv);

                final Cursor c = mProvider.query(SearchHistory.CONTENT_URI, null, selection,
                                                 new String[]{ testStrings[i] }, null);
                try {
                    mAsserter.is(c.getCount(), 1,
                                 "Should have one row for insert of " + testStrings[i]);
                } finally {
                    c.close();
                }
            }
        }
    }

    


    private class TestTimestamp extends TestCase {
        @Override
        public void test() throws Exception {
            String insertedTerm = "Courtside Seats";
            long insertStart;
            long insertFinish;
            long t1Db;
            long t2Db;

            ContentValues cv = new ContentValues();
            cv.put(SearchHistory.QUERY, insertedTerm);

            
            
            insertStart = System.currentTimeMillis();
            mProvider.insert(SearchHistory.CONTENT_URI, cv);
            insertFinish = System.currentTimeMillis();

            final Cursor c1 = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                c1.moveToFirst();
                t1Db = c1.getLong(c1.getColumnIndex(SearchHistory.DATE_LAST_VISITED));
            } finally {
                c1.close();
            }

            mAsserter.dumpLog("First insert:");
            mAsserter.dumpLog("  insertStart " + insertStart);
            mAsserter.dumpLog("  insertFinish " + insertFinish);
            mAsserter.dumpLog("  t1Db " + t1Db);
            mAsserter.ok(t1Db >= insertStart, "DATE_LAST_VISITED",
                         "Date last visited should be set on insert.");
            mAsserter.ok(t1Db <= insertFinish, "DATE_LAST_VISITED",
                         "Date last visited should be set on insert.");

            cv = new ContentValues();
            cv.put(SearchHistory.QUERY, insertedTerm);

            insertStart = System.currentTimeMillis();
            mProvider.insert(SearchHistory.CONTENT_URI, cv);
            insertFinish = System.currentTimeMillis();

            final Cursor c2 = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                c2.moveToFirst();
                t2Db = c2.getLong(c2.getColumnIndex(SearchHistory.DATE_LAST_VISITED));
            } finally {
                c2.close();
            }

            mAsserter.dumpLog("Second insert:");
            mAsserter.dumpLog("  insertStart " + insertStart);
            mAsserter.dumpLog("  insertFinish " + insertFinish);
            mAsserter.dumpLog("  t2Db " + t2Db);

            mAsserter.ok(t2Db >= insertStart, "DATE_LAST_VISITED",
                         "Date last visited should be set on insert.");
            mAsserter.ok(t2Db <= insertFinish, "DATE_LAST_VISITED",
                         "Date last visited should be set on insert.");
            mAsserter.ok(t2Db >= t1Db, "DATE_LAST_VISITED",
                         "Date last visited should be updated on key increment.");
        }
    }

    


    private class TestDelete extends TestCase {
        @Override
        public void test() throws Exception {
            String insertedTerm = "Courtside Seats";

            ContentValues cv = new ContentValues();
            cv.put(SearchHistory.QUERY, insertedTerm);
            mProvider.insert(SearchHistory.CONTENT_URI, cv);

            final Cursor c1 = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                mAsserter.is(c1.getCount(), 1, "Should have one value");
                mProvider.delete(SearchHistory.CONTENT_URI, null, null);
            } finally {
                c1.close();
            }

            final Cursor c2 = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                mAsserter.is(c2.getCount(), 0, "Should be empty");
                mProvider.insert(SearchHistory.CONTENT_URI, cv);
            } finally {
                c2.close();
            }

            final Cursor c3 = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                mAsserter.is(c3.getCount(), 1, "Should have one value");
            } finally {
                c3.close();
            }
        }
    }


    


    private class TestIncrement extends TestCase {
        @Override
        public void test() throws Exception {
            ContentValues cv = new ContentValues();
            cv.put(SearchHistory.QUERY, "omaha");
            mProvider.insert(SearchHistory.CONTENT_URI, cv);

            cv = new ContentValues();
            cv.put(SearchHistory.QUERY, "omaha");
            mProvider.insert(SearchHistory.CONTENT_URI, cv);

            Cursor c = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                c.moveToFirst();
                mAsserter.is(c.getCount(), 1, "Should have one result");
                mAsserter.is(c.getInt(c.getColumnIndex(SearchHistory.VISITS)), 2,
                             "Counter should be 2");
            } finally {
                c.close();
            }

            cv = new ContentValues();
            cv.put(SearchHistory.QUERY, "Omaha");
            mProvider.insert(SearchHistory.CONTENT_URI, cv);
            c = mProvider.query(SearchHistory.CONTENT_URI, null, null, null, null);
            try {
                mAsserter.is(c.getCount(), 2, "Should have two results");
            } finally {
                c.close();
            }
        }
    }
}
