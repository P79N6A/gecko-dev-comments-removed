



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.PaintedSurface;







public class testFlingCorrectness extends PixelTest {
    public void testFlingCorrectness() {
        String url = getAbsoluteUrl(mStringHelper.ROBOCOP_BOXES_URL);

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
