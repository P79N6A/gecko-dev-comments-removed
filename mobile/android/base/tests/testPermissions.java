package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

import android.widget.CheckBox;
import java.util.ArrayList;

public class testPermissions extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testPermissions() {
        blockForGeckoReady();

        geolocationTest();
    }

    private void geolocationTest() {
        Actions.RepeatedEventExpecter paintExpecter;

        
        loadAndPaint(getAbsoluteUrl("/robocop/robocop_geolocation.html"));
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
