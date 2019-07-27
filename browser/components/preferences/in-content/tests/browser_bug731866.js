


Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");

function test() {
  waitForExplicitFinish();
  open_preferences(runTest);
}

let gElements;

function checkElements(expectedPane) {
  for (let element of gElements) {
    
    
    if (element.nodeName == "preferences" || element.id === "drmGroup") {
      continue;
    }
    let attributeValue = element.getAttribute("data-category");
    if (attributeValue == "pane" + expectedPane) {
      is_element_visible(element, expectedPane + " elements should be visible");
    } else {
      is_element_hidden(element, "Elements not in " + expectedPane + " should be hidden");
    }
  }
}

function runTest(win) {
  is(gBrowser.currentURI.spec, "about:preferences", "about:preferences loaded");

  let tab = win.document;
  gElements = tab.getElementById("mainPrefPane").children;

  let panes = [
    "General", "Search", "Content", "Applications",
    "Privacy", "Security", "Sync", "Advanced",
  ];

  for (let pane of panes) {
    win.gotoPref("pane" + pane);
    checkElements(pane);
  }

  gBrowser.removeCurrentTab();
  win.close();
  finish();
}
