




var menu;

function test() {
  waitForExplicitFinish();
  
  gBrowser.selectedTab = gBrowser.addTab();

  
  gBrowser.addEventListener("load", testNormal, false);
  content.location = "http://example.com/";
}

function testNormal() {
  gBrowser.removeEventListener("load", testNormal, false);
  
  
  menu = document.getElementById("menu_HelpPopup");
  ok(menu, "Help menu should exist!");
  
  menu.addEventListener("popupshown", testNormal_PopupListener, false);
  menu.openPopup(null, "", 0, 0, false, null);
}

function testNormal_PopupListener() {
  menu.removeEventListener("popupshown", testNormal_PopupListener, false);
  
  var reportMenu = document.getElementById("menu_HelpPopup_reportPhishingtoolmenu");
  var errorMenu = document.getElementById("menu_HelpPopup_reportPhishingErrortoolmenu");
  is(reportMenu.hidden, false, "Report phishing menu should be visible on normal sites");
  is(errorMenu.hidden, true, "Report error menu item should be hidden on normal sites");
  menu.hidePopup();
  
  
  
  content.location = "http://www.mozilla.com/firefox/its-a-trap.html";
  window.setTimeout(testPhishing, 2000);
}

function testPhishing() {
  menu.addEventListener("popupshown", testPhishing_PopupListener, false);
  menu.openPopup(null, "", 0, 0, false, null);
}

function testPhishing_PopupListener() {
  menu.removeEventListener("popupshown", testPhishing_PopupListener, false);
  
  var reportMenu = document.getElementById("menu_HelpPopup_reportPhishingtoolmenu");
  var errorMenu = document.getElementById("menu_HelpPopup_reportPhishingErrortoolmenu");
  is(reportMenu.hidden, true, "Report phishing menu should be hidden on phishing sites");
  is(errorMenu.hidden, false, "Report error menu item should be visible on phishing sites");
  menu.hidePopup();
  
  gBrowser.removeCurrentTab();
  finish();
}
