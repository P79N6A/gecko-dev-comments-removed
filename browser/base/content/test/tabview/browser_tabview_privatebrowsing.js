





































let contentWindow = null;
let normalURLs = []; 
let pbTabURL = "about:privatebrowsing";
let groupTitles = [];
let normalIteration = 0;

let pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);


function test() {
  waitForExplicitFinish();

  
  window.addEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  TabView.toggle();
}


function onTabViewLoadedAndShown() {
  window.removeEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  ok(TabView.isVisible(), "Tab View is visible");

  
  contentWindow = document.getElementById("tab-view").contentWindow;  
  verifyCleanState("start");
  
  
  registerCleanupFunction(function() {
    pb.privateBrowsingEnabled = false;
  });
  
  
  let box = new contentWindow.Rect(20, 20, 180, 180);
  let groupItem = new contentWindow.GroupItem([], {bounds: box, title: "test1"});
  let id = groupItem.id; 
  is(contentWindow.GroupItems.groupItems.length, 2, "we now have two groups");
  registerCleanupFunction(function() {
    contentWindow.GroupItems.groupItem(id).close();
  });
  
  
  contentWindow.GroupItems.setActiveGroupItem(groupItem);
  
  
  let count = contentWindow.GroupItems.groupItems.length;
  for (let a = 0; a < count; a++) {
    let gi = contentWindow.GroupItems.groupItems[a];
    groupTitles[a] = gi.getTitle();
  }
  
  
  gBrowser.addTab("about:robots");
  is(gBrowser.tabs.length, 2, "we now have 2 tabs");
  registerCleanupFunction(function() {
    gBrowser.removeTab(gBrowser.tabs[1]);
  });

  afterAllTabsLoaded(function() {
    
    for (let a = 0; a < gBrowser.tabs.length; a++)
      normalURLs.push(gBrowser.tabs[a].linkedBrowser.currentURI.spec);

    
    verifyNormal();

    
    togglePBAndThen(function() {
      ok(!TabView.isVisible(), "Tab View is no longer visible");
      verifyPB();
      
      
      togglePBAndThen(function() {
        ok(TabView.isVisible(), "Tab View is visible again");
        verifyNormal();
        
        
        window.addEventListener("tabviewhidden", onTabViewHidden, false);
        TabView.toggle();
      });  
    });
  });
}


function onTabViewHidden() {
  window.removeEventListener("tabviewhidden", onTabViewHidden, false);
  ok(!TabView.isVisible(), "Tab View is not visible");
  
  
  togglePBAndThen(function() {
    ok(!TabView.isVisible(), "Tab View is still not visible");
    verifyPB();
    
    
    togglePBAndThen(function() {
      verifyNormal();
      
      
      ok(!TabView.isVisible(), "we finish with Tab View not visible");
      registerCleanupFunction(verifyCleanState); 
      finish();
    });
  });
}


function verifyCleanState(mode) {
  let prefix = "we " + (mode || "finish") + " with ";
  is(gBrowser.tabs.length, 1, prefix + "one tab");
  is(contentWindow.GroupItems.groupItems.length, 1, prefix + "1 group");
  ok(gBrowser.tabs[0].tabItem.parent == contentWindow.GroupItems.groupItems[0], 
      "the tab is in the group");
  ok(!pb.privateBrowsingEnabled, prefix + "private browsing off");
}


function verifyPB() {
  ok(pb.privateBrowsingEnabled == true, "private browsing is on");
  is(gBrowser.tabs.length, 1, "we have 1 tab in private browsing");
  is(contentWindow.GroupItems.groupItems.length, 1, "we have 1 group in private browsing");
  ok(gBrowser.tabs[0].tabItem.parent == contentWindow.GroupItems.groupItems[0], 
      "the tab is in the group");

  let browser = gBrowser.tabs[0].linkedBrowser;
  is(browser.currentURI.spec, pbTabURL, "correct URL for private browsing");
}


function verifyNormal() {
  let prefix = "verify normal " + normalIteration + ": "; 
  normalIteration++;
  
  ok(pb.privateBrowsingEnabled == false, prefix + "private browsing is off");
  
  let tabCount = gBrowser.tabs.length;
  let groupCount = contentWindow.GroupItems.groupItems.length;
  is(tabCount, 2, prefix + "we have 2 tabs");
  is(groupCount, 2, prefix + "we have 2 groups");
  ok(tabCount == groupCount, prefix + "same number of tabs as groups"); 
  for (let a = 0; a < tabCount; a++) {
    let tab = gBrowser.tabs[a];
    is(tab.linkedBrowser.currentURI.spec, normalURLs[a],
        prefix + "correct URL");

    let groupItem = contentWindow.GroupItems.groupItems[a];
    is(groupItem.getTitle(), groupTitles[a], prefix + "correct group title");
    
    ok(tab.tabItem.parent == groupItem,
        prefix + "tab " + a + " is in group " + a);
  }
}


function togglePBAndThen(callback) {
  function pbObserver(aSubject, aTopic, aData) {
    if (aTopic != "private-browsing-transition-complete")
      return;

    Services.obs.removeObserver(pbObserver, "private-browsing-transition-complete");
    
    afterAllTabsLoaded(callback);
  }

  Services.obs.addObserver(pbObserver, "private-browsing-transition-complete", false);
  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}


function afterAllTabsLoaded(callback) {
  let stillToLoad = 0; 
  function onLoad() {
    this.removeEventListener("load", onLoad, true);
    
    stillToLoad--;
    if (!stillToLoad)
      callback();
  }

  for (let a = 0; a < gBrowser.tabs.length; a++) {
    let browser = gBrowser.tabs[a].linkedBrowser;
    if (browser.webProgress.isLoadingDocument) {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }
}
