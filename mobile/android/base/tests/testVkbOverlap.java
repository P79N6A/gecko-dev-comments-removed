package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.PaintedSurface;

import android.net.Uri;








public class testVkbOverlap extends PixelTest {
    private static final int CURSOR_BLINK_PERIOD = 500;
    private static final int LESS_THAN_CURSOR_BLINK_PERIOD = CURSOR_BLINK_PERIOD - 50;
    private static final int PAGE_SETTLE_TIME = 5000;

    public void testVkbOverlap() {
        blockForGeckoReady();
        testSetup("initial-scale=1.0, user-scalable=no", false);
        testSetup("initial-scale=1.0", false);
        testSetup("", "phone".equals(mDevice.type));
    }

    private void testSetup(String viewport, boolean shouldZoom) {
        loadAndPaint(getAbsoluteUrl("/robocop/test_viewport.sjs?metadata=" + Uri.encode(viewport)));

        
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());
        meh.dragSync(10, 150, 10, 50);

        
        int greenPixelCount = 0;

        PaintedSurface painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            greenPixelCount = countGreenPixels(painted);
        } finally {
            painted.close();
        }

        mAsserter.ok(greenPixelCount > 0, "testInputVisible", "Found " + greenPixelCount + " green pixels after scrolling");

        paintExpecter = mActions.expectPaint();
        
        meh.tap(5, mDriver.getGeckoHeight() - 5);

        
        
        
        
        try {
            Thread.sleep(PAGE_SETTLE_TIME);
        } catch (InterruptedException ie) {
            ie.printStackTrace();
        }

        
        
        paintExpecter.blockUntilClear(LESS_THAN_CURSOR_BLINK_PERIOD);
        paintExpecter.unregisterListener();
        painted = mDriver.getPaintedSurface();
        try {
            
            
            
            int newCount = countGreenPixels(painted);

            
            if (shouldZoom) {
                mAsserter.ok(newCount > greenPixelCount * 1.5, "testVkbOverlap", "Found " + newCount + " green pixels after tapping; expected " + greenPixelCount);
            } else {
                mAsserter.ok((Math.abs(greenPixelCount - newCount) / greenPixelCount < 0.1), "testVkbOverlap", "Found " + newCount + " green pixels after tapping; expected " + greenPixelCount);
            }
        } finally {
            painted.close();
        }
    }

    private int countGreenPixels(PaintedSurface painted) {
        int count = 0;
        for (int y = painted.getHeight() - 1; y >= 0; y--) {
            for (int x = painted.getWidth() - 1; x >= 0; x--) {
                int pixel = painted.getPixelAt(x, y);
                int r = (pixel >> 16) & 0xFF;
                int g = (pixel >> 8) & 0xFF;
                int b = (pixel & 0xFF);
                if (g > (r + 0x30) && g > (b + 0x30)) {
                    
                    
                    
                    
                    count++;
                }
                
                
            }
        }
        return count;
    }
}
