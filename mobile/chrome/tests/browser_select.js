let testURL = "chrome://mochikit/content/browser/mobile/chrome/browser_select.html";
let new_tab = null;



function test() {
  
  waitForExplicitFinish();
  
  
  new_tab = Browser.addTab(testURL, true);
  ok(new_tab, "Tab Opened");	

  
  new_tab.browser.addEventListener("load", onPageLoaded, true);
}

function onPageLoaded() {
  let combo = new_tab.browser.contentDocument.getElementById("combobox");
  isnot(combo, null, "Get the select from web content");

  
  
  
  finish();
  
  
  
}
  
function onUIReady() {    
  let selectui = document.getElementById("select-container");
  is(selectui.hidden, false, "Select UI should be open");
  
  let doneButton = document.getElementById("select-buttons-done");
  doneButton.click();

  
  Browser.closeTab(new_tab);
  
  
  finish();
}
