



package org.mozilla.gecko.tests;

import java.util.Map;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.FennecInstrumentationTestRunner;
import org.mozilla.gecko.FennecMochitestAssert;
import org.mozilla.gecko.FennecNativeDriver;
import org.mozilla.gecko.FennecTalosAssert;

import android.app.Activity;
import android.test.ActivityInstrumentationTestCase2;

public abstract class BaseRobocopTest extends ActivityInstrumentationTestCase2<Activity> {
    public enum Type {
        MOCHITEST,
        TALOS
    }

    protected Assert mAsserter;
    protected String mLogFile;

    protected Map<String, String> mConfig;
    protected String mRootPath;

    








    @SuppressWarnings("unchecked")
    public BaseRobocopTest() {
        this((Class<Activity>) AppConstants.BROWSER_INTENT_CLASS);
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
        String configFile = FennecNativeDriver.getFile(mRootPath + "/robotium.config");
        mConfig = FennecNativeDriver.convertTextToTable(configFile);
        mLogFile = (String) mConfig.get("logfile");

        
        if (getTestType() == Type.TALOS) {
            mAsserter = new FennecTalosAssert();
        } else {
            mAsserter = new FennecMochitestAssert();
        }
        mAsserter.setLogFile(mLogFile);
        mAsserter.setTestName(this.getClass().getName());
    }
}
