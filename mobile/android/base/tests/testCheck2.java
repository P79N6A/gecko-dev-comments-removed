



package org.mozilla.gecko.tests;

import org.json.JSONObject;

public class testCheck2 extends PixelTest {
    @Override
    protected Type getTestType() {
        return Type.TALOS;
    }

    public void testCheck2() {
        String url = getAbsoluteUrl("/startup_test/fennecmark/cnn/cnn.com/index.html");

        
        JSONObject jsonPref = new JSONObject();
        try {
            jsonPref.put("name", "browser.ui.zoom.force-user-scalable");
            jsonPref.put("type", "bool");
            jsonPref.put("value", true);
            setPreferenceAndWaitForChange(jsonPref);
        } catch (Exception ex) {
            mAsserter.ok(false, "exception in testCheck2", ex.toString());
        }

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
            e.printStackTrace();
            mAsserter.ok(false, "Exception while replaying events", e.toString());
        }

        mAsserter.dumpLog("__start_report" + completeness + "__end_report");
        System.out.println("Completeness score: " + completeness);
        long msecs = System.currentTimeMillis();
        mAsserter.dumpLog("__startTimestamp" + msecs + "__endTimestamp");
    }
}
