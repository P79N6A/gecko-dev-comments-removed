

















































const PREF_ACTIVE = "security.mixed_content.block_active_content";



const gHttpTestRoot1 = "https://test1.example.com/browser/browser/base/content/test/general/";
const gHttpTestRoot2 = "https://test2.example.com/browser/browser/base/content/test/general/";

let origBlockActive;
let gTestWin = null;
let mainTab = null;
let curClickHandler = null;
let curContextMenu = null;
let curTestFunc = null;
let curTestName = null;
let curChildTabLink = null;



registerCleanupFunction(function() {
  
  Services.prefs.setBoolPref(PREF_ACTIVE, origBlockActive);
});





function waitForCondition(condition, nextTest, errorMsg) {
  var tries = 0;
  var interval = setInterval(function() {
    if (tries >= 30) {
      ok(false, errorMsg);
      moveOn();
    }
    if (condition()) {
      moveOn();
    }
    tries++;
  }, 100);
  var moveOn = function() {
    clearInterval(interval); nextTest();
  };
}




let clickHandler = function (aEvent, aFunc) {
  gTestWin.gBrowser.removeEventListener("click", curClickHandler, true);
  gTestWin.contentAreaClick(aEvent, true);
  gTestWin.gBrowser.addEventListener("load", aFunc, true);
  aEvent.preventDefault();
  aEvent.stopPropagation();
}




let contextMenuOpenHandler = function(aEvent, aFunc) {
  gTestWin.document.removeEventListener("popupshown", curContextMenu, false);
  gTestWin.gBrowser.addEventListener("load", aFunc, true);
  var openLinkInTabCommand = gTestWin.document.getElementById("context-openlinkintab");
  openLinkInTabCommand.doCommand();
  aEvent.target.hidePopup();
};

function setUpTest(aTestName, aIDForNextTest, aFuncForNextTest, aChildTabLink) {
  curTestName = aTestName;
  curTestFunc = aFuncForNextTest;
  curChildTabLink = aChildTabLink;

  mainTab = gTestWin.gBrowser.selectedTab;
  
  let target = gTestWin.content.document.getElementById(aIDForNextTest);
  gTestWin.gBrowser.addTab(target);
  gTestWin.gBrowser.selectTabAtIndex(1);
  gTestWin.gBrowser.addEventListener("load", checkPopUpNotification, true);
}

function checkPopUpNotification() {
  gTestWin.gBrowser.removeEventListener("load", checkPopUpNotification, true);
  gTestWin.gBrowser.addEventListener("load", reloadedTabAfterDisablingMCB, true);

  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in " + curTestName + "!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in " + curTestName + "!");

  
  gTestWin.PopupNotifications.panel.firstChild.disableMixedContentProtection();
  notification.remove();
}

function reloadedTabAfterDisablingMCB() {
  gTestWin.gBrowser.removeEventListener("load", reloadedTabAfterDisablingMCB, true);

  var expected = "Mixed Content Blocker disabled";
  waitForCondition(
    function() gTestWin.content.document.getElementById('mctestdiv').innerHTML == expected,
    makeSureMCBisDisabled, "Error: Waited too long for mixed script to run in " + curTestName + "!");
}

function makeSureMCBisDisabled() {
  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Made sure MCB is disabled in " + curTestName + "!");

  
  let doc = gTestWin.content.document;
  let mainDiv = gTestWin.content.document.createElement("div");
  mainDiv.innerHTML =
    '<p><a id="' + curTestName + '" href="' + curChildTabLink + '">' +
    curTestName + '</a></p>';
  doc.body.appendChild(mainDiv);

  curTestFunc();
}



function test1() {
  curClickHandler = function (e) { clickHandler(e, test1A) };
  gTestWin.gBrowser.addEventListener("click", curClickHandler , true);

  
  let targetElt = gTestWin.content.document.getElementById("Test1");
  EventUtils.synthesizeMouseAtCenter(targetElt, { button: 1 }, gTestWin.content);
}

function test1A() {
  gTestWin.gBrowser.removeEventListener("load", test1A, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 1A!");
  notification.reshow();
  ok(!gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test 1A!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 1A");

  gTestWin.gBrowser.removeCurrentTab();
  test1B();
}

function test1B() {
  curContextMenu = function (e) { contextMenuOpenHandler(e, test1C) };
  gTestWin.document.addEventListener("popupshown", curContextMenu, false);

  
  let targetElt = gTestWin.content.document.getElementById("Test1");
  
  EventUtils.synthesizeMouseAtCenter(targetElt, { type : "contextmenu", button : 2 } , gTestWin.content);
}

function test1C() {
  gTestWin.gBrowser.removeEventListener("load", test1C, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 1C!");
  notification.reshow();
  ok(!gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test 1C!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 1C");

  
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[2], {animate: false});
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[1], {animate: false});
  gTestWin.gBrowser.selectTabAtIndex(0);

  var childTabLink = gHttpTestRoot2 + "file_bug906190_2.html";
  setUpTest("Test2", "linkForTest2", test2, childTabLink);
}



function test2() {
  curClickHandler = function (e) { clickHandler(e, test2A) };
  gTestWin.gBrowser.addEventListener("click", curClickHandler, true);

  
  let targetElt = gTestWin.content.document.getElementById("Test2");
  EventUtils.synthesizeMouseAtCenter(targetElt, { button: 1 }, gTestWin.content);
}

function test2A() {
  gTestWin.gBrowser.removeEventListener("load", test2A, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 2A!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 2A!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker enabled", "OK: Blocked mixed script in Test 2A");

  gTestWin.gBrowser.removeCurrentTab();
  test2B();
}

function test2B() {
  curContextMenu = function (e) { contextMenuOpenHandler(e, test2C) };
  gTestWin.document.addEventListener("popupshown", curContextMenu, false);

  
  let targetElt = gTestWin.content.document.getElementById("Test2");
  
  EventUtils.synthesizeMouseAtCenter(targetElt, { type : "contextmenu", button : 2 } , gTestWin.content);
}

function test2C() {
  gTestWin.gBrowser.removeEventListener("load", test2C, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 2C!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 2C!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker enabled", "OK: Blocked mixed script in Test 2C");

  
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[2], {animate: false});
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[1], {animate: false});
  gTestWin.gBrowser.selectTabAtIndex(0);

  
  var childTabLink = gHttpTestRoot1 + "file_bug906190_3_4.html";
  setUpTest("Test3", "linkForTest3", test3, childTabLink);
}



function test3() {
  curClickHandler = function (e) { clickHandler(e, test3A) };
  gTestWin.gBrowser.addEventListener("click", curClickHandler, true);
  
  let targetElt = gTestWin.content.document.getElementById("Test3");
  EventUtils.synthesizeMouseAtCenter(targetElt, { button: 1 }, gTestWin.content);
}

function test3A() {
  
  gTestWin.gBrowser.removeEventListener("load", test3A, true);
  gTestWin.gBrowser.addEventListener("load", test3B, true);
}

function test3B() {
  gTestWin.gBrowser.removeEventListener("load", test3B, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 3B!");
  notification.reshow();
  ok(!gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test 3B!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 3B");

  
  gTestWin.gBrowser.removeCurrentTab();
  test3C();
}

function test3C() {
  curContextMenu = function (e) { contextMenuOpenHandler(e, test3D) };
  gTestWin.document.addEventListener("popupshown", curContextMenu, false);

  
  let targetElt = gTestWin.content.document.getElementById("Test3");
  EventUtils.synthesizeMouseAtCenter(targetElt, { type : "contextmenu", button : 2 } , gTestWin.content);
}

function test3D() {
  
  gTestWin.gBrowser.removeEventListener("load", test3D, true);
  gTestWin.gBrowser.addEventListener("load", test3E, true);
}

function test3E() {
  gTestWin.gBrowser.removeEventListener("load", test3E, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 3E!");
  notification.reshow();
  ok(!gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test 3E!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 3E");

  
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[2], {animate: false});
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[1], {animate: false});
  gTestWin.gBrowser.selectTabAtIndex(0);

  var childTabLink = gHttpTestRoot1 + "file_bug906190_3_4.html";
  setUpTest("Test4", "linkForTest4", test4, childTabLink);
}



function test4() {
  curClickHandler = function (e) { clickHandler(e, test4A) };
  gTestWin.gBrowser.addEventListener("click", curClickHandler, true);

  
  let targetElt = gTestWin.content.document.getElementById("Test4");
  EventUtils.synthesizeMouseAtCenter(targetElt, { button: 1 }, gTestWin.content);
}

function test4A() {
  
  gTestWin.gBrowser.removeEventListener("load", test4A, true);
  gTestWin.gBrowser.addEventListener("load", test4B, true);
}

function test4B() {
  gTestWin.gBrowser.removeEventListener("load", test4B, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 4B!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 4B!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker enabled", "OK: Blocked mixed script in Test 4B");

  
  gTestWin.gBrowser.removeCurrentTab();
  test4C();
}

function test4C() {
  curContextMenu = function (e) { contextMenuOpenHandler(e, test4D) };
  gTestWin.document.addEventListener("popupshown", curContextMenu, false);

  
  let targetElt = gTestWin.content.document.getElementById("Test4");
  EventUtils.synthesizeMouseAtCenter(targetElt, { type : "contextmenu", button : 2 } , gTestWin.content);
}

function test4D() {
  
  gTestWin.gBrowser.removeEventListener("load", test4D, true);
  gTestWin.gBrowser.addEventListener("load", test4E, true);
}

function test4E() {
  gTestWin.gBrowser.removeEventListener("load", test4E, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 4E!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 4E!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker enabled", "OK: Blocked mixed script in Test 4E");

  
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[2], {animate: false});
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[1], {animate: false});
  gTestWin.gBrowser.selectTabAtIndex(0);

  
  var childTabLink = gHttpTestRoot1 + "file_bug906190.sjs";
  setUpTest("Test5", "linkForTest5", test5, childTabLink);
}



function test5() {
  curClickHandler = function (e) { clickHandler(e, test5A) };
  gTestWin.gBrowser.addEventListener("click", curClickHandler, true);

  
  let targetElt = gTestWin.content.document.getElementById("Test5");
  EventUtils.synthesizeMouseAtCenter(targetElt, { button: 1 }, gTestWin.content);
}

function test5A() {
  gTestWin.gBrowser.removeEventListener("load", test5A, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 5A!");
  notification.reshow();
  todo(!gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test 5A!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  todo_is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 5A!");

  
  gTestWin.gBrowser.removeCurrentTab();
  test5B();
}

function test5B() {
  curContextMenu = function (e) { contextMenuOpenHandler(e, test5C) };
  gTestWin.document.addEventListener("popupshown", curContextMenu, false);

  
  let targetElt = gTestWin.content.document.getElementById("Test5");
  EventUtils.synthesizeMouseAtCenter(targetElt, { type : "contextmenu", button : 2 } , gTestWin.content);
}

function test5C() {
  gTestWin.gBrowser.removeEventListener("load", test5C, true);
  
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 5C!");
  notification.reshow();
  todo(!gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test 5C!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  todo_is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 5C!");

  
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[2], {animate: false});
  gTestWin.gBrowser.removeTab(gTestWin.gBrowser.tabs[1], {animate: false});
  gTestWin.gBrowser.selectTabAtIndex(0);

  
  var childTabLink = gHttpTestRoot2 + "file_bug906190.sjs";
  setUpTest("Test6", "linkForTest6", test6, childTabLink);
}



function test6() {
  curClickHandler = function (e) { clickHandler(e, test6A) };
  gTestWin.gBrowser.addEventListener("click", curClickHandler, true);

  
  let targetElt = gTestWin.content.document.getElementById("Test6");
  EventUtils.synthesizeMouseAtCenter(targetElt, { button: 1 }, gTestWin.content);
}

function test6A() {
  gTestWin.gBrowser.removeEventListener("load", test6A, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 6A!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 6A!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker enabled", "OK: Blocked mixed script in Test 6A");

  
  gTestWin.gBrowser.removeCurrentTab();
  test6B();
}

function test6B() {
  curContextMenu = function (e) { contextMenuOpenHandler(e, test6C) };
  gTestWin.document.addEventListener("popupshown", curContextMenu, false);

  
  let targetElt = gTestWin.content.document.getElementById("Test6");
  EventUtils.synthesizeMouseAtCenter(targetElt, { type : "contextmenu", button : 2 } , gTestWin.content);
}

function test6C() {
  gTestWin.gBrowser.removeEventListener("load", test6C, true);
  gTestWin.gBrowser.selectTabAtIndex(2);

  
  var notification = PopupNotifications.getNotification("bad-content", gTestWin.gBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 6C!");
  notification.reshow();
  ok(gTestWin.PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 6C!");
  notification.remove();

  var actual = gTestWin.content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker enabled", "OK: Blocked mixed script in Test 6C");

  gTestWin.close();
  finish();
}



function setupTestBrowserWindow() {
  
  let doc = gTestWin.content.document;
  let mainDiv = doc.createElement("div");
  mainDiv.innerHTML =
    '<p><a id="linkForTest1" href="'+ gHttpTestRoot1 + 'file_bug906190_1.html">Test 1</a></p>' +
    '<p><a id="linkForTest2" href="'+ gHttpTestRoot1 + 'file_bug906190_2.html">Test 2</a></p>' +
    '<p><a id="linkForTest3" href="'+ gHttpTestRoot1 + 'file_bug906190_1.html">Test 3</a></p>' +
    '<p><a id="linkForTest4" href="'+ gHttpTestRoot2 + 'file_bug906190_1.html">Test 4</a></p>' +
    '<p><a id="linkForTest5" href="'+ gHttpTestRoot1 + 'file_bug906190_1.html">Test 5</a></p>' +
    '<p><a id="linkForTest6" href="'+ gHttpTestRoot1 + 'file_bug906190_1.html">Test 6</a></p>';
  doc.body.appendChild(mainDiv);
}

function startTests() {
  mainTab = gTestWin.gBrowser.selectedTab;
  var childTabLink = gHttpTestRoot1 + "file_bug906190_2.html";
  setUpTest("Test1", "linkForTest1", test1, childTabLink);
}

function test() {
  
  waitForExplicitFinish();

  
  origBlockActive = Services.prefs.getBoolPref(PREF_ACTIVE);
  Services.prefs.setBoolPref(PREF_ACTIVE, true);

  gTestWin = openDialog(location, "", "chrome,all,dialog=no", "about:blank");
  whenDelayedStartupFinished(gTestWin, function () {
    info("browser window opened");
    waitForFocus(function() {
      info("browser window focused");
      waitForFocus(function() {
        info("setting up browser...");
        setupTestBrowserWindow();
        info("running tests...");
        executeSoon(startTests);
      }, gTestWin.content, true);
    }, gTestWin);
  });
}
