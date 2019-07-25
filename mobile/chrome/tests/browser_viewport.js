let testURL_blank = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
let testURL_vport_01 = "chrome://mochikit/content/browser/mobile/chrome/browser_viewport_01.html";
let testURL_vport_02 = "chrome://mochikit/content/browser/mobile/chrome/browser_viewport_02.html";

let working_tab;



function test() {
  
  waitForExplicitFinish();

  
  working_tab = Browser.addTab(testURL_blank, true);
  ok(working_tab, "Tab Opened");

  
  waitFor(load_first_blank, function() { return working_tab.isLoading() == false; });
}

function load_first_blank() {
  
  var uri = working_tab.browser.currentURI.spec;
  is(uri, testURL_blank, "URL Matches newly created Tab");

  
  ok(working_tab.browser.classList.contains("browser"), "Normal 'browser' class");
  let style = window.getComputedStyle(working_tab.browser, null);
  is(style.width, "800px", "Normal 'browser' width is 800 pixels");

  
  BrowserUI.goToURI(testURL_vport_01);

  
  waitFor(load_first_viewport, function() { return working_tab.isLoading() == false; });
}

function load_first_viewport() {
  
  var uri = working_tab.browser.currentURI.spec;
  is(uri, testURL_vport_01, "URL Matches newly created Tab");

  
  ok(working_tab.browser.classList.contains("browser-viewport"), "Viewport 'browser-viewport' class");
  let style = window.getComputedStyle(working_tab.browser, null);
  is(style.width, window.innerWidth + "px", "Viewport device-width is equal to window.innerWidth");

  is(Browser._browserView.getZoomLevel(), 1, "Viewport scale=1");

  
  BrowserUI.goToURI(testURL_blank);

  
  waitFor(load_second_blank, function() { return working_tab.isLoading() == false; });
}

function load_second_blank() {
  
  var uri = working_tab.browser.currentURI.spec;
  is(uri, testURL_blank, "URL Matches newly created Tab");

  
  ok(working_tab.browser.classList.contains("browser"), "Normal 'browser' class");
  let style = window.getComputedStyle(working_tab.browser, null);
  is(style.width, "800px", "Normal 'browser' width is 800 pixels");

  
  BrowserUI.goToURI(testURL_vport_02);

  
  waitFor(load_second_viewport, function() { return working_tab.isLoading() == false; });
}

function load_second_viewport() {
  
  var uri = working_tab.browser.currentURI.spec;
  is(uri, testURL_vport_02, "URL Matches newly created Tab");

  
  ok(working_tab.browser.classList.contains("browser-viewport"), "Viewport 'browser-viewport' class");
  let style = window.getComputedStyle(working_tab.browser, null);
  is(style.width, "320px", "Viewport width=320");

  is(Browser._browserView.getZoomLevel(), 1, "Viewport scale=1");

  
  BrowserUI.goToURI(testURL_blank);

  
  waitFor(load_third_blank, function() { return working_tab.isLoading() == false; });
}

function load_third_blank() {
  
  var uri = working_tab.browser.currentURI.spec;
  is(uri, testURL_blank, "URL Matches newly created Tab");

  
  ok(working_tab.browser.classList.contains("browser"), "Normal 'browser' class");
  let style = window.getComputedStyle(working_tab.browser, null);
  is(style.width, "800px", "Normal 'browser' width is 800 pixels");

  
  test_close();
}

function test_close() {
  
  Browser.closeTab(working_tab);

  
  finish();
}
