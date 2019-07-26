package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import android.app.Activity;
import android.graphics.Color;

public class testFindInPage extends PixelTest {
    private static final int WAIT_FOR_TEST = 3000;
    protected Element next, close;
    int height,width;

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testFindInPage() {
        blockForGeckoReady();
        String url = getAbsoluteUrl("/robocop/robocop_text_page.html");
        loadAndPaint(url);

        
        height = mDriver.getGeckoHeight()/8;
        width = mDriver.getGeckoWidth()/8;

        












        
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        findText("Robocop", 3);
        PaintedSurface painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            mAsserter.isnotpixel(painted.getPixelAt(width,height), 255, 0, 0, "Pixel at " + String.valueOf(width) + "," + String.valueOf(height));
        } finally {
            painted.close();
        }
    }

    public void findText(String text, int nrOfMatches){
        selectMenuItem("Find in Page");
        close = mDriver.findElement(getActivity(), R.id.find_close);
        boolean success = waitForTest ( new BooleanTest() {
            public boolean test() {
                next = mDriver.findElement(getActivity(), R.id.find_next);
                if (next != null) {
                    return true;
                } else {
                    return false;
                }
            }
        }, WAIT_FOR_TEST);
        mAsserter.ok(success, "Looking for the next search match button in the Find in Page UI", "Found the next match button");

        
        
        mSolo.sleep(500);

        mActions.sendKeys(text);
        mActions.sendSpecialKey(Actions.SpecialKey.ENTER);

        
        for (int i=1;i < nrOfMatches;i++) {
            success = waitForTest ( new BooleanTest() {
                public boolean test() {
                    if (next.click()) {
                        return true;
                    } else {
                        return false;
                    }
                }
            }, WAIT_FOR_TEST);
            mSolo.sleep(500); 
            mAsserter.ok(success, "Checking if the next button was clicked", "button was clicked");
        }
        close.click(); 
    }
}
