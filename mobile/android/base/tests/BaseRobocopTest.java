



package org.mozilla.gecko.tests;

import java.util.Map;

import org.mozilla.gecko.Assert;
import org.mozilla.gecko.FennecInstrumentationTestRunner;
import org.mozilla.gecko.FennecMochitestAssert;
import org.mozilla.gecko.FennecNativeDriver;
import org.mozilla.gecko.FennecTalosAssert;

import org.mozilla.gecko.AppConstants;

import android.app.Activity;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

public abstract class BaseRobocopTest extends ActivityInstrumentationTestCase2<Activity> {
    public enum Type {
        MOCHITEST,
        TALOS
    }

    private static final String DEFAULT_ROOT_PATH = "/mnt/sdcard/tests";

    




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

    protected Assert mAsserter;
    protected String mLogFile;

    protected Map<String, String> mConfig;
    protected String mRootPath;

    








    @SuppressWarnings("unchecked")
    public BaseRobocopTest() {
        this((Class<Activity>) BROWSER_INTENT_CLASS);
    }

    






    protected BaseRobocopTest(Class<Activity> activityClass) {
        super(activityClass);
    }

    





    protected Type getTestType() {
        return Type.MOCHITEST;
    }

    @Override
    protected void setUp() throws Exception {
        
        mRootPath = FennecInstrumentationTestRunner.getFennecArguments().getString("deviceroot");
        if (mRootPath == null) {
            Log.w("Robocop", "Did not find deviceroot in arguments; falling back to: " + DEFAULT_ROOT_PATH);
            mRootPath = DEFAULT_ROOT_PATH;
        }
        String configFile = FennecNativeDriver.getFile(mRootPath + "/robotium.config");
        mConfig = FennecNativeDriver.convertTextToTable(configFile);
        mLogFile = mConfig.get("logfile");

        
        if (getTestType() == Type.TALOS) {
            mAsserter = new FennecTalosAssert();
        } else {
            mAsserter = new FennecMochitestAssert();
        }
        mAsserter.setLogFile(mLogFile);
        mAsserter.setTestName(getClass().getName());
    }
}
