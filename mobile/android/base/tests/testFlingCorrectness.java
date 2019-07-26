package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;







public class testFlingCorrectness extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testFlingCorrectness() {
        String url = getAbsoluteUrl("/robocop/robocop_boxes.html");

        MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());

        blockForGeckoReady();

        
        loadAndVerifyBoxes(url);

        
        
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        meh.dragSync(10, 150, 10, 50);
        meh.dragSync(10, 150, 10, 50);
        PaintedSurface painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            checkScrollWithBoxes(painted, 0, 200);
        } finally {
            painted.close();
        }

        
        
        paintExpecter = mActions.expectPaint();
        meh.flingSync(10, 50, 10, 150, 15);
        painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            checkScrollWithBoxes(painted, 0, 0);
        } finally {
            painted.close();
        }
    }
}
