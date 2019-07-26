package org.mozilla.gecko.tests;

public class testCheck2 extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_TALOS;
    }

    public void testCheck2() {
        String url = getAbsoluteUrl("/startup_test/fennecmark/cnn/cnn.com/index.html");

        blockForGeckoReady();
        loadAndPaint(url);

        mDriver.setupScrollHandling();

        













        MotionEventReplayer mer = new MotionEventReplayer(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop(),
                mDriver.getGeckoWidth(), mDriver.getGeckoHeight());

        float completeness = 0.0f;
        mDriver.startCheckerboardRecording();
        
        try {
            mer.replayEvents(getAsset("testcheck2-motionevents"));
            
            Thread.sleep(1000);
            completeness = mDriver.stopCheckerboardRecording();
        } catch (Exception e) {
            mAsserter.ok(false, "Exception while replaying events", e.toString());
        }

        mAsserter.dumpLog("__start_report" + completeness + "__end_report");
        System.out.println("Completeness score: " + completeness);
        long msecs = System.currentTimeMillis();
        mAsserter.dumpLog("__startTimestamp" + msecs + "__endTimestamp");
    }
}
