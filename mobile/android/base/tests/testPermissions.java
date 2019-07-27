



package org.mozilla.gecko.tests;

import java.util.ArrayList;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.PaintedSurface;

import android.widget.CheckBox;

public class testPermissions extends PixelTest {
    public void testPermissions() {
        blockForGeckoReady();

        geolocationTest();
    }

    private void geolocationTest() {
        Actions.RepeatedEventExpecter paintExpecter;

        
        loadAndPaint(getAbsoluteUrl(mStringHelper.ROBOCOP_GEOLOCATION_URL));
        waitForText("wants your location");

        
        ArrayList<CheckBox> checkBoxes = mSolo.getCurrentViews(CheckBox.class);
        mAsserter.ok(checkBoxes.size() == 1, "checkbox count", "only one checkbox visible");
        mAsserter.ok(mSolo.isCheckBoxChecked(0), "checkbox checked", "checkbox is checked");
        mSolo.clickOnCheckBox(0);
        mAsserter.ok(!mSolo.isCheckBoxChecked(0), "checkbox not checked", "checkbox is not checked");

        
        paintExpecter = mActions.expectPaint();
        mSolo.clickOnText("Share");
        PaintedSurface painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            mAsserter.ispixel(painted.getPixelAt(10, 10), 0, 0x80, 0, "checking page background is green");
        } finally {
            painted.close();
        }

        
        reloadAndPaint();
        waitForText("wants your location");

        
        mAsserter.ok(mSolo.isCheckBoxChecked(0), "checkbox checked", "checkbox is checked");

        
        paintExpecter = mActions.expectPaint();
        mSolo.clickOnText("Share");
        painted = waitForPaint(paintExpecter);
        paintExpecter.unregisterListener();
        try {
            mAsserter.ispixel(painted.getPixelAt(10, 10), 0, 0x80, 0, "checking page background is green");
        } finally {
            painted.close();
        }

        
        painted = reloadAndGetPainted();
        try {
            mAsserter.ispixel(painted.getPixelAt(10, 10), 0, 0x80, 0, "checking page background is green");
        } finally {
            painted.close();
        }
    }
}
