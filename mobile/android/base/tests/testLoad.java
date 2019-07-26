package org.mozilla.gecko.tests;







public class testLoad extends PixelTest {
    public void testLoad() {
        String url = getAbsoluteUrl("/robocop/robocop_boxes.html");

        blockForGeckoReady();

        loadAndVerifyBoxes(url);

        verifyUrl(url);
    }
}
