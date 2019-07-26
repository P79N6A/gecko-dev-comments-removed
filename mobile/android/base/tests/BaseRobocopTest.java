



package org.mozilla.gecko.tests;

import java.util.Map;

import org.mozilla.gecko.Assert;
import org.mozilla.gecko.FennecInstrumentationTestRunner;
import org.mozilla.gecko.FennecMochitestAssert;
import org.mozilla.gecko.FennecNativeDriver;
import org.mozilla.gecko.FennecTalosAssert;

import android.app.Activity;
import android.test.ActivityInstrumentationTestCase2;

public abstract class BaseRobocopTest extends ActivityInstrumentationTestCase2<Activity> {
    public static final int TEST_MOCHITEST = 0;
    public static final int TEST_TALOS = 1;

    protected static final String TARGET_PACKAGE_ID = "org.mozilla.gecko";
    protected Assert mAsserter;
    protected String mLogFile;

    protected Map<?, ?> mConfig;
    protected String mRootPath;

    public BaseRobocopTest(Class<Activity> activityClass) {
        super(activityClass);
    }

    @SuppressWarnings("deprecation")
    public BaseRobocopTest(String targetPackageId, Class<Activity> activityClass) {
        super(targetPackageId, activityClass);
    }

    protected abstract int getTestType();

    @Override
    protected void setUp() throws Exception {
        
        mRootPath = FennecInstrumentationTestRunner.getFennecArguments().getString("deviceroot");
        String configFile = FennecNativeDriver.getFile(mRootPath + "/robotium.config");
        mConfig = FennecNativeDriver.convertTextToTable(configFile);
        mLogFile = (String) mConfig.get("logfile");

        
        if (getTestType() == TEST_TALOS) {
            mAsserter = new FennecTalosAssert();
        } else {
            mAsserter = new FennecMochitestAssert();
        }
        mAsserter.setLogFile(mLogFile);
        mAsserter.setTestName(this.getClass().getName());
    }
}