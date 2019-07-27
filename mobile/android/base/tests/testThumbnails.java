package org.mozilla.gecko.tests;

import org.mozilla.gecko.db.BrowserDB;

import android.content.ContentResolver;
import android.graphics.Color;

import com.jayway.android.robotium.solo.Condition;









public class testThumbnails extends BaseTest {
    public void testThumbnails() {
        final String site1Url = getAbsoluteUrl("/robocop/robocop_404.sjs?type=changeColor");
        final String site2Url = getAbsoluteUrl("/robocop/robocop_404.sjs?type=do404");
        final String site1Title = "changeColor";
        final String site2Title = "do404";

        
        
        
        final int thumbnailDelay = 3000;

        blockForGeckoReady();

        
        inputAndLoadUrl(site1Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(site2Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(mStringHelper.ABOUT_HOME_URL);
        waitForCondition(new ThumbnailTest(site1Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site1Title), Color.GREEN, "Top site thumbnail updated for HTTP 200");
        waitForCondition(new ThumbnailTest(site2Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site2Title), Color.GREEN, "Top site thumbnail updated for HTTP 200");

        
        inputAndLoadUrl(site1Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(site2Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(mStringHelper.ABOUT_HOME_URL);
        waitForCondition(new ThumbnailTest(site1Title, Color.RED), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site1Title), Color.RED, "Top site thumbnail updated for HTTP 200");
        waitForCondition(new ThumbnailTest(site2Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site2Title), Color.GREEN, "Top site thumbnail not updated for HTTP 404");

        
        final ContentResolver resolver = getActivity().getContentResolver();
        final DatabaseHelper helper = new DatabaseHelper(getActivity(), mAsserter);
        final BrowserDB db = helper.getProfileDB();

        
        byte[] thumbnailData = db.getThumbnailForUrl(resolver, site1Url);
        mAsserter.ok(thumbnailData != null && thumbnailData.length > 0, "Checking for thumbnail data", "No thumbnail data found");
        
        db.removeThumbnails(resolver);
        
        thumbnailData = db.getThumbnailForUrl(resolver, site1Url);
        mAsserter.ok(thumbnailData == null || thumbnailData.length == 0, "Checking for thumbnail data", "Thumbnail data found");
    }

    private class ThumbnailTest implements Condition {
        private final String mTitle;
        private final int mColor;

        public ThumbnailTest(String title, int color) {
            mTitle = title;
            mColor = color;
        }

        @Override
        public boolean isSatisfied() {
            return getTopSiteThumbnailColor(mTitle) == mColor;
        }
    }

    private int getTopSiteThumbnailColor(String title) {
        
        return -1;




























    }
}
