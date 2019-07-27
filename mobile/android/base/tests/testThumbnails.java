package org.mozilla.gecko.tests;

import org.mozilla.gecko.db.BrowserDB;

import android.content.ContentResolver;
import android.graphics.Color;









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
        inputAndLoadUrl(StringHelper.ABOUT_HOME_URL);
        waitForTest(new ThumbnailTest(site1Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site1Title), Color.GREEN, "Top site thumbnail updated for HTTP 200");
        waitForTest(new ThumbnailTest(site2Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site2Title), Color.GREEN, "Top site thumbnail updated for HTTP 200");

        
        inputAndLoadUrl(site1Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(site2Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(StringHelper.ABOUT_HOME_URL);
        waitForTest(new ThumbnailTest(site1Title, Color.RED), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site1Title), Color.RED, "Top site thumbnail updated for HTTP 200");
        waitForTest(new ThumbnailTest(site2Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site2Title), Color.GREEN, "Top site thumbnail not updated for HTTP 404");

        
        final ContentResolver resolver = getActivity().getContentResolver();
        
        byte[] thumbnailData = BrowserDB.getThumbnailForUrl(resolver, site1Url);
        mAsserter.ok(thumbnailData != null && thumbnailData.length > 0, "Checking for thumbnail data", "No thumbnail data found");
        
        BrowserDB.removeThumbnails(resolver);
        
        thumbnailData = BrowserDB.getThumbnailForUrl(resolver, site1Url);
        mAsserter.ok(thumbnailData == null || thumbnailData.length == 0, "Checking for thumbnail data", "Thumbnail data found");
    }

    private class ThumbnailTest implements BooleanTest {
        private String mTitle;
        private int mColor;

        public ThumbnailTest(String title, int color) {
            mTitle = title;
            mColor = color;
        }

        @Override
        public boolean test() {
            return getTopSiteThumbnailColor(mTitle) == mColor;
        }
    }

    private int getTopSiteThumbnailColor(String title) {
        
        return -1;




























    }
}
