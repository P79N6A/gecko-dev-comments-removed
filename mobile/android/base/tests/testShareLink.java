package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.GridView;
import android.widget.ListView;
import android.widget.TextView;
import java.util.ArrayList;
import java.util.List;





public class testShareLink extends AboutHomeTest {
    String url;
    String urlTitle = "Big Link";

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testShareLink() {
        url = getAbsoluteUrl("/robocop/robocop_big_link.html");
        ArrayList<String> shareOptions;
        blockForGeckoReady();

        
        openAboutHomeTab(AboutHomeTabs.READING_LIST);

        inputAndLoadUrl(url);
        verifyPageTitle(urlTitle); 

        selectMenuItem("Share");
        if (Build.VERSION.SDK_INT >= 14) {
            
            waitForText("Sync$");
        } else {
            waitForText("Share via");
        }

        
        shareOptions = getShareOptions();
        ArrayList<String> displayedOptions = getShareOptionsList();
        for (String option:shareOptions) {
             
             mAsserter.ok(optionDisplayed(option, displayedOptions), "Share option found", option);
        }

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK); 
        mSolo.clickLongOnText(urlTitle);
        verifySharePopup(shareOptions,"urlbar");

        
        float top = mDriver.getGeckoTop() + 30 * mDevice.density;
        float left = mDriver.getGeckoLeft() + mDriver.getGeckoWidth() / 2;
        mSolo.clickLongOnScreen(left, top);
        verifySharePopup("Share Link",shareOptions,"Link");

        
        openAboutHomeTab(AboutHomeTabs.BOOKMARKS);

        ListView bookmarksList = findListViewWithTag("bookmarks");
        mAsserter.is(waitForNonEmptyListToLoad(bookmarksList), true, "list is properly loaded");

        View bookmarksItem = bookmarksList.getChildAt(bookmarksList.getHeaderViewsCount());
        mSolo.clickLongOnView(bookmarksItem);
        verifySharePopup(shareOptions,"bookmarks");

        
        
        inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_blank_01.html"));
        inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_blank_02.html"));
        inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_blank_03.html"));
        inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_blank_04.html"));
        if (mDevice.type.equals("tablet")) {
            
            inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_blank_05.html"));
            inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_boxes.html"));
            inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_search.html"));
            inputAndLoadUrl(getAbsoluteUrl("/robocop/robocop_text_page.html"));
        }

        
        openAboutHomeTab(AboutHomeTabs.TOP_SITES);

        
        int width = mDriver.getGeckoWidth();
        int height = mDriver.getGeckoHeight();
        mActions.drag(width / 2, width / 2, height - 10, height / 2);

        ListView topSitesList = findListViewWithTag("top_sites");
        mAsserter.is(waitForNonEmptyListToLoad(topSitesList), true, "list is properly loaded");
        View mostVisitedItem = topSitesList.getChildAt(topSitesList.getHeaderViewsCount());
        mSolo.clickLongOnView(mostVisitedItem);
        verifySharePopup(shareOptions,"top_sites");

        
        openAboutHomeTab(AboutHomeTabs.MOST_RECENT);

        ListView mostRecentList = findListViewWithTag("most_recent");
        mAsserter.is(waitForNonEmptyListToLoad(mostRecentList), true, "list is properly loaded");

        
        View mostRecentItem = mostRecentList.getChildAt(mostRecentList.getHeaderViewsCount() + 1);
        mSolo.clickLongOnView(mostRecentItem);
        verifySharePopup(shareOptions,"most recent");
    }

    public void verifySharePopup(ArrayList<String> shareOptions, String openedFrom) {
        verifySharePopup("Share", shareOptions, openedFrom);
    }

    public void verifySharePopup(String shareItemText, ArrayList<String> shareOptions, String openedFrom) {
        waitForText(shareItemText);
        mSolo.clickOnText(shareItemText);
        waitForText("Share via");
        ArrayList<String> displayedOptions = getSharePopupOption();
        for (String option:shareOptions) {
             
             mAsserter.ok(optionDisplayed(option, displayedOptions), "Share option for " + openedFrom + (openedFrom.equals("urlbar") ? "" : " item") + " found", option);
        }
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        





        waitForText(urlTitle);
    }

    
    public ArrayList getShareOptions() {
        ArrayList<String> shareOptions = new ArrayList();
        Activity currentActivity = getActivity();
        final Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.putExtra(Intent.EXTRA_TEXT, url);
        shareIntent.putExtra(Intent.EXTRA_SUBJECT, "Robocop Blank 01");
        shareIntent.setType("text/plain");
        PackageManager pm = currentActivity.getPackageManager();
        List<ResolveInfo> activities = pm.queryIntentActivities(shareIntent, 0);
        for (ResolveInfo activity : activities) {
            shareOptions.add(activity.loadLabel(pm).toString());
        }
        return shareOptions;
    }

    
    private void getGroupTextViews(ViewGroup group, ArrayList<String> list) {
        for (int i = 0; i < group.getChildCount(); i++) {
            View child = group.getChildAt(i);
            if (child instanceof AbsListView) {
                getGroupTextViews((AbsListView)child, list);
            } else if (child instanceof ViewGroup) {
                getGroupTextViews((ViewGroup)child, list);
            } else if (child instanceof TextView) {
                String viewText = ((TextView)child).getText().toString();
                if (viewText != null && viewText.length() > 0) {
                    list.add(viewText);
                }
            }
        }
    }

    
    
    
    
    private void getGroupTextViews(AbsListView group, ArrayList<String> list) {
        for (int i = 0; i < group.getAdapter().getCount(); i++) {
            View child = group.getAdapter().getView(i, null, group);
            if (child instanceof AbsListView) {
                getGroupTextViews((AbsListView)child, list);
            } else if (child instanceof ViewGroup) {
                getGroupTextViews((ViewGroup)child, list);
            } else if (child instanceof TextView) {
                String viewText = ((TextView)child).getText().toString();
                if (viewText != null && viewText.length() > 0) {
                    list.add(viewText);
                }
            }
        }
    }

    public ArrayList<String> getSharePopupOption() {
        ArrayList<String> displayedOptions = new ArrayList();
        AbsListView shareMenu = getDisplayedShareList();
        getGroupTextViews(shareMenu, displayedOptions);
        return displayedOptions;
    }

    public ArrayList<String> getShareSubMenuOption() {
        ArrayList<String> displayedOptions = new ArrayList();
        AbsListView shareMenu = getDisplayedShareList();
        getGroupTextViews(shareMenu, displayedOptions);
        return displayedOptions;
    }

    public ArrayList<String> getShareOptionsList() {
        if (Build.VERSION.SDK_INT >= 14) {
            return getShareSubMenuOption();
        } else {
            return getSharePopupOption();
        }
    }

    private boolean optionDisplayed(String shareOption, ArrayList<String> displayedOptions) {
        for (String displayedOption: displayedOptions) {
            if (shareOption.equals(displayedOption)) {
                return true;
            }
        }
        return false;
    }

    private AbsListView mViewGroup;

    private AbsListView getDisplayedShareList() {
        mViewGroup = null;
        boolean success = waitForTest(new BooleanTest() {
            @Override
            public boolean test() {
                ArrayList<View> views = mSolo.getCurrentViews();
                for (View view : views) {
                    
                    
                    if (view instanceof ListView ||
                        view instanceof GridView) {
                        mViewGroup = (AbsListView)view;
                        return true;
                    }
                }
                return false;
            }
        }, MAX_WAIT_MS);
        mAsserter.ok(success,"Got the displayed share options?", "Got the share options view");
        return mViewGroup;
    }
}
