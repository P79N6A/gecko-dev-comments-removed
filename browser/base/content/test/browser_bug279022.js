




































let testPage1 = 'data:text/html,<p>Mozilla</p>';
let testPage2 = 'data:text/html,<p>Firefox</p>';
let testPage3 = 'data:text/html,<p>Seamonkey</p><input type="text" value="Gecko"></input>';
let hButton = gFindBar.getElement("highlight");

var testTab1, testTab2, testTab3;
var testBrowser1, testBrowser2, testBrowser3;

function test() {
  waitForExplicitFinish();

  testTab1 = gBrowser.addTab();
  testTab2 = gBrowser.addTab();
  testTab3 = gBrowser.addTab();
  testBrowser1 = gBrowser.getBrowserForTab(testTab1);
  testBrowser2 = gBrowser.getBrowserForTab(testTab2);
  testBrowser3 = gBrowser.getBrowserForTab(testTab3);

  
  testBrowser3.loadURI(testPage3);
  testBrowser2.loadURI(testPage2);
  testBrowser1.addEventListener("load", testTabs, true);
  testBrowser1.loadURI(testPage1);
}

function testTabs() {
  testBrowser1.removeEventListener("load", testTabs, true);

  
  gBrowser.selectedTab = testTab1;
  gFindBar.getElement("find-case-sensitive").checked = false;
  gFindBar._highlightDoc(true, "Mozilla");

  
  gBrowser.selectedTab = testTab2;
  ok(!hButton.checked, "highlight button deselected after changing tab");
  gFindBar._findField.value = "";

  
  gBrowser.selectedTab = testTab1;
  ok(hButton.checked, "highlight button re-enabled on tab with highlighting");
  var searchTermOK = gFindBar._findField.value == "Mozilla";
  ok(searchTermOK, "detected correct search term");

  
  gBrowser.selectedTab = testTab3;
  gFindBar._highlightDoc(true, "Gecko");

  
  gBrowser.selectedTab = testTab2;
  ok(!hButton.checked, "highlight button deselected again");
  gFindBar._findField.value = "";

  
  gBrowser.selectedTab = testTab3;
  ok(hButton.checked, "highlighting detected in editable element");
  searchTermOK = gFindBar._findField.value == "Gecko";
  ok(searchTermOK, "detected correct search term");

  
  gBrowser.selectedTab = testTab1;
  searchTermOK = gFindBar._findField.value == "Mozilla";
  ok(searchTermOK, "correctly changed search term");

  gBrowser.removeTab(testTab3);
  gBrowser.removeTab(testTab2);
  gBrowser.removeTab(testTab1);
  finish();
}

