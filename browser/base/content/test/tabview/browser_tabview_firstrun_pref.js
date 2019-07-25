




































var prefsBranch = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefService).
                  getBranch("browser.panorama.");

function test() {
  waitForExplicitFinish();

  ok(!TabView.isVisible(), "Main window TabView is hidden");

  prefsBranch.setBoolPref("experienced_first_run", false);
  ok(!experienced(), "not experienced");

  newWindowWithTabView(checkFirstRun, part2);
}

function experienced() {
  return prefsBranch.prefHasUserValue("experienced_first_run") &&
    prefsBranch.getBoolPref("experienced_first_run");
}

function checkFirstRun(win) {
  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  
  let infoItems = contentWindow.iQ(".info-item");
  is(infoItems.length, 1, "There should be an info item");

  let groupItems = contentWindow.GroupItems.groupItems;
  is(groupItems.length, 1, "There should be one group");
  
  ok(experienced(), "we're now experienced");
}

function part2() {
  newWindowWithTabView(checkNotFirstRun, endGame);
}

function checkNotFirstRun(win) {
  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  
  let infoItems = contentWindow.iQ(".info-item");
  is(infoItems.length, 0, "There should be no info items");
}

function endGame() {
  ok(!TabView.isVisible(), "Main window TabView is still hidden");
  finish();
}

function newWindowWithTabView(callback, completeCallback) {
  let charsetArg = "charset=" + window.content.document.characterSet;
  let win = window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no,height=800,width=800",
                              "about:blank", charsetArg, null, null, true);
  let onLoad = function() {
    win.removeEventListener("load", onLoad, false);
    let onShown = function() {
      win.removeEventListener("tabviewshown", onShown, false);
      callback(win);
      win.close();
      if (typeof completeCallback == "function")
        completeCallback();
    };
    win.addEventListener("tabviewshown", onShown, false);
    win.TabView.toggle();
  }
  win.addEventListener("load", onLoad, false);
}
