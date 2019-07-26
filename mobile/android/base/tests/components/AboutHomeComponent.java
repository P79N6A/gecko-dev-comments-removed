



package org.mozilla.gecko.tests.components;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertEquals;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import org.mozilla.gecko.R;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.helpers.DeviceHelper;
import org.mozilla.gecko.tests.helpers.WaitHelper;

import android.support.v4.view.ViewPager;
import android.view.View;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;




public class AboutHomeComponent extends BaseComponent {
    private static final String LOGTAG = AboutHomeComponent.class.getSimpleName();

    
    public enum PanelType {
        HISTORY,
        TOP_SITES,
        BOOKMARKS,
        READING_LIST,
        RECENT_TABS
    }

    
    
    
    private enum PhonePanel {
        RECENT_TABS,
        HISTORY,
        TOP_SITES,
        BOOKMARKS,
        READING_LIST
    }

    
    private enum TabletPanel {
        TOP_SITES,
        BOOKMARKS,
        READING_LIST,
        HISTORY,
        RECENT_TABS
    }

    
    
    private static final float SWIPE_PERCENTAGE = 0.70f;

    public AboutHomeComponent(final UITestContext testContext) {
        super(testContext);
    }

    private View getHomePagerContainer() {
        return mSolo.getView(R.id.home_pager_container);
    }

    private ViewPager getHomePagerView() {
        return (ViewPager) mSolo.getView(R.id.home_pager);
    }

    private View getHomeBannerView() {
        return mSolo.getView(R.id.home_banner);
    }

    public AboutHomeComponent assertCurrentPanel(final PanelType expectedPanel) {
        assertVisible();

        final int expectedPanelIndex = getPanelIndexForDevice(expectedPanel.ordinal());
        fAssertEquals("The current HomePager panel is " + expectedPanel,
                     expectedPanelIndex, getHomePagerView().getCurrentItem());
        return this;
    }

    public AboutHomeComponent assertNotVisible() {
        fAssertTrue("The HomePager is not visible",
                    getHomePagerContainer().getVisibility() != View.VISIBLE ||
                    getHomePagerView().getVisibility() != View.VISIBLE);
        return this;
    }

    public AboutHomeComponent assertVisible() {
        fAssertTrue("The HomePager is visible",
                    getHomePagerContainer().getVisibility() == View.VISIBLE &&
                    getHomePagerView().getVisibility() == View.VISIBLE);
        return this;
    }

    public AboutHomeComponent assertBannerNotVisible() {
        View banner = getHomeBannerView();
        fAssertTrue("The HomeBanner is not visible",
                    getHomePagerContainer().getVisibility() != View.VISIBLE ||
                    banner.getVisibility() != View.VISIBLE ||
                    banner.getTranslationY() == banner.getHeight());
        return this;
    }

    public AboutHomeComponent assertBannerVisible() {
        fAssertTrue("The HomeBanner is visible",
                    getHomePagerContainer().getVisibility() == View.VISIBLE &&
                    getHomeBannerView().getVisibility() == View.VISIBLE);
        return this;
    }

    public AboutHomeComponent assertBannerText(String text) {
        assertBannerVisible();

        final TextView textView = (TextView) getHomeBannerView().findViewById(R.id.text);
        fAssertEquals("The correct HomeBanner text is shown",
                     text, textView.getText().toString());
        return this;
    }

    public AboutHomeComponent clickOnBanner() {
        assertBannerVisible();

        mTestContext.dumpLog(LOGTAG, "Clicking on HomeBanner.");
        mSolo.clickOnView(getHomeBannerView());
        return this;
    }

    public AboutHomeComponent dismissBanner() {
        assertBannerVisible();

        mTestContext.dumpLog(LOGTAG, "Clicking on HomeBanner close button.");
        mSolo.clickOnView(getHomeBannerView().findViewById(R.id.close));
        return this;
    }

    public AboutHomeComponent swipeToPanelOnRight() {
        mTestContext.dumpLog(LOGTAG, "Swiping to the panel on the right.");
        swipeToPanel(Solo.RIGHT);
        return this;
    }

    public AboutHomeComponent swipeToPanelOnLeft() {
        mTestContext.dumpLog(LOGTAG, "Swiping to the panel on the left.");
        swipeToPanel(Solo.LEFT);
        return this;
    }

    private void swipeToPanel(final int panelDirection) {
        fAssertTrue("Swiping in a valid direction",
                panelDirection == Solo.LEFT || panelDirection == Solo.RIGHT);
        assertVisible();

        final int panelIndex = getHomePagerView().getCurrentItem();

        mSolo.scrollViewToSide(getHomePagerView(), panelDirection, SWIPE_PERCENTAGE);

        
        final int unboundedPanelIndex = panelIndex + (panelDirection == Solo.LEFT ? -1 : 1);
        final int panelCount = DeviceHelper.isTablet() ?
                TabletPanel.values().length : PhonePanel.values().length;
        final int maxPanelIndex = panelCount - 1;
        final int expectedPanelIndex = Math.min(Math.max(0, unboundedPanelIndex), maxPanelIndex);

        waitForPanelIndex(expectedPanelIndex);
    }

    private void waitForPanelIndex(final int expectedIndex) {
        final String panelName;
        if (DeviceHelper.isTablet()) {
            panelName = TabletPanel.values()[expectedIndex].name();
        } else {
            panelName = PhonePanel.values()[expectedIndex].name();
        }

        WaitHelper.waitFor("HomePager " + panelName + " panel", new Condition() {
            @Override
            public boolean isSatisfied() {
                return (getHomePagerView().getCurrentItem() == expectedIndex);
            }
        });
    }

    



    private int getPanelIndexForDevice(final int panelIndex) {
        final String panelName = PanelType.values()[panelIndex].name();
        final Class devicePanelEnum =
                DeviceHelper.isTablet() ? TabletPanel.class : PhonePanel.class;
        return Enum.valueOf(devicePanelEnum, panelName).ordinal();
    }
}
