



package org.mozilla.gecko.tests;

import java.util.ArrayList;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.home.HomePager;

import android.support.v4.view.ViewPager;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TabWidget;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Condition;







abstract class AboutHomeTest extends PixelTest {
    protected enum AboutHomeTabs {
        RECENT_TABS,
        HISTORY,
        TOP_SITES,
        BOOKMARKS,
        READING_LIST
    };

    private ArrayList<String> aboutHomeTabs = new ArrayList<String>() {{
                  add("TOP_SITES");
                  add("BOOKMARKS");
                  add("READING_LIST");
              }};


    @Override
    public void setUp() throws Exception {
        super.setUp();

        if (aboutHomeTabs.size() < 4) {
            
            if (mDevice.type.equals("phone")) {
                aboutHomeTabs.add(0, AboutHomeTabs.HISTORY.toString());
                aboutHomeTabs.add(0, AboutHomeTabs.RECENT_TABS.toString());
            } else {
                aboutHomeTabs.add(AboutHomeTabs.HISTORY.toString());
                aboutHomeTabs.add(AboutHomeTabs.RECENT_TABS.toString());
            }
        }
    }

    


    protected ListView getHistoryList(String waitText, int expectedChildCount) {
        return null;
    }
    protected ListView getHistoryList(String waitText) {
        return null;
    }

    
    protected void isBookmarkDisplayed(final String url) {
        boolean isCorrect = waitForTest(new BooleanTest() {
            @Override
            public boolean test() {
                View bookmark = getDisplayedBookmark(url);
                return bookmark != null;
            }
        }, MAX_WAIT_MS);

        mAsserter.ok(isCorrect, "Checking that " + url + " displayed as a bookmark", url + " displayed");
    }

    
    protected void loadBookmark(String url) {
        View bookmark = getDisplayedBookmark(url);
        if (bookmark != null) {
            Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
            mSolo.clickOnView(bookmark);
            contentEventExpecter.blockForEvent();
            contentEventExpecter.unregisterListener();
        } else {
            mAsserter.ok(false, url + " is not one of the displayed bookmarks", "Please make sure the url provided is bookmarked");
        }
    }

    
    protected void openBookmarkContextMenu(String url) {
        View bookmark = getDisplayedBookmark(url);
        if (bookmark != null) {
            mSolo.waitForView(bookmark);
            mSolo.clickLongOnView(bookmark, LONG_PRESS_TIME);
            mSolo.waitForDialogToOpen();
        } else {
            mAsserter.ok(false, url + " is not one of the displayed bookmarks", "Please make sure the url provided is bookmarked");
        }
    }

    
    protected View getDisplayedBookmark(String url) {
        openAboutHomeTab(AboutHomeTabs.BOOKMARKS);
        mSolo.hideSoftKeyboard();
        getInstrumentation().waitForIdleSync();
        ListView bookmarksTabList = findListViewWithTag(HomePager.LIST_TAG_BOOKMARKS);
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

    
    private void waitForAboutHomeTab(final int tabIndex) {
        boolean correctTab = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                ViewPager pager = mSolo.getView(ViewPager.class, 0);
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
        ViewPager pager = mSolo.getView(ViewPager.class, 0);
        final int currentTabIndex = pager.getCurrentItem();
        int tabOffset;

        
        if (mDevice.type.equals("tablet")) {
            clickAboutHomeTab(tab);
            return;
        }

        
        tabOffset = aboutHomeTabs.indexOf(tab.toString()) - currentTabIndex;
        swipeAboutHome(tabOffset);
        waitForAboutHomeTab(aboutHomeTabs.indexOf(tab.toString()));
    }
}
