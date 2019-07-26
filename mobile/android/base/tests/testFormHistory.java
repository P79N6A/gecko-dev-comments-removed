package org.mozilla.gecko.tests;

import android.content.ContentValues;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import java.io.File;
import java.lang.ClassLoader;








public class testFormHistory extends BaseTest {
    private static final String DB_NAME = "formhistory.sqlite";

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testFormHistory() {
        Context context = (Context)getActivity();
        ContentResolver cr = context.getContentResolver();
        ContentValues[] cvs = new ContentValues[1];
        cvs[0] = new ContentValues();
 
        blockForGeckoReady();

        Uri formHistoryUri;
        Uri insertUri;
        Uri expectedUri;
        int numUpdated;
        int numDeleted;

        try {
            ClassLoader classLoader = getActivity().getClassLoader();
            Class fh = classLoader.loadClass("org.mozilla.gecko.db.BrowserContract$FormHistory");
      
            cvs[0].put("fieldname", "fieldname");
            cvs[0].put("value", "value");
            cvs[0].put("timesUsed", "0");
            cvs[0].put("guid", "guid");
    
            
            formHistoryUri = (Uri)fh.getField("CONTENT_URI").get(null);
            Uri.Builder builder = formHistoryUri.buildUpon();
            formHistoryUri = builder.appendQueryParameter("profilePath", mProfile).build();
        } catch(ClassNotFoundException ex) {
            mAsserter.is(false, true, "Error getting class");
            return;
        } catch(NoSuchFieldException ex) {
            mAsserter.is(false, true, "Error getting field");
            return;
        } catch(IllegalAccessException ex) {
            mAsserter.is(false, true, "Error using field");
            return;
        }

        insertUri = cr.insert(formHistoryUri, cvs[0]);
        expectedUri = formHistoryUri.buildUpon().appendPath("1").build();
        mAsserter.is(expectedUri.toString(), insertUri.toString(), "Insert returned correct uri");
        SqliteCompare(DB_NAME, "SELECT * FROM moz_formhistory", cvs);
  
        cvs[0].put("fieldname", "fieldname2");
        cvs[0].putNull("guid");
  
        numUpdated = cr.update(formHistoryUri, cvs[0], null, null);
        mAsserter.is(1, numUpdated, "Correct number updated");
        SqliteCompare(DB_NAME, "SELECT * FROM moz_formhistory", cvs);
  
        numDeleted = cr.delete(formHistoryUri, null, null);
        mAsserter.is(1, numDeleted, "Correct number deleted");
        cvs = new ContentValues[0];
        SqliteCompare(DB_NAME, "SELECT * FROM moz_formhistory", cvs);
        
        cvs = new ContentValues[1];
        cvs[0] = new ContentValues();
        cvs[0].put("fieldname", "fieldname");
        cvs[0].put("value", "value");
        cvs[0].put("timesUsed", "0");
        cvs[0].putNull("guid");

        insertUri = cr.insert(formHistoryUri, cvs[0]);
        expectedUri = formHistoryUri.buildUpon().appendPath("1").build();
        mAsserter.is(expectedUri.toString(), insertUri.toString(), "Insert returned correct uri");
        SqliteCompare(DB_NAME, "SELECT * FROM moz_formhistory", cvs);
  
        cvs[0].put("guid", "guid");
  
        numUpdated = cr.update(formHistoryUri, cvs[0], null, null);
        mAsserter.is(1, numUpdated, "Correct number updated");
        SqliteCompare(DB_NAME, "SELECT * FROM moz_formhistory", cvs);
 
        numDeleted = cr.delete(formHistoryUri, null, null);
        mAsserter.is(1, numDeleted, "Correct number deleted");
        cvs = new ContentValues[0];
        SqliteCompare(DB_NAME, "SELECT * FROM moz_formhistory", cvs);
    }

    @Override
    public void tearDown() throws Exception {
        
        File profile = new File(mProfile);
        File db = new File(profile, "formhistory.sqlite");
        if (db.delete()) {
            mAsserter.dumpLog("tearDown deleted "+db.toString());
        } else {
            mAsserter.dumpLog("tearDown did not delete "+db.toString());
        }

        super.tearDown();
    }
}
