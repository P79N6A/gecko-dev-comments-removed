package org.mozilla.gecko.tests;

import org.mozilla.gecko.util.HardwareUtils;





public class testTitleBar extends PixelTest {
    public void testTitleBar() {
        
        if (HardwareUtils.isTablet()) {
            return;
        }

        blockForGeckoReady();
        checkOption();
    }

    public void checkOption() {

        String blank1 = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String title = mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE;

        
        loadUrlAndWait(blank1);
        verifyUrlBarTitle(blank1);

        
        selectOption(mStringHelper.SHOW_PAGE_TITLE_LABEL);
        loadUrlAndWait(blank1);
        verifyUrlBarTitle(title);

        
        selectOption(mStringHelper.SHOW_PAGE_ADDRESS_LABEL);
        loadUrlAndWait(blank1);
        verifyUrlBarTitle(blank1);
    }

    
    public void selectOption(String option) {
        selectSettingsItem(mStringHelper.DISPLAY_SECTION_LABEL, mStringHelper.TITLE_BAR_LABEL);
        mAsserter.ok(waitForText(mStringHelper.SHOW_PAGE_TITLE_LABEL), "Waiting for the pop-up to open", "Pop up with the options was openend");
        mSolo.clickOnText(option);
        mAsserter.ok(waitForText(mStringHelper.CHARACTER_ENCODING_LABEL), "Waiting to press the option", "The pop-up is dismissed once clicked");
        if (mDevice.type.equals("phone")) {
            mSolo.goBack();
            mAsserter.ok(waitForText(mStringHelper.CUSTOMIZE_SECTION_LABEL), "Waiting to perform one back", "One back performed");
            mSolo.goBack();
            mAsserter.ok(waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL), "Waiting to exit settings", "Exit settings done");
        }
        else {
            mSolo.goBack();
            mAsserter.ok(waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL), "Waiting to exit settings", "Exit settings done");
        }
    }
}
