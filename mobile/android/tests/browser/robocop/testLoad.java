



package org.mozilla.gecko.tests;







public class testLoad extends PixelTest {
    public void testLoad() {
        String url = getAbsoluteUrl(mStringHelper.ROBOCOP_BOXES_URL);

        blockForGeckoReady();

        loadAndVerifyBoxes(url);

        verifyUrl(url);
    }
}
