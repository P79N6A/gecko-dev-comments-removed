


package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants;

import android.app.Activity;
import android.content.Context;
import android.test.ActivityInstrumentationTestCase2;




@SuppressWarnings("unchecked")
public class BrowserTestCase extends ActivityInstrumentationTestCase2<Activity> {
    @SuppressWarnings("unused")
    private static String LOG_TAG = "BrowserTestCase";

    




    public static final Class<? extends Activity> BROWSER_INTENT_CLASS;

    
    
    
    static {
        Class<? extends Activity> cl;
        try {
            cl = (Class<? extends Activity>) Class.forName(AppConstants.BROWSER_INTENT_CLASS_NAME);
        } catch (ClassNotFoundException e) {
            
            cl = Activity.class;
        }
        BROWSER_INTENT_CLASS = cl;
    }

    public BrowserTestCase() {
        super((Class<Activity>) BROWSER_INTENT_CLASS);
    }

    public Context getApplicationContext() {
        return this.getInstrumentation().getTargetContext().getApplicationContext();
    }
}
