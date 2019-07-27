package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;




public class testReaderModeTitle extends UITest {
    public void testReaderModeTitle() {
        GeckoHelper.blockForReady();

        NavigationHelper.enterAndLoadUrl(mStringHelper.ROBOCOP_READER_MODE_BASIC_ARTICLE);

        mToolbar.pressReaderModeButton();

        mToolbar.assertTitle(mStringHelper.ROBOCOP_READER_MODE_BASIC_ARTICLE);
    }
}
