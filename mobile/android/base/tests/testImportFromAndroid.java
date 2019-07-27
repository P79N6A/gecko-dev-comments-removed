



package org.mozilla.gecko.tests;

import java.util.ArrayList;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoProfile;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.provider.Browser;

import com.jayway.android.robotium.solo.Condition;









public class testImportFromAndroid extends AboutHomeTest {
    private static final int MAX_WAIT_TIMEOUT = 15000;
    ArrayList<String> androidData = new ArrayList<String>();
    ArrayList<String> firefoxHistory = new ArrayList<String>();

    public void testImportFromAndroid() {
        ArrayList<String> firefoxBookmarks = new ArrayList<String>();
        ArrayList<String> oldFirefoxHistory = new ArrayList<String>();
        ArrayList<String> oldFirefoxBookmarks = new ArrayList<String>();
        blockForGeckoReady();

        
        androidData = getAndroidUrls("history");

        
        addData();

        
        oldFirefoxBookmarks = mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.BOOKMARKS);
        oldFirefoxHistory = mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.HISTORY);

        
        importDataFromAndroid();

        
        firefoxHistory = mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.HISTORY);
        firefoxBookmarks = mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.BOOKMARKS);

        



        boolean success = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                if (androidData.size() <= firefoxHistory.size()) {
                    return true;
                } else {
                    return false;
                }
            }
        }, MAX_WAIT_MS);

        



        for (String url:androidData) {
            mAsserter.ok(firefoxHistory.contains(url)||firefoxBookmarks.contains(url), "Checking if Android" + (firefoxBookmarks.contains(url) ? " Bookmark" : " History item") + " is present", url + " was imported");
        }

        
        for (String url:oldFirefoxBookmarks) {
             mAsserter.ok(firefoxBookmarks.contains(url), "Checking if original Firefox Bookmark is present", " Firefox Bookmark " + url + " was not removed");
        }

        
        for (String url:oldFirefoxHistory) {
             mAsserter.ok(firefoxHistory.contains(url), "Checking original Firefox History item is present", " Firefox History item " + url + " was not removed");
        }

        
        importDataFromAndroid();

        
        ArrayList<String> verifiedBookmarks = new ArrayList<String>();
        firefoxBookmarks = mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.BOOKMARKS);
        for (String url:firefoxBookmarks) {
             if (verifiedBookmarks.contains(url)) {
                 mAsserter.ok(false, "Bookmark " + url + " should not be duplicated", "Bookmark is duplicated");
             } else {
                 verifiedBookmarks.add(url);
                 mAsserter.ok(true, "Bookmark " + url + " was not duplicated", "Bookmark is unique");
             }
        }

        
        mAsserter.ok(firefoxHistory.size() == mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.HISTORY).size(), "The number of history entries was not increased", "None of the items were duplicated");
    }

    private void addData() {
        ArrayList<String> androidBookmarks = getAndroidUrls("bookmarks");

        
        for (String url:androidBookmarks) {
            
            if ((androidBookmarks.indexOf(url) % 3) == 0) {
                mDatabaseHelper.addOrUpdateMobileBookmark("Bookmark Number" + String.valueOf(androidBookmarks.indexOf(url)), url);
            }
        }

        
        ContentResolver resolver = getActivity().getContentResolver();
        Uri uri = Uri.parse("content://" + AppConstants.ANDROID_PACKAGE_NAME + ".db.browser/history");
        uri = uri.buildUpon().appendQueryParameter("profile", GeckoProfile.DEFAULT_PROFILE)
                             .appendQueryParameter("sync", "true").build();
        for (String url:androidData) {
            
            if ((androidData.indexOf(url) % 3) == 0) {
                 ContentValues values = new ContentValues();
                 values.put("title", "Page" + url);
                 values.put("url", url);
                 values.put("date", System.currentTimeMillis());
                 values.put("visits", androidData.indexOf(url));
                 resolver.insert(uri, values);
            }
        }
    }

    private void importDataFromAndroid() {
        waitForText(mStringHelper.TITLE_PLACE_HOLDER);
        selectSettingsItem(mStringHelper.CUSTOMIZE_SECTION_LABEL, mStringHelper.IMPORT_FROM_ANDROID_LABEL);

        
        waitForText("Cancel");
        mSolo.clickOnButton("Import");

        
        boolean importComplete = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return !mSolo.searchText("Please wait...");
            }
        }, MAX_WAIT_TIMEOUT);

        mAsserter.ok(importComplete, "Waiting for import to finish and the pop-up to be dismissed", "Import was completed and the pop-up was dismissed");

        
        if ("phone".equals(mDevice.type)) {
            
            waitForText(mStringHelper.IMPORT_FROM_ANDROID_LABEL);
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        }
        waitForText(mStringHelper.PRIVACY_SECTION_LABEL); 
        mActions.sendSpecialKey(Actions.SpecialKey.BACK); 
        
        mAsserter.ok(mSolo.waitForText(mStringHelper.TITLE_PLACE_HOLDER), "Waiting for search bar", "Search bar found");

    }

    public ArrayList<String> getAndroidUrls(String data) {
        
        ArrayList<String> urls = new ArrayList<String>();
        ContentResolver resolver = getActivity().getContentResolver();
        Cursor cursor = null;
        try {
            if (data.equals("history")) {
                cursor = Browser.getAllVisitedUrls(resolver);
            } else if (data.equals("bookmarks")) {
                cursor = Browser.getAllBookmarks(resolver);
            }
            if (cursor != null) {
                cursor.moveToFirst();
                for (int i = 0; i < cursor.getCount(); i++ ) {
                     urls.add(cursor.getString(cursor.getColumnIndex("url")));
                     if(!cursor.isLast()) {
                        cursor.moveToNext();
                     }
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return urls;
    }

    public void deleteImportedData() {
        
        ArrayList<String> androidBookmarks = getAndroidUrls("bookmarks");
        for (String url:androidBookmarks) {
             mDatabaseHelper.deleteBookmark(url);
        }
        
        for (String url:androidData) {
             mDatabaseHelper.deleteHistoryItem(url);
        }
    }

    @Override
    public void tearDown() throws Exception {
        deleteImportedData();
        super.tearDown();
    }
}
