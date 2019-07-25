


let handleDialog;
let timer; 

function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let groupItemOne = contentWindow.GroupItems.getActiveGroupItem();

  
  let box = new contentWindow.Rect(10, 10, 300, 300);
  let groupItemTwo = new contentWindow.GroupItem([], { bounds: box });
  contentWindow.UI.setActive(groupItemTwo);

  let testTab = 
    gBrowser.addTab(
      "http://mochi.test:8888/browser/browser/base/content/test/tabview/test_bug599626.html");
  let browser = gBrowser.getBrowserForTab(testTab);
  let onLoad = function() {
    browser.removeEventListener("load", onLoad, true);

    testStayOnPage(contentWindow, groupItemOne, groupItemTwo);
  }
  browser.addEventListener("load", onLoad, true);
}

function testStayOnPage(contentWindow, groupItemOne, groupItemTwo) {
  setupAndRun(contentWindow, groupItemOne, groupItemTwo, function(doc) {
    groupItemTwo.addSubscriber(groupItemTwo, "groupShown", function() {
      groupItemTwo.removeSubscriber(groupItemTwo, "groupShown");

      is(gBrowser.tabs.length, 2, 
         "The total number of tab is 2 when staying on the page");
      is(contentWindow.TabItems.getItems().length, 2, 
         "The total number of tab items is 2 when staying on the page");

      let onTabViewShown = function() {
        window.removeEventListener("tabviewshown", onTabViewShown, false);

        
        testLeavePage(contentWindow, groupItemOne, groupItemTwo);
      };
      window.addEventListener("tabviewshown", onTabViewShown, false);
      TabView.toggle();
    });
    
    doc.documentElement.getButton("cancel").click();
  });
}

function testLeavePage(contentWindow, groupItemOne, groupItemTwo) {
  setupAndRun(contentWindow, groupItemOne, groupItemTwo, function(doc) {
    
    groupItemTwo.addSubscriber(groupItemTwo, "close", function() {
      groupItemTwo.removeSubscriber(groupItemTwo, "close");

      is(gBrowser.tabs.length, 1,
         "The total number of tab is 1 after leaving the page");
      is(contentWindow.TabItems.getItems().length, 1, 
         "The total number of tab items is 1 after leaving the page");

      let endGame = function() {
        window.removeEventListener("tabviewhidden", endGame, false);
        finish();
      };
      window.addEventListener("tabviewhidden", endGame, false);
    });

    
    doc.documentElement.getButton("accept").click();
  });
}

function setupAndRun(contentWindow, groupItemOne, groupItemTwo, callback) {
  let closeButton = groupItemTwo.container.getElementsByClassName("close");
  ok(closeButton[0], "Group close button exists");
  
  EventUtils.sendMouseEvent({ type: "click" }, closeButton[0], contentWindow);

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    handleDialog = function(doc) {
      callback(doc);
    };
    startCallbackTimer();
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);

  let tabItem = groupItemOne.getChild(0);
  tabItem.zoomIn();
}


let observer = {
  QueryInterface : function (iid) {
    const interfaces = [Ci.nsIObserver, Ci.nsISupports, Ci.nsISupportsWeakReference];

    if (!interfaces.some( function(v) { return iid.equals(v) } ))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },

  observe : function (subject, topic, data) {
    let doc = getDialogDoc();
    if (doc)
      handleDialog(doc);
    else
      startCallbackTimer(); 
  }
};

function startCallbackTimer() {
   
   const dialogDelay = 10;

   
   timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
   timer.init(observer, dialogDelay, Ci.nsITimer.TYPE_ONE_SHOT);
}

function getDialogDoc() {
  
  
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
            getService(Ci.nsIWindowMediator);
  let enumerator = wm.getXULWindowEnumerator(null);

   while (enumerator.hasMoreElements()) {
     let win = enumerator.getNext();
     let windowDocShell = win.QueryInterface(Ci.nsIXULWindow).docShell;
 
     let containedDocShells = windowDocShell.getDocShellEnumerator(
                                Ci.nsIDocShellTreeItem.typeChrome,
                                Ci.nsIDocShell.ENUMERATE_FORWARDS);
     while (containedDocShells.hasMoreElements()) {
       
       let childDocShell = containedDocShells.getNext();
       
       if (childDocShell.busyFlags != Ci.nsIDocShell.BUSY_FLAGS_NONE)
         continue;
       let childDoc = childDocShell.QueryInterface(Ci.nsIDocShell).
                      contentViewer.DOMDocument;

       if (childDoc.location.href == "chrome://global/content/commonDialog.xul")
         return childDoc;
     }
   }
 
  return null;
}
