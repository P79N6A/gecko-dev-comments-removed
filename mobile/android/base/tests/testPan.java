package org.mozilla.gecko.tests;






public class testPan extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_TALOS;
    }

    public void testPan() {
        String url = getAbsoluteUrl("/startup_test/fennecmark/wikipedia.html");

        blockForGeckoReady();

        loadAndPaint(url);

        mDriver.setupScrollHandling();

        
        int midX = mDriver.getGeckoLeft() + mDriver.getGeckoWidth()/2;
        int midY = mDriver.getGeckoTop() + mDriver.getGeckoHeight()/2;
        int endY = mDriver.getGeckoTop() + mDriver.getGeckoHeight()/10;

        mDriver.startFrameRecording();

        int i = 0;
        
        do {
            mActions.drag(midX, midX, midY, endY);
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            i++;
        } while (i < 1000 && mDriver.getScrollHeight() + 2 * mDriver.getHeight() < mDriver.getPageHeight());
        

        int frames = mDriver.stopFrameRecording();
        mAsserter.dumpLog("__start_report" + Integer.toString(frames) + "__end_report");
        long msecs = System.currentTimeMillis();
        mAsserter.dumpLog("__startTimestamp" + msecs + "__endTimestamp");
    }
}
