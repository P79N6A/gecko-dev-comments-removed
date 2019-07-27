package org.mozilla.gecko.tests;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.home.HomePager;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.GridView;
import android.widget.ListView;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Condition;





public class testShareLink extends AboutHomeTest {
    String url;
    String urlTitle = mStringHelper.ROBOCOP_BIG_LINK_TITLE;

    public void testShareLink() {
        url = getAbsoluteUrl(mStringHelper.ROBOCOP_BIG_LINK_URL);
        ArrayList<String> shareOptions;
        blockForGeckoReady();

        
        openAboutHomeTab(AboutHomeTabs.READING_LIST);

        inputAndLoadUrl(url);
        verifyUrlBarTitle(url); 

        selectMenuItem(mStringHelper.SHARE_LABEL);
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

        final ListView bookmarksList = findListViewWithTag(HomePager.LIST_TAG_BOOKMARKS);
        mAsserter.is(waitForNonEmptyListToLoad(bookmarksList), true, "list is properly loaded");

        int headerViewsCount = bookmarksList.getHeaderViewsCount();
        View bookmarksItem = bookmarksList.getChildAt(headerViewsCount);
        if (bookmarksItem == null) {
            mAsserter.dumpLog("no child at index " + headerViewsCount + "; waiting for one...");
            Condition listWaitCondition = new Condition() {
                @Override
                public boolean isSatisfied() {
                    if (bookmarksList.getChildAt(bookmarksList.getHeaderViewsCount()) == null)
                        return false;
                    return true;
                }
            };
            waitForCondition(listWaitCondition, MAX_WAIT_MS);
            headerViewsCount = bookmarksList.getHeaderViewsCount();
            bookmarksItem = bookmarksList.getChildAt(headerViewsCount);
        }

        mSolo.clickLongOnView(bookmarksItem);
        verifySharePopup(shareOptions,"bookmarks");

        
        
        inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL));
        inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_02_URL));
        inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_03_URL));
        inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_04_URL));
        if (mDevice.type.equals("tablet")) {
            
            inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_05_URL));
            inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_BOXES_URL));
            inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_SEARCH_URL));
            inputAndLoadUrl(getAbsoluteUrl(mStringHelper.ROBOCOP_TEXT_PAGE_URL));
        }

        
        openAboutHomeTab(AboutHomeTabs.TOP_SITES);

        
        int width = mDriver.getGeckoWidth();
        int height = mDriver.getGeckoHeight();
        mActions.drag(width / 2, width / 2, height - 10, height / 2);

        ListView topSitesList = findListViewWithTag(HomePager.LIST_TAG_TOP_SITES);
        mAsserter.is(waitForNonEmptyListToLoad(topSitesList), true, "list is properly loaded");
        View mostVisitedItem = topSitesList.getChildAt(topSitesList.getHeaderViewsCount());
        mSolo.clickLongOnView(mostVisitedItem);
        verifySharePopup(shareOptions,"top_sites");

        
        openAboutHomeTab(AboutHomeTabs.HISTORY);

        ListView mostRecentList = findListViewWithTag(HomePager.LIST_TAG_HISTORY);
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

    
    public ArrayList<String> getShareOptions() {
        ArrayList<String> shareOptions = new ArrayList<>();
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
        ArrayList<String> displayedOptions = new ArrayList<>();
        AbsListView shareMenu = getDisplayedShareList();
        getGroupTextViews(shareMenu, displayedOptions);
        return displayedOptions;
    }

    public ArrayList<String> getShareSubMenuOption() {
        ArrayList<String> displayedOptions = new ArrayList<>();
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
        boolean success = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
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
