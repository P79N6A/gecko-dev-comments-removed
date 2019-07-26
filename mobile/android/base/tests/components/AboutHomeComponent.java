



package org.mozilla.gecko.tests.components;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.home.HomePager.Page;
import org.mozilla.gecko.R;
import org.mozilla.gecko.tests.helpers.*;
import org.mozilla.gecko.tests.UITestContext;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;

import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;




public class AboutHomeComponent extends BaseComponent {
    public AboutHomeComponent(final UITestContext testContext) {
        super(testContext);
    }

    private ViewPager getHomePagerView() {
        return (ViewPager) mSolo.getView(R.id.home_pager);
    }

    public AboutHomeComponent assertCurrentPage(final Page expectedPage) {
        assertVisible();

        
        
        
        int expectedPageIndex = expectedPage.ordinal();
        if (DeviceHelper.isTablet()) {
            
            expectedPageIndex -= 1;
            expectedPageIndex =
                    (expectedPageIndex >= 0) ? expectedPageIndex : Page.values().length - 1;
        }

        assertEquals("The current HomePager page is " + Page.values()[expectedPageIndex],
                     expectedPageIndex, getHomePagerView().getCurrentItem());
        return this;
    }

    public AboutHomeComponent assertNotVisible() {
        assertFalse("The HomePager is not visible",
                    getHomePagerView().getVisibility() == View.VISIBLE);
        return this;
    }

    public AboutHomeComponent assertVisible() {
        assertEquals("The HomePager is visible",
                     View.VISIBLE, getHomePagerView().getVisibility());
        return this;
    }

    
    public AboutHomeComponent swipeToPageOnRight() {
        swipe(Solo.LEFT);
        return this;
    }

    public AboutHomeComponent swipeToPageOnLeft() {
        swipe(Solo.RIGHT);
        return this;
    }

    private void swipe(final int direction) {
        assertVisible();

        final int pageIndex = getHomePagerView().getCurrentItem();
        if (direction == Solo.LEFT) {
            GestureHelper.swipeLeft();
        } else {
            GestureHelper.swipeRight();
        }

        final PagerAdapter adapter = getHomePagerView().getAdapter();
        assertNotNull("The HomePager's PagerAdapter is not null", adapter);

        
        final int unboundedPageIndex = pageIndex + (direction == Solo.LEFT ? 1 : -1);
        final int expectedPageIndex = Math.min(Math.max(0, unboundedPageIndex), adapter.getCount() - 1);

        waitForPageIndex(expectedPageIndex);
    }

    private void waitForPageIndex(final int expectedIndex) {
        final String pageName = Page.values()[expectedIndex].toString();
        WaitHelper.waitFor("HomePager " + pageName + " page", new Condition() {
            @Override
            public boolean isSatisfied() {
                return (getHomePagerView().getCurrentItem() == expectedIndex);
            }
        });
    }
}
