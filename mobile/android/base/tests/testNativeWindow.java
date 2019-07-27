package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;

import android.widget.CheckBox;

import java.util.ArrayList;

public class testNativeWindow extends BaseTest {

    private static final String TEST_URL = "chrome://roboextender/content/testNativeWindow.html";
    private static final String TOAST_TEXT = "TOAST!";
    private static final String MENU_TEXT1 = "MENU-A";
    private static final String MENU_TEXT2 = "MENU-B";
    private static final String DOORHANGER_TEXT = "DOORHANGER";
    private static final String DOORHANGER_BUTTON1 = "BUTTON1";
    private static final String DOORHANGER_BUTTON2 = "BUTTON2";
    private static final String DOORHANGER_CHECK = "CHECKBOX";

    public void testNativeWindow() {
        blockForGeckoReady();

        
        addToastTest();

        prepNextTest();
        addMenuTest();

        prepNextTest();
        updateMenuTest();

        prepNextTest();
        removeMenuTest();

        prepNextTest();
        addDoorhangerTest();
    }

    


    private void prepNextTest() {
        loadUrl("about:blank");
        mAsserter.ok(waitForText("about:blank"), "Loaded blank page", "page title match");
    }

    


    private void addToastTest() {
        Actions.EventExpecter eventExpecter = mActions.expectGeckoEvent("TestNativeWindow:ShowToast");
        loadUrl(TEST_URL + "#showToast");
        eventExpecter.blockForEvent();

        
        mAsserter.ok(waitForText(TOAST_TEXT), "Waiting for the toast", "Toast has been displayed");
    }

    


    private void addMenuTest() {
        Actions.EventExpecter eventExpecter = mActions.expectGeckoEvent("TestNativeWindow:AddMenu");
        loadUrl(TEST_URL + "#addMenu");
        eventExpecter.blockForEvent();

        
        Actions.EventExpecter menuExpecter = mActions.expectGeckoEvent("TestNativeWindow:FireMenu");
        selectMenuItem(MENU_TEXT1);
        menuExpecter.blockForEvent();
    }

    


    private void updateMenuTest() {
        
        Actions.EventExpecter eventExpecter = mActions.expectGeckoEvent("TestNativeWindow:UpdateMenu");
        loadUrl(TEST_URL + "#updateMenu");
        eventExpecter.blockForEvent();

        
        Actions.EventExpecter menuExpecter = mActions.expectGeckoEvent("TestNativeWindow:FireMenu");
        selectMenuItem(MENU_TEXT2);
        menuExpecter.blockForEvent();
    }

    


    private void removeMenuTest() {
        Actions.EventExpecter eventExpecter = mActions.expectGeckoEvent("TestNativeWindow:RemoveMenu");
        loadUrl(TEST_URL + "#removeMenu");
        eventExpecter.blockForEvent();

        
        mActions.sendSpecialKey(Actions.SpecialKey.MENU);
        waitForText("^New Tab$");
        mAsserter.ok(mSolo.searchText("^" + MENU_TEXT2 + "$") == false, "Checking for menu", "Menu has been removed");

        
        if (mSolo.searchText("^New Tab$")) {
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        }
    }

    


    private void addDoorhangerTest() {
        Actions.EventExpecter eventExpecter = mActions.expectGeckoEvent("TestNativeWindow:AddDoorhanger");
        loadUrl(TEST_URL + "#addDoorhanger");
        eventExpecter.blockForEvent();

        
        Actions.EventExpecter menuExpecter = mActions.expectGeckoEvent("TestNativeWindow:FireDoorhanger");
        waitForText(DOORHANGER_TEXT);

        
        ArrayList<CheckBox> checkBoxes = mSolo.getCurrentViews(CheckBox.class);
        mAsserter.ok(checkBoxes.size() == 1, "checkbox count", "only one checkbox visible");
        mAsserter.ok(mSolo.isCheckBoxChecked(0), "checkbox checked", "checkbox is checked");
        mSolo.clickOnCheckBox(0);
        mAsserter.ok(!mSolo.isCheckBoxChecked(0), "checkbox not checked", "checkbox is not checked");

        mSolo.clickOnText(DOORHANGER_BUTTON1);

        menuExpecter.blockForEvent();
    }
}
