package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.helpers.NavigationHelper;




public class testReaderModeTitle extends UITest {
    public void testReaderModeTitle() {
        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_READER_MODE_BASIC_ARTICLE);

        mToolbar.pressReaderModeButton();

        mToolbar.assertTitle(StringHelper.ROBOCOP_READER_MODE_BASIC_ARTICLE);
    }
}
