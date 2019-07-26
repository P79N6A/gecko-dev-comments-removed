



package org.mozilla.gecko.tests;

import java.util.Map;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.Driver;
import org.mozilla.gecko.FennecNativeActions;
import org.mozilla.gecko.FennecNativeDriver;
import org.mozilla.gecko.tests.components.AboutHomeComponent;
import org.mozilla.gecko.tests.components.AppMenuComponent;
import org.mozilla.gecko.tests.components.BaseComponent;
import org.mozilla.gecko.tests.components.GeckoViewComponent;
import org.mozilla.gecko.tests.components.ToolbarComponent;
import org.mozilla.gecko.tests.helpers.HelperInitializer;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;

import com.jayway.android.robotium.solo.Solo;










abstract class UITest extends BaseRobocopTest
                      implements UITestContext {

    private static final String JUNIT_FAILURE_MSG = "A JUnit method was called. Make sure " +
        "you are using AssertionHelper to make assertions. Try `fAssert*(...);`";

    private Solo mSolo;
    private Driver mDriver;
    private Actions mActions;

    
    private String mBaseHostnameUrl;
    
    private String mBaseIpUrl;

    protected AboutHomeComponent mAboutHome;
    protected AppMenuComponent mAppMenu;
    protected GeckoViewComponent mGeckoView;
    protected ToolbarComponent mToolbar;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        
        final Intent intent = createActivityIntent(mConfig);
        setActivityIntent(intent);
        final Activity activity = getActivity();

        mSolo = new Solo(getInstrumentation(), activity);
        mDriver = new FennecNativeDriver(activity, mSolo, mRootPath);
        mActions = new FennecNativeActions(activity, mSolo, getInstrumentation(), mAsserter);

        mBaseHostnameUrl = ((String) mConfig.get("host")).replaceAll("(/$)", "");
        mBaseIpUrl = ((String) mConfig.get("rawhost")).replaceAll("(/$)", "");

        
        initComponents();
        initHelpers();
    }

    @Override
    public void tearDown() throws Exception {
        try {
            mAsserter.endTest();
            mSolo.finishOpenedActivities();
        } catch (Throwable e) {
            e.printStackTrace();
        }

        super.tearDown();
    }

    private void initComponents() {
        mAboutHome = new AboutHomeComponent(this);
        mAppMenu = new AppMenuComponent(this);
        mGeckoView = new GeckoViewComponent(this);
        mToolbar = new ToolbarComponent(this);
    }

    private void initHelpers() {
        HelperInitializer.init(this);
    }

    @Override
    public Solo getSolo() {
        return mSolo;
    }

    @Override
    public Assert getAsserter() {
        return mAsserter;
    }

    @Override
    public Driver getDriver() {
        return mDriver;
    }

    @Override
    public Actions getActions() {
        return mActions;
    }

    @Override
    public void dumpLog(final String logtag, final String message) {
        mAsserter.dumpLog(logtag + ": " + message);
    }

    @Override
    public void dumpLog(final String logtag, final String message, final Throwable t) {
        mAsserter.dumpLog(logtag + ": " + message, t);
    }

    @Override
    public BaseComponent getComponent(final ComponentType type) {
        switch (type) {
            case ABOUTHOME:
                return mAboutHome;

            case APPMENU:
                return mAppMenu;

            case GECKOVIEW:
                return mGeckoView;

            case TOOLBAR:
                return mToolbar;

            default:
                fail("Unknown component type, " + type + ".");
                return null; 
        }
    }

    



    protected Type getTestType() {
        return Type.MOCHITEST;
    }

    @Override
    public String getAbsoluteHostnameUrl(final String url) {
        return getAbsoluteUrl(mBaseHostnameUrl, url);
    }

    @Override
    public String getAbsoluteIpUrl(final String url) {
        return getAbsoluteUrl(mBaseIpUrl, url);
    }

    private String getAbsoluteUrl(final String baseUrl, final String url) {
        return baseUrl + "/" + url.replaceAll("(^/)", "");
    }

    private static Intent createActivityIntent(final Map<String, String> config) {
        final Intent intent = new Intent(Intent.ACTION_MAIN);

        final String profile = (String) config.get("profile");
        intent.putExtra("args", "-no-remote -profile " + profile);

        final String envString = (String) config.get("envvars");
        if (!TextUtils.isEmpty(envString)) {
            final String[] envStrings = envString.split(",");

            for (int iter = 0; iter < envStrings.length; iter++) {
                intent.putExtra("env" + iter, envStrings[iter]);
            }
        }

        return intent;
    }

    




    private static void junit() {
        throw new UnsupportedOperationException(JUNIT_FAILURE_MSG);
    }

    
    
    public static void assertEquals(short e, short a) { junit(); }
    public static void assertEquals(String m, int e, int a) { junit(); }
    public static void assertEquals(String m, short e, short a) { junit(); }
    public static void assertEquals(char e, char a) { junit(); }
    public static void assertEquals(String m, String e, String a) { junit(); }
    public static void assertEquals(int e, int a) { junit(); }
    public static void assertEquals(String m, double e, double a, double delta) { junit(); }
    public static void assertEquals(String m, long e, long a) { junit(); }
    public static void assertEquals(byte e, byte a) { junit(); }
    public static void assertEquals(Object e, Object a) { junit(); }
    public static void assertEquals(boolean e, boolean a) { junit(); }
    public static void assertEquals(String m, float e, float a, float delta) { junit(); }
    public static void assertEquals(String m, boolean e, boolean a) { junit(); }
    public static void assertEquals(String e, String a) { junit(); }
    public static void assertEquals(float e, float a, float delta) { junit(); }
    public static void assertEquals(String m, byte e, byte a) { junit(); }
    public static void assertEquals(double e, double a, double delta) { junit(); }
    public static void assertEquals(String m, char e, char a) { junit(); }
    public static void assertEquals(String m, Object e, Object a) { junit(); }
    public static void assertEquals(long e, long a) { junit(); }

    public static void assertFalse(String m, boolean c) { junit(); }
    public static void assertFalse(boolean c) { junit(); }

    public static void assertNotNull(String m, Object o) { junit(); }
    public static void assertNotNull(Object o) { junit(); }

    public static void assertNotSame(Object e, Object a) { junit(); }
    public static void assertNotSame(String m, Object e, Object a) { junit(); }

    public static void assertNull(Object o) { junit(); }
    public static void assertNull(String m, Object o) { junit(); }

    public static void assertSame(Object e, Object a) { junit(); }
    public static void assertSame(String m, Object e, Object a) { junit(); }

    public static void assertTrue(String m, boolean c) { junit(); }
    public static void assertTrue(boolean c) { junit(); }

    public static void fail(String m) { junit(); }
    public static void fail() { junit(); }

    public static void failNotEquals(String m, Object e, Object a) { junit(); }
    public static void failNotSame(String m, Object e, Object a) { junit(); }
    public static void failSame(String m) { junit(); }

    public static String format(String m, Object e, Object a) { junit(); return null; }
}
