



package org.mozilla.gecko.tests;

import java.util.Map;

import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.mozilla.gecko.Actions;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.Driver;
import org.mozilla.gecko.FennecInstrumentationTestRunner;
import org.mozilla.gecko.FennecMochitestAssert;
import org.mozilla.gecko.FennecNativeActions;
import org.mozilla.gecko.FennecNativeDriver;
import org.mozilla.gecko.FennecTalosAssert;
import org.mozilla.gecko.updater.UpdateServiceHelper;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.PowerManager;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import com.jayway.android.robotium.solo.Solo;

@SuppressWarnings("unchecked")
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

    protected String mBaseHostnameUrl;
    protected String mBaseIpUrl;

    protected Map<String, String> mConfig;
    protected String mRootPath;

    protected Solo mSolo;
    protected Driver mDriver;
    protected Actions mActions;

    protected String mProfile;

    protected StringHelper mStringHelper;

    protected abstract Intent createActivityIntent();

    








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
        
        UpdateServiceHelper.setEnabled(false);

        
        mRootPath = FennecInstrumentationTestRunner.getFennecArguments().getString("deviceroot");
        if (mRootPath == null) {
            Log.w("Robocop", "Did not find deviceroot in arguments; falling back to: " + DEFAULT_ROOT_PATH);
            mRootPath = DEFAULT_ROOT_PATH;
        }
        String configFile = FennecNativeDriver.getFile(mRootPath + "/robotium.config");
        mConfig = FennecNativeDriver.convertTextToTable(configFile);
        mLogFile = mConfig.get("logfile");
        mProfile = mConfig.get("profile");
        mBaseHostnameUrl = mConfig.get("host").replaceAll("(/$)", "");
        mBaseIpUrl = mConfig.get("rawhost").replaceAll("(/$)", "");

        
        if (getTestType() == Type.TALOS) {
            mAsserter = new FennecTalosAssert();
        } else {
            mAsserter = new FennecMochitestAssert();
        }
        mAsserter.setLogFile(mLogFile);
        mAsserter.setTestName(getClass().getName());

        
        final Intent intent = createActivityIntent();
        setActivityIntent(intent);

        
        Activity tempActivity = getActivity();

        StringHelper.initialize(tempActivity.getResources());
        mStringHelper = StringHelper.get();

        mSolo = new Solo(getInstrumentation(), tempActivity);
        mDriver = new FennecNativeDriver(tempActivity, mSolo, mRootPath);
        mActions = new FennecNativeActions(tempActivity, mSolo, getInstrumentation(), mAsserter);

    }

    




    public void throwIfHttpGetFails() {
        if (getTestType() == Type.TALOS) {
            return;
        }

        
        
        
        final String rawUrl = ((String) mConfig.get("rawhost")).replaceAll("(/$)", "");

        try {
            final HttpClient httpclient = new DefaultHttpClient();
            final HttpResponse response = httpclient.execute(new HttpGet(rawUrl));
            final int statusCode = response.getStatusLine().getStatusCode();
            if (200 != statusCode) {
                throw new IllegalStateException("Status code: " + statusCode);
            }
        } catch (Exception e) {
            mAsserter.ok(false, "Robocop tests on your device need network/wifi access to reach: [" + rawUrl + "].", e.toString());
        }
    }

    


    public void throwIfScreenNotOn() {
        final PowerManager pm = (PowerManager) getActivity().getSystemService(Context.POWER_SERVICE);
        mAsserter.ok(pm.isScreenOn(),
            "Robocop tests need the test device screen to be powered on.", "");
    }
}
