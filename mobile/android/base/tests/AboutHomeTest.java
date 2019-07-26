package org.mozilla.gecko.tests;

import com.jayway.android.robotium.solo.Condition;
import org.mozilla.gecko.*;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.support.v4.view.ViewPager;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.GridView;
import android.widget.LinearLayout;
import android.widget.TabWidget;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.lang.reflect.Method;
import java.util.ArrayList;







abstract class AboutHomeTest extends PixelTest {
    protected enum AboutHomeTabs {HISTORY, MOST_RECENT, TABS_FROM_LAST_TIME, TOP_SITES, BOOKMARKS, READING_LIST};
    private ArrayList<String> aboutHomeTabs = new ArrayList<String>() {{
                  add("TOP_SITES");
                  add("BOOKMARKS");
                  add("READING_LIST");
              }};


    @Override
    protected void setUp() throws Exception {
        super.setUp();

        if (aboutHomeTabs.size() < 4) {
            
            if (mDevice.type.equals("phone")) {
                aboutHomeTabs.add(0, AboutHomeTabs.HISTORY.toString());
            } else {
                aboutHomeTabs.add(AboutHomeTabs.HISTORY.toString());
            }
        }
    }

    


    protected ListView getHistoryList(String waitText, int expectedChildCount) {
        return null;
    }
    protected ListView getHistoryList(String waitText) {
        return null;
    }

    
    protected boolean isBookmarkDisplayed(String url) {
        View bookmark = getDisplayedBookmark(url);
        if (bookmark != null) {
            return true;
        } else {
            return false;
        }
    }

    
    protected void loadBookmark(String url) {
        View bookmark = getDisplayedBookmark(url);
        if (bookmark != null) {
            Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
            mSolo.clickOnView(bookmark);
            contentEventExpecter.blockForEvent();
            contentEventExpecter.unregisterListener();
        } else {
            mAsserter.ok(false, url + " is not one of the displayed bookmarks", "Please make sure the url provided is ookmarked");
        }
    }

    
    protected void openBookmarkContextMenu(String url) {
        View bookmark = getDisplayedBookmark(url);
        if (bookmark != null) {
            mSolo.clickLongOnView(bookmark);
            waitForText("Share");
        } else {
            mAsserter.ok(false, url + " is not one of the displayed bookmarks", "Please make sure the url provided is bookmarked");
        }
    }

    
    protected View getDisplayedBookmark(String url) {
        openAboutHomeTab(AboutHomeTabs.BOOKMARKS);
        ListView bookmarksTabList = findListViewWithTag("bookmarks");
        waitForNonEmptyListToLoad(bookmarksTabList);
        ListAdapter adapter = bookmarksTabList.getAdapter();
        if (adapter != null) {
            for (int i = 0; i < adapter.getCount(); i++ ) {
                
                bookmarksTabList.smoothScrollToPosition(i);
                View bookmarkView = bookmarksTabList.getChildAt(i);
                if (bookmarkView instanceof android.widget.LinearLayout) {
                    ViewGroup bookmarkItemView = (ViewGroup) bookmarkView;
                    for (int j = 0 ; j < bookmarkItemView.getChildCount(); j++) {
                         View bookmarkContent = bookmarkItemView.getChildAt(j);
                         if (bookmarkContent instanceof android.widget.LinearLayout) {
                             ViewGroup bookmarkItemLayout = (ViewGroup) bookmarkContent;
                             for (int k = 0 ; k < bookmarkItemLayout.getChildCount(); k++) {
                                  
                                  TextView bookmarkTextContent = (TextView)bookmarkItemLayout.getChildAt(k);
                                  if (url.equals(bookmarkTextContent.getText().toString())) {
                                      return bookmarkView;
                                  }
                            }
                        }
                    }
                }
            }
        }
        return null;
    }

    






    protected boolean waitForListToLoad(final ListView listView, final int minSize) {
        Condition listWaitCondition = new Condition() {
            @Override
            public boolean isSatisfied() {
                if (listView == null) {
                    return false;
                }

                final ListAdapter adapter = listView.getAdapter();
                if (adapter == null) {
                    return false;
                }

                return (listView.getCount() - listView.getHeaderViewsCount() >= minSize);
            }
        };
        return waitForCondition(listWaitCondition, MAX_WAIT_MS);
    }

    protected boolean waitForNonEmptyListToLoad(final ListView listView) {
        return waitForListToLoad(listView, 1);
    }

    




    protected final ListView findListViewWithTag(String tag) {
        for (ListView listView : mSolo.getCurrentViews(ListView.class)) {
            final String listTag = (String) listView.getTag();
            if (TextUtils.isEmpty(listTag)) {
                continue;
            }

            if (TextUtils.equals(listTag, tag)) {
                return listView;
            }
        }

        return null;
    }

   







    protected void editBookmark(int bookmarkIndex, int fieldIndex, String addedText, ListView list) {

        
        View child;
        mSolo.clickOnText("Bookmarks");
        child = list.getChildAt(bookmarkIndex);
        mAsserter.ok(child != null, "edit item can be retrieved", child != null ? child.toString() : "null!");
        waitForText("Switch to tab");
        mSolo.clickLongOnView(child);
        waitForText("Share");
        mSolo.clickOnText("Edit");
        waitForText("Edit Bookmark");

        
        mSolo.clearEditText(fieldIndex);

        
        mSolo.clickOnEditText(fieldIndex);
        mActions.sendKeys(addedText);
        mSolo.clickOnText("OK");
        waitForText("Bookmark updated");
    }

    
    protected boolean checkBookmarkEdit(int bookmarkIndex, String addedText, ListView list) {
        
        View child;
        mSolo.clickOnText("Bookmarks");
        child = list.getChildAt(bookmarkIndex);
        mAsserter.ok(child != null, "check item can be retrieved", child != null ? child.toString() : "null!");
        waitForText("Switch to tab");
        mSolo.clickLongOnView(child);
        waitForText("Share");
        mSolo.clickOnText("Edit");
        waitForText("Edit Bookmark");

        
        if (mSolo.searchText(addedText)) {
            clickOnButton("Cancel");
            waitForText("about:home");
            return true;
        } else {
            clickOnButton("Cancel");
            waitForText("about:home");
            return false;
        }
    }

    
    private void waitForAboutHomeTab(final int tabIndex) {
        boolean correctTab = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                ViewPager pager = (ViewPager)mSolo.getView(ViewPager.class, 0);
                return (pager.getCurrentItem() == tabIndex);
            }
        }, MAX_WAIT_MS);
        mAsserter.ok(correctTab, "Checking that the correct tab is displayed", "The " + aboutHomeTabs.get(tabIndex) + " tab is displayed");
    }

    private void clickAboutHomeTab(AboutHomeTabs tab) {
        mSolo.clickOnText(tab.toString().replace("_", " "));
    }

    



    private void swipeAboutHome(int swipeVector) {
        
        int swipeWidth = mDriver.getGeckoWidth() - 1;
        int swipeHeight = mDriver.getGeckoHeight() / 2;

        if (swipeVector >= 0) {
            
            for (int i = 0; i < swipeVector; i++) {
                mActions.drag(swipeWidth, 0, swipeHeight, swipeHeight);
                mSolo.sleep(100);
            }
        } else {
            
            for (int i = 0; i > swipeVector; i--) {
                mActions.drag(0, swipeWidth, swipeHeight, swipeHeight);
                mSolo.sleep(100);
            }
        }
    }

    




    protected void openAboutHomeTab(AboutHomeTabs tab) {
        focusUrlBar();
        ViewPager pager = (ViewPager)mSolo.getView(ViewPager.class, 0);
        final int currentTabIndex = pager.getCurrentItem();
        int tabOffset;

        
        if (mDevice.type.equals("tablet")) {
            if (AboutHomeTabs.MOST_RECENT == tab || AboutHomeTabs.TABS_FROM_LAST_TIME == tab) {
                tabOffset = aboutHomeTabs.indexOf(AboutHomeTabs.HISTORY.toString()) - currentTabIndex;
                swipeAboutHome(tabOffset);
                waitForAboutHomeTab(aboutHomeTabs.indexOf(StringHelper.HISTORY_LABEL));
                TabWidget tabwidget = (TabWidget)mSolo.getView(TabWidget.class, 0);

                switch (tab) {
                    case MOST_RECENT: {
                        mSolo.clickOnView(tabwidget.getChildAt(0));
                        
                        mAsserter.ok(waitForText(StringHelper.TODAY_LABEL), "Checking that we are in the most recent tab of about:home", "We are in the most recent tab");
                        break;
                    }
                    case TABS_FROM_LAST_TIME: {
                        mSolo.clickOnView(tabwidget.getChildAt(1));
                        mAsserter.ok(waitForText(StringHelper.TABS_FROM_LAST_TIME_LABEL), "Checking that we are in the Tabs from last time tab of about:home", "We are in the Tabs from last time tab");
                        break;
                    }
                }
            } else {
                clickAboutHomeTab(tab);
            }
            return;
        }

        
        tabOffset = aboutHomeTabs.indexOf(tab.toString()) - currentTabIndex;
        switch (tab) {
            case TOP_SITES : {
                swipeAboutHome(tabOffset);
                waitForAboutHomeTab(aboutHomeTabs.indexOf(tab.toString()));
                break;
            }
            case BOOKMARKS : {
                swipeAboutHome(tabOffset);
                waitForAboutHomeTab(aboutHomeTabs.indexOf(tab.toString()));
                break;
            }
            case MOST_RECENT: {
                
                tabOffset = aboutHomeTabs.indexOf(AboutHomeTabs.HISTORY.toString()) - currentTabIndex;
                swipeAboutHome(tabOffset);
                waitForAboutHomeTab(aboutHomeTabs.indexOf(StringHelper.HISTORY_LABEL));
                TabWidget tabwidget = (TabWidget)mSolo.getView(TabWidget.class, 0);
                mSolo.clickOnView(tabwidget.getChildAt(0));
                
                mAsserter.ok(waitForText(StringHelper.TODAY_LABEL), "Checking that we are in the most recent tab of about:home", "We are in the most recent tab");
                break;
            }
            case TABS_FROM_LAST_TIME: {
                
                tabOffset = aboutHomeTabs.indexOf(AboutHomeTabs.HISTORY.toString()) - currentTabIndex;
                swipeAboutHome(tabOffset);
                waitForAboutHomeTab(aboutHomeTabs.indexOf(StringHelper.HISTORY_LABEL));
                TabWidget tabwidget = (TabWidget)mSolo.getView(TabWidget.class, 0);
                mSolo.clickOnView(tabwidget.getChildAt(1));
                mAsserter.ok(waitForText(StringHelper.TABS_FROM_LAST_TIME_LABEL), "Checking that we are in the Tabs from last time tab of about:home", "We are in the Tabs from last time tab");
                break;
            }
            case READING_LIST: {
                swipeAboutHome(tabOffset);
                waitForAboutHomeTab(aboutHomeTabs.indexOf(tab.toString()));
                break;
            }

        }
    }
}
