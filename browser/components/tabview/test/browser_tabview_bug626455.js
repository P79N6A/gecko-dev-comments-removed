








"use strict";

const TEST_URL = 'data:text/html,<script>window.onbeforeunload=' +
                 'function(e){e.returnValue="?"}</script>';

let contentWindow;
let activeGroup;

function test() {
  waitForExplicitFinish();

  showTabView(function () {
    contentWindow = TabView.getContentWindow();
    activeGroup = contentWindow.GroupItems.getActiveGroupItem();

    gBrowser.browsers[0].loadURI("data:text/html,<p>test for bug 626455, tab1");

    let tab = gBrowser.addTab(TEST_URL);
    afterAllTabsLoaded(() => testStayOnPage(tab));
  });
}

function testStayOnPage(blockingTab) {
  let browser = blockingTab.linkedBrowser;
  waitForOnBeforeUnloadDialog(browser, function (btnLeave, btnStay) {
    
    btnStay.click();

    executeSoon(function () {
      showTabView(function () {
        is(gBrowser.tabs.length, 1,
           "The total number of tab is 1 when staying on the page");

        
        
        let url = gBrowser.browsers[0].currentURI.spec;
        ok(url.contains("onbeforeunload"), "The open tab is the expected one");

        is(contentWindow.GroupItems.getActiveGroupItem(), activeGroup,
           "Active group is still the same");

        is(contentWindow.GroupItems.groupItems.length, 1,
           "Only one group is open");

        
        testLeavePage(gBrowser.tabs[0]);
      });
    });
  });

  closeGroupItem(activeGroup);
}

function testLeavePage(blockingTab) {
  let browser = blockingTab.linkedBrowser;
  waitForOnBeforeUnloadDialog(browser, function (btnLeave, btnStay) {
    
    btnLeave.click();
  });

  whenGroupClosed(activeGroup, finishTest);
  closeGroupItem(activeGroup);
}

function finishTest() {
  is(gBrowser.tabs.length, 1,
     "The total number of tab is 1 after leaving the page");
  is(contentWindow.TabItems.getItems().length, 1,
     "The total number of tab items is 1 after leaving the page");

  let location = gBrowser.browsers[0].currentURI.spec;
  is(location, BROWSER_NEW_TAB_URL, "The open tab is the expected one");

  isnot(contentWindow.GroupItems.getActiveGroupItem(), activeGroup,
     "Active group is no longer the same");

  is(contentWindow.GroupItems.groupItems.length, 1,
     "Only one group is open");

  contentWindow = null;
  activeGroup = null;
  finish();
}


function whenGroupClosed(group, callback) {
  group.addSubscriber("close", function onClose() {
    group.removeSubscriber("close", onClose);
    callback();
  });
}
