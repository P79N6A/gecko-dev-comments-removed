package org.mozilla.gecko.tests;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;

import org.mozilla.gecko.*;
import org.mozilla.gecko.GeckoThread.LaunchState;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.ContentUris;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.SystemClock;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.text.InputType;
import android.text.TextUtils;
import android.test.ActivityInstrumentationTestCase2;
import android.util.DisplayMetrics;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashMap;




abstract class BaseTest extends ActivityInstrumentationTestCase2<Activity> {
    public static final int TEST_MOCHITEST = 0;
    public static final int TEST_TALOS = 1;

    private static final String TARGET_PACKAGE_ID = "org.mozilla.gecko";
    private static final String LAUNCH_ACTIVITY_FULL_CLASSNAME = TestConstants.ANDROID_PACKAGE_NAME + ".App";
    private static final int VERIFY_URL_TIMEOUT = 2000;
    private static final int MAX_LIST_ATTEMPTS = 3;
    private static final int MAX_WAIT_ENABLED_TEXT_MS = 10000;
    private static final int MAX_WAIT_HOME_PAGER_HIDDEN_MS = 15000;
    public static final int MAX_WAIT_MS = 4500;
    public static final int LONG_PRESS_TIME = 6000;

    private static Class<Activity> mLauncherActivityClass;
    private Activity mActivity;
    protected Solo mSolo;
    protected Driver mDriver;
    protected Assert mAsserter;
    protected Actions mActions;
    protected String mBaseUrl;
    protected String mRawBaseUrl;
    private String mLogFile;
    protected String mProfile;
    public Device mDevice;
    protected DatabaseHelper mDatabaseHelper;
    protected StringHelper mStringHelper;

    protected void blockForGeckoReady() {
        try {
            Actions.EventExpecter geckoReadyExpector = mActions.expectGeckoEvent("Gecko:Ready");
            if (!GeckoThread.checkLaunchState(LaunchState.GeckoRunning)) {
                geckoReadyExpector.blockForEvent();
            }
            geckoReadyExpector.unregisterListener();
        } catch (Exception e) {
            mAsserter.dumpLog("Exception in blockForGeckoReady", e);
        }
    }

    static {
        try {
            mLauncherActivityClass = (Class<Activity>)Class.forName(LAUNCH_ACTIVITY_FULL_CLASSNAME);
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
    }

    public BaseTest() {
        super(TARGET_PACKAGE_ID, mLauncherActivityClass);
    }

    protected abstract int getTestType();

    @Override
    protected void setUp() throws Exception {
        
        String rootPath = FennecInstrumentationTestRunner.getFennecArguments().getString("deviceroot");
        String configFile = FennecNativeDriver.getFile(rootPath + "/robotium.config");
        HashMap config = FennecNativeDriver.convertTextToTable(configFile);
        mLogFile = (String)config.get("logfile");
        mBaseUrl = ((String)config.get("host")).replaceAll("(/$)", "");
        mRawBaseUrl = ((String)config.get("rawhost")).replaceAll("(/$)", "");
        
        if (getTestType() == TEST_TALOS) {
            mAsserter = new FennecTalosAssert();
        } else {
            mAsserter = new FennecMochitestAssert();
        }
        mAsserter.setLogFile(mLogFile);
        mAsserter.setTestName(this.getClass().getName());
        
        Intent i = new Intent(Intent.ACTION_MAIN);
        mProfile = (String)config.get("profile");
        i.putExtra("args", "-no-remote -profile " + mProfile);
        String envString = (String)config.get("envvars");
        if (envString != "") {
            String[] envStrings = envString.split(",");
            for (int iter = 0; iter < envStrings.length; iter++) {
                i.putExtra("env" + iter, envStrings[iter]);
            }
        }
        
        setActivityIntent(i);
        mActivity = getActivity();
        
        mSolo = new Solo(getInstrumentation(), mActivity);
        mDriver = new FennecNativeDriver(mActivity, mSolo, rootPath);
        mActions = new FennecNativeActions(mActivity, mSolo, getInstrumentation(), mAsserter);
        mDevice = new Device();
        mDatabaseHelper = new DatabaseHelper(mActivity, mAsserter);
        mStringHelper = new StringHelper();
    }

    @Override
    protected void runTest() throws Throwable {
        try {
            super.runTest();
        } catch (Throwable t) {
            
            
            mSolo.takeScreenshot("robocop-screenshot");
            if (mAsserter != null) {
                mAsserter.dumpLog("Exception caught during test!", t);
                mAsserter.ok(false, "Exception caught", t.toString());
            }
            
            throw t;
        }
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

    public void assertMatches(String value, String regex, String name) {
        if (value == null) {
            mAsserter.ok(false, name, "Expected /" + regex + "/, got null");
            return;
        }
        mAsserter.ok(value.matches(regex), name, "Expected /" + regex +"/, got \"" + value + "\"");
    }

    


    protected final void focusUrlBar() {
        
        final View toolbarView = mSolo.getView(R.id.browser_toolbar);
        mSolo.clickOnView(toolbarView);

        
        boolean success = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                EditText urlEditText = mSolo.getEditText(0);
                if (urlEditText.isInputMethodTarget()) {
                    return true;
                } else {
                    mSolo.clickOnEditText(0);
                    return false;
                }
            }
        }, MAX_WAIT_ENABLED_TEXT_MS);

        mAsserter.ok(success, "waiting for urlbar text to gain focus", "urlbar text gained focus");
    }

    protected final void enterUrl(String url) {
        final EditText urlEditView = (EditText) mSolo.getView(R.id.url_edit_text);

        focusUrlBar();

        
        mSolo.clearEditText(urlEditView);
        mSolo.enterText(urlEditView, url);

        
        final String urlBarText = urlEditView.getText().toString();
        mAsserter.is(url, urlBarText, "URL typed properly");
    }

    protected final Fragment getBrowserSearch() {
        final FragmentManager fm = ((FragmentActivity) getActivity()).getSupportFragmentManager();
        return fm.findFragmentByTag("browser_search");
    }

    protected final void hitEnterAndWait() {
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
        mActions.sendSpecialKey(Actions.SpecialKey.ENTER);
        
        contentEventExpecter.blockForEvent();
        contentEventExpecter.unregisterListener();
    }

    





    protected final void inputAndLoadUrl(String url) {
        enterUrl(url);
        hitEnterAndWait();
    }

    






    protected final void loadUrl(final String url) {
        try {
            Tabs.getInstance().loadUrl(url);
        } catch (Exception e) {
            mAsserter.dumpLog("Exception in loadUrl", e);
            throw new RuntimeException(e);
        }
    }

    public final void verifyUrl(String url) {
        final EditText urlEditText = (EditText) mSolo.getView(R.id.url_edit_text);
        String urlBarText = null;
        if (urlEditText != null) {
            
            
            waitForCondition(new VerifyTextViewText(urlEditText, url), VERIFY_URL_TIMEOUT);
            urlBarText = urlEditText.getText().toString();

        }
        mAsserter.is(urlBarText, url, "Browser toolbar URL stayed the same");
    }

    class VerifyTextViewText implements Condition {
        private TextView mTextView;
        private String mExpected;
        public VerifyTextViewText(TextView textView, String expected) {
            mTextView = textView;
            mExpected = expected;
        }

        @Override
        public boolean isSatisfied() {
            String textValue = mTextView.getText().toString();
            return mExpected.equals(textValue);
        }
    }

    protected final String getAbsoluteUrl(String url) {
        return mBaseUrl + "/" + url.replaceAll("(^/)", "");
    }

    protected final String getAbsoluteRawUrl(String url) {
        return mRawBaseUrl + "/" + url.replaceAll("(^/)", "");
    }

    


    protected final boolean waitForCondition(Condition condition, int timeout) {
        boolean result = mSolo.waitForCondition(condition, timeout);
        if (!result) {
            
            
            mAsserter.dumpLog("waitForCondition timeout after " + timeout + " ms.");
        }
        return result;
    }

    
    
    protected final boolean waitForTest(BooleanTest t, int timeout) {
        long end = SystemClock.uptimeMillis() + timeout;
        while (SystemClock.uptimeMillis() < end) {
            if (t.test()) {
                return true;
            }
            mSolo.sleep(100);
        }
        
        
        
        mAsserter.dumpLog("waitForTest timeout after "+timeout+" ms");
        return false;
    }

    
    
    protected interface BooleanTest {
        public boolean test();
    }

    @SuppressWarnings({"unchecked", "non-varargs"})
    public void SqliteCompare(String dbName, String sqlCommand, ContentValues[] cvs) {
        File profile = new File(mProfile);
        String dbPath = new File(profile, dbName).getPath();

        Cursor c = mActions.querySql(dbPath, sqlCommand);
        SqliteCompare(c, cvs);
    }

    private boolean CursorMatches(Cursor c, String[] columns, ContentValues cv) {
        for (int i = 0; i < columns.length; i++) {
            String column = columns[i];
            if (cv.containsKey(column)) {
                mAsserter.info("Comparing", "Column values for: " + column);
                Object value = cv.get(column);
                if (value == null) {
                    if (!c.isNull(i)) {
                        return false;
                    }
                } else {
                    if (c.isNull(i) || !value.toString().equals(c.getString(i))) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    @SuppressWarnings({"unchecked", "non-varargs"})
    public void SqliteCompare(Cursor c, ContentValues[] cvs) {
        mAsserter.is(c.getCount(), cvs.length, "List is correct length");
        if (c.moveToFirst()) {
            do {
                boolean found = false;
                for (int i = 0; !found && i < cvs.length; i++) {
                    if (CursorMatches(c, cvs[i])) {
                        found = true;
                    }
                }
                mAsserter.is(found, true, "Password was found");
            } while(c.moveToNext());
        }
    }

    public boolean CursorMatches(Cursor c, ContentValues cv) {
        for (int i = 0; i < c.getColumnCount(); i++) {
            String column = c.getColumnName(i);
            if (cv.containsKey(column)) {
                mAsserter.info("Comparing", "Column values for: " + column);
                Object value = cv.get(column);
                if (value == null) {
                    if (!c.isNull(i)) {
                        return false;
                    }
                } else {
                    if (c.isNull(i) || !value.toString().equals(c.getString(i))) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    public InputStream getAsset(String filename) throws IOException {
        AssetManager assets = getInstrumentation().getContext().getAssets();
        return assets.open(filename);
    }

    public boolean waitForText(String text) {
        boolean rc = mSolo.waitForText(text);
        if (!rc) {
            
            
            mAsserter.dumpLog("waitForText timeout on "+text);
        }
        return rc;
    }

    


    public boolean waitForEnabledText(String text) {
        final String testText = text;
        boolean rc = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                
                
                ArrayList<View> views = mSolo.getCurrentViews();
                for (View view : views) {
                    if (view instanceof TextView) {
                        TextView tv = (TextView)view;
                        String viewText = tv.getText().toString();
                        if (tv.isEnabled() && viewText != null && viewText.matches(testText)) {
                            return true;
                        }
                    }
                }
                return false;
            }
        }, MAX_WAIT_ENABLED_TEXT_MS);
        if (!rc) {
            
            
            mAsserter.dumpLog("waitForEnabledText timeout on "+text);
        }
        return rc;
    }


    


    public void selectSettingsItem(String section, String item) {
        String[] itemPath = { "Settings", section, item };
        selectMenuItemByPath(itemPath);
    }

    


    public void selectMenuItemByPath(String[] listItems) {
        int listLength = listItems.length;
        if (listLength > 0) {
            selectMenuItem(listItems[0]);
        }
        if (listLength > 1) {
            for (int i = 1; i < listLength; i++) {
                String itemName = "^" + listItems[i] + "$";
                if (!waitForEnabledText(itemName)) {
                    mSolo.scrollDown();
                }
                mSolo.clickOnText(itemName);
            }
        }
    }

    public final void selectMenuItem(String menuItemName) {
        
        String itemName = "^" + menuItemName + "$";
        mActions.sendSpecialKey(Actions.SpecialKey.MENU);
        if (waitForText(itemName)) {
            mSolo.clickOnText(itemName);
        } else {
            
            
            if (mSolo.searchText("(^More$|^Tools$)")) {
                mSolo.clickOnText("(^More$|^Tools$)");
            }
            waitForText(itemName);
            mSolo.clickOnText(itemName);
        }
    }

    public final void verifyHomePagerHidden() {
        final View homePagerContainer = mSolo.getView(R.id.home_pager_container);

        boolean rc = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return homePagerContainer.getVisibility() != View.VISIBLE;
            }
        }, MAX_WAIT_HOME_PAGER_HIDDEN_MS);

        if (!rc) {
            mAsserter.ok(rc, "Verify HomePager is hidden", "HomePager is hidden");
        }
    }

    public final void verifyPageTitle(String title) {
        final TextView urlBarTitle = (TextView) mSolo.getView(R.id.url_bar_title);
        String pageTitle = null;
        if (urlBarTitle != null) {
            
            
            waitForCondition(new VerifyTextViewText(urlBarTitle, title), MAX_WAIT_MS);
            pageTitle = urlBarTitle.getText().toString();
        }
        mAsserter.is(pageTitle, title, "Page title is correct");
    }

    public final void verifyTabCount(int expectedTabCount) {
        Element tabCount = mDriver.findElement(getActivity(), R.id.tabs_counter);
        String tabCountText = tabCount.getText();
        int tabCountInt = Integer.parseInt(tabCountText);
        mAsserter.is(tabCountInt, expectedTabCount, "The correct number of tabs are opened");
    }

    
    public void clickOnButton(String label) {
        final Button button = mSolo.getButton(label);
        try {
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    button.performClick();
                }
            });
       } catch (Throwable throwable) {
           mAsserter.ok(false, "Unable to click the button","Was unable to click button ");
       }
    }

    
    public void toggleVKB() {
        InputMethodManager imm = (InputMethodManager) getActivity().getSystemService(Activity.INPUT_METHOD_SERVICE);
        imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
    }

    public void addTab() {
        mSolo.clickOnView(mSolo.getView(R.id.tabs));
        
        boolean success = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                View addTabView = mSolo.getView(R.id.add_tab);
                if (addTabView == null) {
                    return false;
                }
                return true;
            }
        }, MAX_WAIT_MS);
        mAsserter.ok(success, "waiting for add tab view", "add tab view available");
        mSolo.clickOnView(mSolo.getView(R.id.add_tab));
    }

    public void addTab(String url) {
        addTab();

        
        inputAndLoadUrl(url);
    }

    




    private final AdapterView<ListAdapter> getTabsList() {
        Element tabs = mDriver.findElement(getActivity(), R.id.tabs);
        tabs.click();
        return (AdapterView<ListAdapter>) getActivity().findViewById(R.id.normal_tabs);
    }

    




    private View getTabViewAt(final int index) {
        final View[] childView = { null };

        final AdapterView<ListAdapter> view = getTabsList();

        runOnUiThreadSync(new Runnable() {
            @Override
            public void run() {
                view.setSelection(index);

                
                
                
                view.post(new Runnable() {
                    @Override
                    public void run() {
                        
                        
                        
                        
                        childView[0] = view.getChildAt(index - view.getFirstVisiblePosition());
                    }
                });
            }
        });

        boolean result = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return childView[0] != null;
            }
        }, MAX_WAIT_MS);

        mAsserter.ok(result, "list item at index " + index + " exists", null);

        return childView[0];
    }

    




    public void selectTabAt(final int index) {
        mSolo.clickOnView(getTabViewAt(index));
    }

    




    public void closeTabAt(final int index) {
        View closeButton = getTabViewAt(index).findViewById(R.id.close);

        mSolo.clickOnView(closeButton);
    }

    public final void runOnUiThreadSync(Runnable runnable) {
        RobocopUtils.runOnUiThreadSync(mActivity, runnable);
    }

    
    public void toggleBookmark() {
        mActions.sendSpecialKey(Actions.SpecialKey.MENU);
        waitForText("Settings");

        
        
        ArrayList<View> images = mSolo.getCurrentViews();
        for (int i = 0; i < images.size(); i++) {
            final View view = images.get(i);
            boolean found = false;
            found = "Bookmark".equals(view.getContentDescription());

            
            if (!found) {
                if (view instanceof TextView) {
                    found = "Bookmark".equals(((TextView)view).getText());
                }
            }

            if (found) {
                int[] xy = new int[2];
                view.getLocationOnScreen(xy);

                final int viewWidth = view.getWidth();
                final int viewHeight = view.getHeight();
                final float x = xy[0] + (viewWidth / 2.0f);
                float y = xy[1] + (viewHeight / 2.0f);

                mSolo.clickOnScreen(x, y);
            }
        }
    }

    public void clearPrivateData() {
        selectSettingsItem(StringHelper.PRIVACY_SECTION_LABEL, StringHelper.CLEAR_PRIVATE_DATA_LABEL);
        Actions.EventExpecter clearData = mActions.expectGeckoEvent("Sanitize:Finished");
        mSolo.clickOnText("Clear data");
        clearData.blockForEvent();
        clearData.unregisterListener();
    }

    class Device {
        public final String version; 
        public String type; 
        public final int width;
        public final int height;
        public final float density;

        public Device() {
            
            int sdk = Build.VERSION.SDK_INT;
            if (sdk < Build.VERSION_CODES.HONEYCOMB) {
                version = "2.x";
            } else {
                if (sdk > Build.VERSION_CODES.HONEYCOMB_MR2) {
                    version = "4.x";
                } else {
                    version = "3.x";
                }
            }
            
            DisplayMetrics dm = new DisplayMetrics();
            getActivity().getWindowManager().getDefaultDisplay().getMetrics(dm);
            height = dm.heightPixels;
            width = dm.widthPixels;
            density = dm.density;
            
            type = "phone";
            try {
                if (GeckoAppShell.isTablet()) {
                    type = "tablet";
                }
            } catch (Exception e) {
                mAsserter.dumpLog("Exception in detectDevice", e);
            }
        }

        public void rotate() {
            if (getActivity().getRequestedOrientation () == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
                mSolo.setActivityOrientation(Solo.PORTRAIT);
            } else {
                mSolo.setActivityOrientation(Solo.LANDSCAPE);
            }
        }
    }

    class Navigation {
        private String devType;
        private String osVersion;

        public Navigation(Device mDevice) {
            devType = mDevice.type;
            osVersion = mDevice.version;
        }

        public void back() {
            Actions.EventExpecter pageShowExpecter = mActions.expectGeckoEvent("Content:PageShow");

            if (devType.equals("tablet")) {
                Element backBtn = mDriver.findElement(getActivity(), R.id.back);
                backBtn.click();
            } else {
                mActions.sendSpecialKey(Actions.SpecialKey.BACK);
            }

            pageShowExpecter.blockForEvent();
            pageShowExpecter.unregisterListener();
        }

        public void forward() {
            Actions.EventExpecter pageShowExpecter = mActions.expectGeckoEvent("Content:PageShow");

            if (devType.equals("tablet")) {
                Element fwdBtn = mDriver.findElement(getActivity(), R.id.forward);
                fwdBtn.click();
            } else {
                mActions.sendSpecialKey(Actions.SpecialKey.MENU);
                waitForText("^New Tab$");
                if (!osVersion.equals("2.x")) {
                    Element fwdBtn = mDriver.findElement(getActivity(), R.id.forward);
                    fwdBtn.click();
                } else {
                    mSolo.clickOnText("^Forward$");
                }
                ensureMenuClosed();
            }

            pageShowExpecter.blockForEvent();
            pageShowExpecter.unregisterListener();
        }

        public void reload() {
            if (devType.equals("tablet")) {
                Element reloadBtn = mDriver.findElement(getActivity(), R.id.reload);
                reloadBtn.click();
            } else {
                mActions.sendSpecialKey(Actions.SpecialKey.MENU);
                waitForText("^New Tab$");
                if (!osVersion.equals("2.x")) {
                    Element reloadBtn = mDriver.findElement(getActivity(), R.id.reload);
                    reloadBtn.click();
                } else {
                    mSolo.clickOnText("^Reload$");
                }
                ensureMenuClosed();
            }
        }

        
        
        public void bookmark() {
            mActions.sendSpecialKey(Actions.SpecialKey.MENU);
            waitForText("^New Tab$");
            if (mSolo.searchText("^Bookmark$")) {
                
                mSolo.clickOnText("^Bookmark$");
            } else {
                Element bookmarkBtn = mDriver.findElement(getActivity(), R.id.bookmark);
                if (bookmarkBtn != null) {
                    
                    bookmarkBtn.click();
                }
            }
            ensureMenuClosed();
        }

        
        
        private void ensureMenuClosed() {
            if (mSolo.searchText("^New Tab$")) {
                mActions.sendSpecialKey(Actions.SpecialKey.BACK);
            }
         }
    }

    





    public static String getStackTraceString(Throwable t) {
        StringWriter sw = new StringWriter();
        t.printStackTrace(new PrintWriter(sw));
        return sw.toString();
    }

    


    private class DescriptionCondition<T extends View> implements Condition {
        public T mView;
        private String mDescr;
        private Class<T> mCls;

        public DescriptionCondition(Class<T> cls, String descr) {
            mDescr = descr;
            mCls = cls;
        }

        @Override
        public boolean isSatisfied() {
            mView = findViewWithContentDescription(mCls, mDescr);
            return (mView != null);
        }
    }

    


    public <T extends View> T waitForViewWithDescription(Class<T> cls, String description) {
        DescriptionCondition<T> c = new DescriptionCondition<T>(cls, description);
        waitForCondition(c, MAX_WAIT_ENABLED_TEXT_MS);
        return c.mView;
    }

    


    public <T extends View> T findViewWithContentDescription(Class<T> cls, String description) {
        for (T view : mSolo.getCurrentViews(cls)) {
            final String descr = (String) view.getContentDescription();
            if (TextUtils.isEmpty(descr)) {
                continue;
            }

            if (TextUtils.equals(description, descr)) {
                return view;
            }
        }

        return null;
    }
}
