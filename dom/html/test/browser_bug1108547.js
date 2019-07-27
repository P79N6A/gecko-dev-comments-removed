


function test() {
  waitForExplicitFinish();

  runPass("file_bug1108547-2.html", function() {
    runPass("file_bug1108547-3.html", function() {
      finish();
    });
  });
}

function runPass(getterFile, finishedCallback) {
  var rootDir = "http://mochi.test:8888/browser/dom/html/test/";
  var testBrowser;
  var privateWin;

  function whenWindowLoaded(win, callback) {
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      executeSoon(callback);
    }, false);
  }

  function whenDelayedStartupFinished(win, callback) {
    let topic = "browser-delayed-startup-finished";
    Services.obs.addObserver(function onStartup(aSubject) {
      if (win != aSubject)
        return;

      Services.obs.removeObserver(onStartup, topic);
      executeSoon(callback);
    }, topic, false);
  }

  
  gBrowser.selectedTab = gBrowser.addTab(rootDir + "file_bug1108547-1.html");
  gBrowser.selectedBrowser.addEventListener("load", afterOpenCookieSetter, true);

  function afterOpenCookieSetter() {
    gBrowser.selectedBrowser.removeEventListener("load", afterOpenCookieSetter, true);
    gBrowser.removeCurrentTab();

    
    privateWin = OpenBrowserWindow({private: true});
    whenWindowLoaded(privateWin, function() {
      whenDelayedStartupFinished(privateWin, afterPrivateWindowOpened);
    });
  }

  function afterPrivateWindowOpened() {
    
    privateWin.gBrowser.selectedTab = privateWin.gBrowser.addTab(rootDir + getterFile);
    testBrowser = privateWin.gBrowser.selectedBrowser;
    privateWin.gBrowser.tabContainer.addEventListener("TabOpen", onNewTabOpened, true);
  }

  function onNewTabOpened() {
    
    privateWin.gBrowser.tabContainer.removeEventListener("TabOpen", onNewTabOpened, true);
    privateWin.gBrowser.tabs[privateWin.gBrowser.tabs.length - 1].linkedBrowser.addEventListener("load", onNewTabLoaded, true);
  }

  function onNewTabLoaded() {
    privateWin.gBrowser.tabs[privateWin.gBrowser.tabs.length - 1].linkedBrowser.removeEventListener("load", onNewTabLoaded, true);

    
    is(testBrowser.contentDocument.getElementById("result").textContent, "",
       "Shouldn't have access to the cookies");

    
    privateWin.close();

    
    Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager).removeAll();

    
    privateWin = OpenBrowserWindow({private: true});
    whenWindowLoaded(privateWin, function() {
      whenDelayedStartupFinished(privateWin, afterPrivateWindowOpened2);
    });
  }

  function afterPrivateWindowOpened2() {
    
    privateWin.gBrowser.selectedTab = privateWin.gBrowser.addTab(rootDir + "file_bug1108547-1.html");
    privateWin.gBrowser.selectedBrowser.addEventListener("load", afterOpenCookieSetter2, true);
  }

  function afterOpenCookieSetter2() {
    
    privateWin.close();

    
    gBrowser.selectedTab = gBrowser.addTab(rootDir + getterFile);
    testBrowser = gBrowser.selectedBrowser;
    gBrowser.tabContainer.addEventListener("TabOpen", onNewTabOpened2, true);
  }

  function onNewTabOpened2() {
    
    gBrowser.tabContainer.removeEventListener("TabOpen", onNewTabOpened2, true);
    gBrowser.tabs[gBrowser.tabs.length - 1].linkedBrowser.addEventListener("load", onNewTabLoaded2, true);
  }

  function onNewTabLoaded2() {
    gBrowser.tabs[gBrowser.tabs.length - 1].linkedBrowser.removeEventListener("load", onNewTabLoaded2, true);

    
    is(testBrowser.contentDocument.getElementById("result").textContent, "",
       "Shouldn't have access to the cookies");

    
    gBrowser.removeCurrentTab();
    gBrowser.removeCurrentTab();

    finishedCallback();
  }
}
