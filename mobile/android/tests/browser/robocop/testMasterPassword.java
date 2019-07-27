



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;



public class testMasterPassword extends PixelTest {
    Device dev;

    public void testMasterPassword() {
        blockForGeckoReady();

        dev = new Device();
        String password = ("Good");
        String badPassword = ("Bad");

        enableMasterPassword(password, badPassword);
        verifyLoginPage(password, badPassword);
        disableMasterPassword(password, badPassword);
    }

    public void enableMasterPassword(String password, String badPassword) {

        
        selectSettingsItem(mStringHelper.PRIVACY_SECTION_LABEL, mStringHelper.MASTER_PASSWORD_LABEL);
        waitForText("^Create Master Password$");

        
        closeTabletKeyboard();
        mAsserter.ok(!mSolo.getButton("OK").isEnabled(), "Verify if the OK button is inactive", "The OK button is inactive until both fields are filled");

        
        editPasswordField(0, password);
        mAsserter.ok(!mSolo.getButton("OK").isEnabled(), "Verify if the OK button is inactive", "The OK button is inactive until the Confirm password field is filled");

        
        editPasswordField(1, badPassword);
        mAsserter.ok(!mSolo.getButton("OK").isEnabled(), "Verify if the OK button is inactive", "The OK button is inactive until both fields contain the same password");

        
        mSolo.clearEditText(0);
        mAsserter.ok(!mSolo.getButton("OK").isEnabled(), "Verify if the OK button is inactive", "The OK button is inactive until the Password field is filled");

        
        mSolo.clickOnEditText(0);
        mActions.sendKeys(password);
        mSolo.clearEditText(1);
        mSolo.clickOnEditText(1);
        mActions.sendKeys(password);
        waitForText("^Cancel$");
        mSolo.clickOnText("^Cancel$");
        waitForText("^" + mStringHelper.MASTER_PASSWORD_LABEL + "$");
        mSolo.clickOnText("^" + mStringHelper.MASTER_PASSWORD_LABEL + "$");
        mAsserter.ok(mSolo.waitForText("^Create Master Password$"), "Checking if no password was set if the action was canceled", "No password was set");

        
        mSolo.clickOnEditText(0);
        mActions.sendKeys(password);
        mSolo.clickOnEditText(1);
        mActions.sendKeys(password);

        
        mAsserter.ok(waitForText("."), "waiting to convert the letters in dots", "The letters are converted in dots");
        mSolo.clickOnButton("OK");

        
        mSolo.searchText("Privacy");
        mAsserter.ok(mSolo.waitForText("^Use master password$"), "Checking if Use master password is present", "Use master password is present");
        mSolo.clickOnText("^Use master password$");
        mAsserter.ok(mSolo.waitForText("Remove Master Password"), "Checking if the password is enabled", "The password is enabled");
        clickOnButton("Cancel"); 

        if ("phone".equals(mDevice.type)) {
            
            waitForText("Use master password");
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        }
        waitForText(mStringHelper.SETTINGS_LABEL);
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
    }

    public void disableMasterPassword(String password, String badPassword) {

        
        selectSettingsItem(mStringHelper.PRIVACY_SECTION_LABEL, mStringHelper.MASTER_PASSWORD_LABEL);
        waitForText("^Remove Master Password$");

        
        closeTabletKeyboard();
        mAsserter.ok(!mSolo.getButton("OK").isEnabled(), "Verify if the OK button is inactive", "The OK button is inactive if the password field is empty");

        
        editPasswordField(0, badPassword);
        mAsserter.ok(mSolo.getButton("OK").isEnabled(), "Verify if the OK button is activated", "The OK button is activated even if the wrong password is filled");
        mSolo.clickOnButton("OK");
        mAsserter.ok(mSolo.waitForText("^Incorrect password$"), "Waiting for Incorrect password notification", "The Incorrect password notification appears");

        
        mSolo.clickOnText("^Use master password$");
        waitForText("^Remove Master Password$");
        closeTabletKeyboard();
        editPasswordField(0, password);
        mSolo.clickOnButton("OK");

        
        mSolo.searchText("Privacy");
        mAsserter.ok(mSolo.waitForText("^Use master password$"), "Checking if Use master password is present", "Use master password is present");
        mSolo.clickOnText("^Use master password$");
        mAsserter.ok(waitForText("^Create Master Password$"), "Checking if the password is disabled", "The password is disabled");
        clickOnButton("Cancel"); 
    }

    public void editPasswordField(int i, String password) {
        mSolo.clickOnEditText(i);
        mActions.sendKeys(password);
        toggleVKB(); 
    }

    public void noDoorhangerDisplayed(String LOGIN_URL) {
        waitForText("Browser Blank Page 01|Enter Search or Address");
        inputAndLoadUrl(LOGIN_URL);
        mAsserter.is(waitForText("Save password for"), false, "Doorhanger notification is hidden");
    }

    public void doorhangerDisplayed(String LOGIN_URL) {
        waitForText("Browser Blank Page 01|Enter Search or Address");
        inputAndLoadUrl(LOGIN_URL);
        mAsserter.is(mSolo.waitForText("Save password for"), true, "Doorhanger notification is displayed");
    }

    
    public void closeTabletKeyboard() {
        if (dev.type.equals("tablet")) {
            mSolo.sleep(1500);
            toggleVKB();
        }
    }

    @Override
    public void clearPrivateData() {

        
        selectSettingsItem(mStringHelper.PRIVACY_SECTION_LABEL, mStringHelper.CLEAR_PRIVATE_DATA_LABEL);

        waitForText("Browsing history"); 
        Actions.EventExpecter clearPrivateDataEventExpecter = mActions.expectGeckoEvent("Sanitize:Finished");
        if (mSolo.searchText("Clear data") && !mSolo.searchText("Cookies")) {
            mSolo.clickOnText("^Clear data$");
            clearPrivateDataEventExpecter.blockForEvent();
        } else { 
            if (mSolo.searchText("Cookies")) {
                mSolo.clickOnText("^Clear private data$");
                waitForText("Browsing history"); 
                mSolo.clickOnText("^Clear data$");
                clearPrivateDataEventExpecter.blockForEvent();
            } else {
                mAsserter.ok(false, "Something happened and the clear data dialog could not be opened", "Failed to clear data");
            }
        }

        
        waitForText("^Use master password$");
        mSolo.clickOnText("^Use master password$");
        mAsserter.ok(mSolo.searchText("^Remove Master Password$"), "Checking if the master password was disabled by clearing private data", "The master password is not disabled by clearing private data");
        clickOnButton("Cancel"); 

        if ("phone".equals(mDevice.type)) {
            
            waitForText("Use master password");
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        }
        waitForText(mStringHelper.SETTINGS_LABEL);
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        
        mAsserter.ok(mSolo.waitForText("Browser Blank Page 01"), "Waiting for blank browser page after exiting settings", "Blank browser page present");
    }

    public void verifyLoginPage(String password, String badPassword) {
        String LOGIN_URL = getAbsoluteUrl(mStringHelper.ROBOCOP_LOGIN_01_URL);
        String option [] = {"Save", "Don't save"};

        doorhangerDisplayed(LOGIN_URL);

        
        mSolo.sleep(2000);

        for (String item:option) {
            if (item.equals("Save")) {
                final String OK_BUTTON_LABEL = "^OK$";
                final String SAVE_BUTTON_LABEL = "^Save$";
                mAsserter.ok(mSolo.waitForText(SAVE_BUTTON_LABEL), "Checking if Save option is present", "Save option is present");
                mSolo.clickOnButton(SAVE_BUTTON_LABEL);

                
                closeTabletKeyboard();
                waitForText(OK_BUTTON_LABEL);
                mSolo.clickOnButton(OK_BUTTON_LABEL);

                
                closeTabletKeyboard();
                editPasswordField(0, badPassword);
                waitForText(OK_BUTTON_LABEL);
                mSolo.clickOnButton(OK_BUTTON_LABEL);

                
                closeTabletKeyboard();
                editPasswordField(0, password);
                waitForText(OK_BUTTON_LABEL);
                mSolo.clickOnButton(OK_BUTTON_LABEL);

                
                noDoorhangerDisplayed(LOGIN_URL);
            } else {
                clearPrivateData();
                doorhangerDisplayed(LOGIN_URL);
                mAsserter.ok(mSolo.waitForText("Don't save"), "Checking if Don't save option is present again", "Don't save option is present again");
                mSolo.clickOnText("Don't save");
                doorhangerDisplayed(LOGIN_URL);
                mAsserter.ok(mSolo.waitForText("Don't save"), "Checking if Don't save option is present again", "Don't save option is present again");
                mSolo.clickOnText("Don't save");
                
                mAsserter.ok(mSolo.waitForText("Browser Blank Page 01"), "Waiting for blank browser page after exiting settings", "Blank browser page present");
            }
        }
    }
}
