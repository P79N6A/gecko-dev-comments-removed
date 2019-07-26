package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

public class test_bug720538 extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void test_bug720538() {
        String url = getAbsoluteUrl("/robocop/test_bug720538.html");

        blockForGeckoReady();

        












        PaintedSurface painted = loadAndGetPainted(url);

        try {
            
            mAsserter.ispixel(painted.getPixelAt(100, 100), 0, 0, 0xFF, "Ensuring double-tap point is in the iframe");
        } finally {
            painted.close();
        }

        
        
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());
        meh.doubleTap(100, 100);
        painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();

        try {
            
            
            
            mAsserter.ispixel(painted.getPixelAt(0, 100), 0, 0x80, 0, "Checking page background to the left of the iframe");
            mAsserter.ispixel(painted.getPixelAt(50, 100), 0, 0, 0xFF, "Checking for iframe a few pixels from the left edge");
            mAsserter.ispixel(painted.getPixelAt(mDriver.getGeckoWidth() - 51, 100), 0, 0, 0xFF, "Checking for iframe a few pixels from the right edge");
            mAsserter.ispixel(painted.getPixelAt(mDriver.getGeckoWidth() - 1, 100), 0, 0x80, 0, "Checking page background the right of the iframe");
        } finally {
            painted.close();
        }

        
        paintExpecter = mActions.expectPaint();
        meh.doubleTap(mDriver.getGeckoWidth() / 2, 100);
        painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();

        try {
            
            
            
            for (int y = 2; y < 10; y++) {
                for (int x = 0; x < 10; x++) {
                    mAsserter.dumpLog("Pixel at " + x + ", " + (mDriver.getGeckoHeight() - y) + ": " + Integer.toHexString(painted.getPixelAt(x, mDriver.getGeckoHeight() - y)));
                }
            }
            mAsserter.ispixel(painted.getPixelAt(0, mDriver.getGeckoHeight() - 2), 0, 0x80, 0, "Checking bottom-left corner of viewport");
        } finally {
            painted.close();
        }
    }
}
