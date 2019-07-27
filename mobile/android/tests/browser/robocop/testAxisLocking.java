



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.PaintedSurface;









public class testAxisLocking extends PixelTest {
    public void testAxisLocking() {
        String url = getAbsoluteUrl(mStringHelper.ROBOCOP_BOXES_URL);

        MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());

        blockForGeckoReady();

        
        loadAndVerifyBoxes(url);

        
        
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        meh.dragSync(20, 150, 10, 50);
        PaintedSurface painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            checkScrollWithBoxes(painted, 0, 100);
            
            
            int[] color = getBoxColorAt(0, 100);
            mAsserter.ispixel(painted.getPixelAt(99, 0), color[0], color[1], color[2], "Pixel at 99, 0 indicates no horizontal scroll");

            
            
            paintExpecter = mActions.expectPaint();
            meh.dragSync(150, 150, 50, 50);
        } finally {
            painted.close();
        }

        painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            checkScrollWithBoxes(painted, 100, 200);
        } finally {
            painted.close();
        }
    }
}
