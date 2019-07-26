package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;







public class testOverscroll extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testOverscroll() {
        String url = getAbsoluteUrl("/robocop/robocop_boxes.html");

        MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());

        blockForGeckoReady();

        
        loadAndVerifyBoxes(url);

        
        
        
        
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        meh.dragSync(10, 50, 10, 150);
        PaintedSurface painted = waitWithNoPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            checkScrollWithBoxes(painted, 0, 0);
        } finally {
            painted.close();
        }

        
        paintExpecter = mActions.expectPaint();
        meh.dragSync(50, 10, 150, 10);
        painted = waitWithNoPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            checkScrollWithBoxes(painted, 0, 0);
        } finally {
            painted.close();
        }
    }
}
