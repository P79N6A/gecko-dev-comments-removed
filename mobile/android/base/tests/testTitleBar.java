package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.NewTabletUI;





public class testTitleBar extends PixelTest {
    public void testTitleBar() {
        
        if (NewTabletUI.isEnabled(getActivity())) {
            return;
        }

        blockForGeckoReady();
        checkOption();
    }

    public void checkOption() {

        String blank1 = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String title = StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE;

        
        inputAndLoadUrl(blank1);
        verifyPageTitle(title, blank1);

        
        selectOption(StringHelper.SHOW_PAGE_ADDRESS_LABEL);
        inputAndLoadUrl(blank1);
        verifyUrl(blank1);

        
        selectOption(StringHelper.SHOW_PAGE_TITLE_LABEL);
        inputAndLoadUrl(blank1);
        verifyPageTitle(title, blank1);
    }

    
    public void selectOption(String option) {
        selectSettingsItem(StringHelper.DISPLAY_SECTION_LABEL, StringHelper.TITLE_BAR_LABEL);
        mAsserter.ok(waitForText(StringHelper.SHOW_PAGE_TITLE_LABEL), "Waiting for the pop-up to open", "Pop up with the options was openend");
        mSolo.clickOnText(option);
        mAsserter.ok(waitForText(StringHelper.CHARACTER_ENCODING_LABEL), "Waiting to press the option", "The pop-up is dismissed once clicked");
        if (mDevice.type.equals("phone")) {
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
            mAsserter.ok(waitForText(StringHelper.CUSTOMIZE_SECTION_LABEL), "Waiting to perform one back", "One back performed");
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
            mAsserter.ok(waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_URL), "Waiting to exit settings", "Exit settings done");
        }
        else {
            mActions.sendSpecialKey(Actions.SpecialKey.BACK);
            mAsserter.ok(waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_URL), "Waiting to exit settings", "Exit settings done");
        }
    }
}
