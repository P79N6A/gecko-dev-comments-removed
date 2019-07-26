package org.mozilla.gecko.tests;

import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;









public class testThumbnails extends BaseTest {
    private int mTopSitesId;
    private int mThumbnailId;
    private int mTitleId;

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testThumbnails() {
        mTopSitesId = mDriver.findElement(getActivity(), "top_sites_grid").getId();
        mThumbnailId = mDriver.findElement(getActivity(), "thumbnail").getId();
        mTitleId = mDriver.findElement(getActivity(), "title").getId();

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
        inputAndLoadUrl("about:home");
        waitForTest(new ThumbnailTest(site1Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site1Title), Color.GREEN, "Top site thumbnail updated for HTTP 200");
        waitForTest(new ThumbnailTest(site2Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site2Title), Color.GREEN, "Top site thumbnail updated for HTTP 200");

        
        inputAndLoadUrl(site1Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl(site2Url);
        mSolo.sleep(thumbnailDelay);
        inputAndLoadUrl("about:home");
        waitForTest(new ThumbnailTest(site1Title, Color.RED), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site1Title), Color.RED, "Top site thumbnail updated for HTTP 200");
        waitForTest(new ThumbnailTest(site2Title, Color.GREEN), 5000);
        mAsserter.is(getTopSiteThumbnailColor(site2Title), Color.GREEN, "Top site thumbnail not updated for HTTP 404");

        
        try {
            ClassLoader cl = getActivity().getApplicationContext().getClassLoader();
            Class browserDB = cl.loadClass("org.mozilla.gecko.db.BrowserDB");
            
            byte[] thumbnailData = (byte[])browserDB
                .getMethod("getThumbnailForUrl", ContentResolver.class, String.class)
                .invoke(null, getActivity().getContentResolver(), site1Url);
            mAsserter.ok(thumbnailData != null && thumbnailData.length > 0, "Checking for thumbnail data", "No thumbnail data found");
            
            browserDB.getMethod("removeThumbnails", ContentResolver.class)
                .invoke(null, getActivity().getContentResolver());
            
            thumbnailData = (byte[])browserDB
                .getMethod("getThumbnailForUrl", ContentResolver.class, String.class)
                .invoke(null, getActivity().getContentResolver(), site1Url);
            mAsserter.ok(thumbnailData == null || thumbnailData.length == 0, "Checking for thumbnail data", "Thumbnail data found");
        } catch (Exception e) {
            mAsserter.ok(false, "Testing removing thumbnails", e.toString());
            mAsserter.dumpLog(e.toString(), e);
        }
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
        ViewGroup topSites = (ViewGroup) getActivity().findViewById(mTopSitesId);
        if (topSites != null) {
            final int childCount = topSites.getChildCount();
            for (int i = 0; i < childCount; i++) {
                View child = topSites.getChildAt(i);
                if (child != null) {
                    TextView titleView = (TextView) child.findViewById(mTitleId);
                    if (titleView != null) {
                        if (titleView.getText().equals(title)) {
                            ImageView thumbnailView = (ImageView) child.findViewById(mThumbnailId);
                            if (thumbnailView != null) {
                                Bitmap thumbnail = ((BitmapDrawable) thumbnailView.getDrawable()).getBitmap();
                                return thumbnail.getPixel(0, 0);
                            } else {
                                mAsserter.dumpLog("getTopSiteThumbnailColor: unable to find mThumbnailId: "+mThumbnailId);
                            }
                        }
                    } else {
                        mAsserter.dumpLog("getTopSiteThumbnailColor: unable to find mTitleId: "+mTitleId);
                    }
                } else {
                    mAsserter.dumpLog("getTopSiteThumbnailColor: skipped null child at index "+i);
                }
            }
        } else {
            mAsserter.dumpLog("getTopSiteThumbnailColor: unable to find mTopSitesId: "+mTopSitesId);
        }
        return -1;
    }
}
