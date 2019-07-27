



package org.mozilla.gecko.tests;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashSet;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Element;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.GeckoThread.LaunchState;
import org.mozilla.gecko.R;
import org.mozilla.gecko.RobocopUtils;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;

import android.app.Activity;
import android.content.ContentValues;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.Build;
import android.os.SystemClock;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;
import com.jayway.android.robotium.solo.Timeout;




@SuppressWarnings("unchecked")
abstract class BaseTest extends BaseRobocopTest {
    private static final int VERIFY_URL_TIMEOUT = 2000;
    private static final int MAX_WAIT_ENABLED_TEXT_MS = 10000;
    private static final int MAX_WAIT_HOME_PAGER_HIDDEN_MS = 15000;
    private static final int MAX_WAIT_VERIFY_PAGE_TITLE_MS = 15000;
    public static final int MAX_WAIT_MS = 4500;
    public static final int LONG_PRESS_TIME = 6000;
    private static final int GECKO_READY_WAIT_MS = 180000;
    public static final int MAX_WAIT_BLOCK_FOR_EVENT_DATA_MS = 90000;

    protected static final String URL_HTTP_PREFIX = "http://";

    private int mPreferenceRequestID = 0;
    public Device mDevice;
    protected DatabaseHelper mDatabaseHelper;
    protected int mScreenMidWidth;
    protected int mScreenMidHeight;
    private final HashSet<Integer> mKnownTabIDs = new HashSet<Integer>();

    protected void blockForDelayedStartup() {
        try {
            Actions.EventExpecter delayedStartupExpector = mActions.expectGeckoEvent("Gecko:DelayedStartup");
            delayedStartupExpector.blockForEvent(GECKO_READY_WAIT_MS, true);
            delayedStartupExpector.unregisterListener();
        } catch (Exception e) {
            mAsserter.dumpLog("Exception in blockForDelayedStartup", e);
        }
    }

    protected void blockForGeckoReady() {
        try {
            Actions.EventExpecter geckoReadyExpector = mActions.expectGeckoEvent("Gecko:Ready");
            if (!GeckoThread.checkLaunchState(LaunchState.GeckoRunning)) {
                geckoReadyExpector.blockForEvent(GECKO_READY_WAIT_MS, true);
            }
            geckoReadyExpector.unregisterListener();
        } catch (Exception e) {
            mAsserter.dumpLog("Exception in blockForGeckoReady", e);
        }
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mDevice = new Device();
        mDatabaseHelper = new DatabaseHelper(getActivity(), mAsserter);

        
        throwIfHttpGetFails();
        throwIfScreenNotOn();
    }

    protected GeckoProfile getTestProfile() {
        if (mProfile.startsWith("/")) {
            return GeckoProfile.get(getActivity(), "default", mProfile);
        }

        return GeckoProfile.get(getActivity(), mProfile);
    }

    protected void initializeProfile() {
        final GeckoProfile profile = getTestProfile();

        
        
        profile.enqueueInitialization(profile.getDir());
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
            
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Robocop:Quit", null));
            mSolo.sleep(120000);
            
            mSolo.finishOpenedActivities();
        } catch (Throwable e) {
            e.printStackTrace();
        }
        super.tearDown();
    }

    @Override
    protected Intent createActivityIntent() {
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.putExtra("args", "-no-remote -profile " + mProfile);

        final String envString = mConfig.get("envvars");
        if (!TextUtils.isEmpty(envString)) {
            final String[] envStrings = envString.split(",");

            for (int iter = 0; iter < envStrings.length; iter++) {
                intent.putExtra("env" + iter, envStrings[iter]);
            }
        }

        return intent;
    }

    public void assertMatches(String value, String regex, String name) {
        if (value == null) {
            mAsserter.ok(false, name, "Expected /" + regex + "/, got null");
            return;
        }
        mAsserter.ok(value.matches(regex), name, "Expected /" + regex +"/, got \"" + value + "\"");
    }

    


    protected final void focusUrlBar() {
        
        mSolo.waitForView(R.id.browser_toolbar);
        final View toolbarView = mSolo.getView(R.id.browser_toolbar);
        mSolo.clickOnView(toolbarView);

        
        boolean success = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                mSolo.waitForView(R.id.url_edit_text);
                EditText urlEditText = (EditText) mSolo.getView(R.id.url_edit_text);
                if (urlEditText.isInputMethodTarget()) {
                    return true;
                }
                return false;
            }
        }, MAX_WAIT_ENABLED_TEXT_MS);

        mAsserter.ok(success, "waiting for urlbar text to gain focus", "urlbar text gained focus");
    }

    protected final void enterUrl(String url) {
        focusUrlBar();

        final EditText urlEditView = (EditText) mSolo.getView(R.id.url_edit_text);

        
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

    



    protected final void loadUrlAndWait(final String url) {
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
        loadUrl(url);
        contentEventExpecter.blockForEvent();
        contentEventExpecter.unregisterListener();
    }

    protected final void closeTab(int tabId) {
        Tabs tabs = Tabs.getInstance();
        Tab tab = tabs.getTab(tabId);
        tabs.closeTab(tab);
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
        private final TextView mTextView;
        private final String mExpected;
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
        return mBaseHostnameUrl + "/" + url.replaceAll("(^/)", "");
    }

    protected final String getAbsoluteRawUrl(String url) {
        return mBaseIpUrl + "/" + url.replaceAll("(^/)", "");
    }

    


    protected final boolean waitForCondition(Condition condition, int timeout) {
        boolean result = mSolo.waitForCondition(condition, timeout);
        if (!result) {
            
            
            mAsserter.dumpLog("waitForCondition timeout after " + timeout + " ms.");
        }
        return result;
    }

    protected interface BooleanTest {
        public boolean test();
    }

    public void SqliteCompare(String dbName, String sqlCommand, ContentValues[] cvs) {
        File profile = new File(mProfile);
        String dbPath = new File(profile, dbName).getPath();

        Cursor c = mActions.querySql(dbPath, sqlCommand);
        SqliteCompare(c, cvs);
    }

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
            } while (c.moveToNext());
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

    public boolean waitForText(final String text) {
        
        
        return waitForText(text, false);
    }

    public boolean waitForText(final String text, final boolean onlyVisibleViews) {
        
        
        final boolean rc =
                mSolo.waitForText(text, 0, Timeout.getLargeTimeout(), true, onlyVisibleViews);
        if (!rc) {
            
            
            mAsserter.dumpLog("waitForText timeout on "+text);
        }
        return rc;
    }

    
    
    
    protected boolean waitForPreferencesText(String txt) {
        boolean foundText = waitForText(txt);
        if (!foundText) {
            if ((mScreenMidWidth == 0) || (mScreenMidHeight == 0)) {
                mScreenMidWidth = mDriver.getGeckoWidth()/2;
                mScreenMidHeight = mDriver.getGeckoHeight()/2;
            }

            
            
            MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());
            meh.dragSync(mScreenMidWidth, mScreenMidHeight+100, mScreenMidWidth, mScreenMidHeight-100);

            foundText = mSolo.waitForText(txt);
        }
        return foundText;
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
                mAsserter.ok(waitForPreferencesText(itemName), "Waiting for and scrolling once to find item " + itemName, itemName + " found");
                mAsserter.ok(waitForEnabledText(itemName), "Waiting for enabled text " + itemName, itemName + " option is present and enabled");
                mSolo.clickOnText(itemName);
            }
        }
    }

    public final void selectMenuItem(String menuItemName) {
        
        String itemName = "^" + menuItemName + "$";
        mActions.sendSpecialKey(Actions.SpecialKey.MENU);
        if (waitForText(itemName, true)) {
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

    public final void verifyUrlBarTitle(String url) {
        mAsserter.isnot(url, null, "The url argument is not null");

        final String expected;
        if (mStringHelper.ABOUT_HOME_URL.equals(url)) {
            expected = mStringHelper.ABOUT_HOME_TITLE;
        } else if (url.startsWith(URL_HTTP_PREFIX)) {
            expected = url.substring(URL_HTTP_PREFIX.length());
        } else {
            expected = url;
        }

        final TextView urlBarTitle = (TextView) mSolo.getView(R.id.url_bar_title);
        String pageTitle = null;
        if (urlBarTitle != null) {
            
            
            waitForCondition(new VerifyTextViewText(urlBarTitle, expected), MAX_WAIT_VERIFY_PAGE_TITLE_MS);
            pageTitle = urlBarTitle.getText().toString();
        }
        mAsserter.is(pageTitle, expected, "Page title is correct");
    }

    public final void verifyTabCount(int expectedTabCount) {
        Element tabCount = mDriver.findElement(getActivity(), R.id.tabs_counter);
        String tabCountText = tabCount.getText();
        int tabCountInt = Integer.parseInt(tabCountText);
        mAsserter.is(tabCountInt, expectedTabCount, "The correct number of tabs are opened");
    }

    public void verifyPinned(final boolean isPinned, final String gridItemTitle) {
        boolean viewFound = waitForText(gridItemTitle);
        mAsserter.ok(viewFound, "Found top site title: " + gridItemTitle, null);

        boolean success = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                
                final TextView gridItemTextView = mSolo.getText(gridItemTitle);
                return isPinned == (gridItemTextView.getCompoundDrawables()[0] != null);
            }
        }, MAX_WAIT_MS);
        mAsserter.ok(success, "Top site item was pinned: " + isPinned, null);
    }

    public void pinTopSite(String gridItemTitle) {
        verifyPinned(false, gridItemTitle);
        mSolo.clickLongOnText(gridItemTitle);
        boolean dialogOpened = mSolo.waitForDialogToOpen();
        mAsserter.ok(dialogOpened, "Pin site dialog opened: " + gridItemTitle, null);
        boolean pinSiteFound = waitForText(mStringHelper.CONTEXT_MENU_PIN_SITE);
        mAsserter.ok(pinSiteFound, "Found pin site menu item", null);
        mSolo.clickOnText(mStringHelper.CONTEXT_MENU_PIN_SITE);
        verifyPinned(true, gridItemTitle);
    }

    public void unpinTopSite(String gridItemTitle) {
        verifyPinned(true, gridItemTitle);
        mSolo.clickLongOnText(gridItemTitle);
        boolean dialogOpened = mSolo.waitForDialogToOpen();
        mAsserter.ok(dialogOpened, "Pin site dialog opened: " + gridItemTitle, null);
        boolean unpinSiteFound = waitForText(mStringHelper.CONTEXT_MENU_UNPIN_SITE);
        mAsserter.ok(unpinSiteFound, "Found unpin site menu item", null);
        mSolo.clickOnText(mStringHelper.CONTEXT_MENU_UNPIN_SITE);
        verifyPinned(false, gridItemTitle);
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
        final Actions.RepeatedEventExpecter pageShowExpecter = mActions.expectGeckoEvent("Content:PageShow");
        mSolo.clickOnView(mSolo.getView(R.id.add_tab));
        
        for(;;) {
            try {
                JSONObject data = new JSONObject(pageShowExpecter.blockForEventData());
                int tabID = data.getInt("tabID");
                if (tabID == 0) {
                    mAsserter.dumpLog("addTab ignoring PageShow for tab 0");
                    continue;
                }
                if (!mKnownTabIDs.contains(tabID)) {
                    mKnownTabIDs.add(tabID);
                    break;
                }
            } catch(JSONException e) {
                mAsserter.ok(false, "Exception in addTab", getStackTraceString(e));
            }
        }
        pageShowExpecter.unregisterListener();
    }

    public void addTab(String url) {
        addTab();

        
        inputAndLoadUrl(url);
    }

    public void closeAddedTabs() {
        for(int tabID : mKnownTabIDs) {
            closeTab(tabID);
        }
    }

    




    private final AdapterView<ListAdapter> getTabsLayout() {
        Element tabs = mDriver.findElement(getActivity(), R.id.tabs);
        tabs.click();
        return (AdapterView<ListAdapter>) getActivity().findViewById(R.id.normal_tabs);
    }

    




    private View getTabViewAt(final int index) {
        final View[] childView = { null };

        final AdapterView<ListAdapter> view = getTabsLayout();

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
        RobocopUtils.runOnUiThreadSync(getActivity(), runnable);
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
        selectSettingsItem(mStringHelper.PRIVACY_SECTION_LABEL, mStringHelper.CLEAR_PRIVATE_DATA_LABEL);
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
        private final String devType;
        private final String osVersion;

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
                mSolo.waitForView(R.id.forward);
                mSolo.clickOnView(mSolo.getView(R.id.forward));
            } else {
                mActions.sendSpecialKey(Actions.SpecialKey.MENU);
                waitForText("^New Tab$");
                if (!osVersion.equals("2.x")) {
                    mSolo.waitForView(R.id.forward);
                    mSolo.clickOnView(mSolo.getView(R.id.forward));
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
                mSolo.waitForView(R.id.reload);
                mSolo.clickOnView(mSolo.getView(R.id.reload));
            } else {
                mActions.sendSpecialKey(Actions.SpecialKey.MENU);
                waitForText("^New Tab$");
                if (!osVersion.equals("2.x")) {
                    mSolo.waitForView(R.id.reload);
                    mSolo.clickOnView(mSolo.getView(R.id.reload));
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
        private final String mDescr;
        private final Class<T> mCls;

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

    


    abstract class TestCase implements Runnable {
        




        protected abstract void test() throws Exception;

        @Override
        public void run() {
            try {
                test();
            } catch (Exception e) {
                mAsserter.ok(false,
                             "Test " + this.getClass().getName() + " threw exception: " + e,
                             "");
            }
        }
    }

    


    public void setPreferenceAndWaitForChange(final JSONObject jsonPref) {
        mActions.sendGeckoEvent("Preferences:Set", jsonPref.toString());

        
        
        String[] prefNames = new String[1];
        try {
            prefNames[0] = jsonPref.getString("name");
        } catch (JSONException e) {
            mAsserter.ok(false, "Exception in setPreferenceAndWaitForChange", getStackTraceString(e));
        }

        
        final int ourRequestID = mPreferenceRequestID--;
        final Actions.RepeatedEventExpecter eventExpecter = mActions.expectGeckoEvent("Preferences:Data");
        mActions.sendPreferencesGetEvent(ourRequestID, prefNames);

        
        waitForCondition(new Condition() {
            final long endTime = SystemClock.elapsedRealtime() + MAX_WAIT_BLOCK_FOR_EVENT_DATA_MS;

            @Override
            public boolean isSatisfied() {
                try {
                    long timeout = endTime - SystemClock.elapsedRealtime();
                    if (timeout < 0) {
                        timeout = 0;
                    }

                    JSONObject data = new JSONObject(eventExpecter.blockForEventDataWithTimeout(timeout));
                    int requestID = data.getInt("requestId");
                    if (requestID != ourRequestID) {
                        return false;
                    }

                    JSONArray preferences = data.getJSONArray("preferences");
                    mAsserter.is(preferences.length(), 1, "Expecting preference array to have one element");
                    JSONObject prefs = (JSONObject) preferences.get(0);
                    mAsserter.is(prefs.getString("name"), jsonPref.getString("name"),
                            "Expecting returned preference name to be the same as the set name");
                    mAsserter.is(prefs.getString("type"), jsonPref.getString("type"),
                            "Expecting returned preference type to be the same as the set type");
                    mAsserter.is(prefs.get("value"), jsonPref.get("value"),
                            "Expecting returned preference value to be the same as the set value");
                    return true;
                } catch(JSONException e) {
                    mAsserter.ok(false, "Exception in setPreferenceAndWaitForChange", getStackTraceString(e));
                    
                    return false;
                }
            }
        }, MAX_WAIT_BLOCK_FOR_EVENT_DATA_MS);

        eventExpecter.unregisterListener();
    }
}
