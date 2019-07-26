


"use strict";

const TEST_URL = 'data:text/html,<script>window.onbeforeunload=' +
                 'function(e){e.returnValue="?"}</script>';

function test() {
  waitForExplicitFinish();
  showTabView(onTabViewShown);
}

function onTabViewShown() {
  let contentWindow = TabView.getContentWindow();
  let groupItemOne = contentWindow.GroupItems.getActiveGroupItem();
  let groupItemTwo = createGroupItemWithTabs(window, 300, 300, 10, [TEST_URL]);

  afterAllTabsLoaded(function () {
    testStayOnPage(contentWindow, groupItemOne, groupItemTwo);
  });
}

function testStayOnPage(contentWindow, groupItemOne, groupItemTwo) {
  
  
  let browser = gBrowser.browsers[1];
  waitForOnBeforeUnloadDialog(browser, function (btnLeave, btnStay) {
    executeSoon(function () {
      is(gBrowser.tabs.length, 2, 
         "The total number of tab is 2 when staying on the page");
      is(contentWindow.TabItems.getItems().length, 2, 
         "The total number of tab items is 2 when staying on the page");

      showTabView(function () {
        
        testLeavePage(contentWindow, groupItemOne, groupItemTwo);
      });
    });

    
    btnStay.click();
  });

  closeGroupItem(groupItemTwo);
}

function testLeavePage(contentWindow, groupItemOne, groupItemTwo) {
  
  
  let browser = gBrowser.browsers[1];
  waitForOnBeforeUnloadDialog(browser, function (btnLeave, btnStay) {
    
    groupItemTwo.addSubscriber("close", function onClose() {
      groupItemTwo.removeSubscriber("close", onClose);

      is(gBrowser.tabs.length, 1,
         "The total number of tab is 1 after leaving the page");
      is(contentWindow.TabItems.getItems().length, 1, 
         "The total number of tab items is 1 after leaving the page");

      hideTabView(finish);
    });

    
    btnLeave.click();
  });

  closeGroupItem(groupItemTwo);
}
