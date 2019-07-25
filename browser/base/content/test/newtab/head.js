


const PREF_NEWTAB_ENABLED = "browser.newtabpage.enabled";

Services.prefs.setBoolPref(PREF_NEWTAB_ENABLED, true);

let tmp = {};
Cu.import("resource:///modules/NewTabUtils.jsm", tmp);
let NewTabUtils = tmp.NewTabUtils;

registerCleanupFunction(function () {
  while (gBrowser.tabs.length > 1)
    gBrowser.removeTab(gBrowser.tabs[1]);

  Services.prefs.clearUserPref(PREF_NEWTAB_ENABLED);
});




let originalProvider = NewTabUtils.links._provider;




function test() {
  TestRunner.run();
}




let TestRunner = {
  


  run: function () {
    waitForExplicitFinish();

    this._iter = runTests();
    this.next();
  },

  


  next: function () {
    try {
      TestRunner._iter.next();
    } catch (e if e instanceof StopIteration) {
      TestRunner.finish();
    }
  },

  


  finish: function () {
    function cleanupAndFinish() {
      
      NewTabUtils.links._provider = originalProvider;

      whenPagesUpdated(finish);
      NewTabUtils.restore();
    }

    let callbacks = NewTabUtils.links._populateCallbacks;
    let numCallbacks = callbacks.length;

    if (numCallbacks)
      callbacks.splice(0, numCallbacks, cleanupAndFinish);
    else
      cleanupAndFinish();
  }
};





function getContentWindow() {
  return gBrowser.selectedBrowser.contentWindow;
}





function getContentDocument() {
  return gBrowser.selectedBrowser.contentDocument;
}





function getGrid() {
  return getContentWindow().gGrid;
}






function getCell(aIndex) {
  return getGrid().cells[aIndex];
}










function setLinks(aLinksPattern) {
  let links = aLinksPattern.split(/\s*,\s*/).map(function (id) {
    return {url: "about:blank#" + id, title: "site#" + id};
  });

  NewTabUtils.links._provider = {getLinks: function (c) c(links)};
  NewTabUtils.links._links = links;
}










function setPinnedLinks(aLinksPattern) {
  let pinnedLinks = [];

  aLinksPattern.split(/\s*,\s*/).forEach(function (id, index) {
    let link;

    if (id)
      link = {url: "about:blank#" + id, title: "site#" + id};

    pinnedLinks[index] = link;
  });

  
  NewTabUtils.pinnedLinks._links = pinnedLinks;
}




function restore() {
  whenPagesUpdated();
  NewTabUtils.restore();
}




function addNewTabPageTab() {
  let tab = gBrowser.selectedTab = gBrowser.addTab("about:newtab");
  let browser = tab.linkedBrowser;

  
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);

    if (NewTabUtils.allPages.enabled) {
      
      NewTabUtils.links.populateCache(function () {
        executeSoon(TestRunner.next);
      });
    } else {
      TestRunner.next();
    }
  }, true);
}











function checkGrid(aSitesPattern, aSites) {
  let length = aSitesPattern.split(",").length;
  let sites = (aSites || getGrid().sites).slice(0, length);
  let current = sites.map(function (aSite) {
    if (!aSite)
      return "";

    let pinned = aSite.isPinned();
    let pinButton = aSite.node.querySelector(".newtab-control-pin");
    let hasPinnedAttr = pinButton.hasAttribute("pinned");

    if (pinned != hasPinnedAttr)
      ok(false, "invalid state (site.isPinned() != site[pinned])");

    return aSite.url.replace(/^about:blank#(\d+)$/, "$1") + (pinned ? "p" : "");
  });

  is(current, aSitesPattern, "grid status = " + aSitesPattern);
}





function blockCell(aIndex) {
  whenPagesUpdated();
  getCell(aIndex).site.block();
}






function pinCell(aIndex, aPinIndex) {
  getCell(aIndex).site.pin(aPinIndex);
}





function unpinCell(aIndex) {
  whenPagesUpdated();
  getCell(aIndex).site.unpin();
}






function simulateDrop(aDropIndex, aDragIndex) {
  let draggedSite;
  let {gDrag: drag, gDrop: drop} = getContentWindow();
  let event = createDragEvent("drop", "about:blank#99\nblank");

  if (typeof aDragIndex != "undefined")
    draggedSite = getCell(aDragIndex).site;

  if (draggedSite)
    drag.start(draggedSite, event);

  whenPagesUpdated();
  drop.drop(getCell(aDropIndex), event);

  if (draggedSite)
    drag.end(draggedSite);
}







function sendDragEvent(aEventType, aTarget, aData) {
  let event = createDragEvent(aEventType, aData);
  let ifaceReq = getContentWindow().QueryInterface(Ci.nsIInterfaceRequestor);
  let windowUtils = ifaceReq.getInterface(Ci.nsIDOMWindowUtils);
  windowUtils.dispatchDOMEventViaPresShell(aTarget, event, true);
}







function createDragEvent(aEventType, aData) {
  let dataTransfer = {
    mozUserCancelled: false,
    setData: function () null,
    setDragImage: function () null,
    getData: function () aData,

    types: {
      contains: function (aType) aType == "text/x-moz-url"
    },

    mozGetDataAt: function (aType, aIndex) {
      if (aIndex || aType != "text/x-moz-url")
        return null;

      return aData;
    }
  };

  let event = getContentDocument().createEvent("DragEvents");
  event.initDragEvent(aEventType, true, true, getContentWindow(), 0, 0, 0, 0, 0,
                      false, false, false, false, 0, null, dataTransfer);

  return event;
}




function whenPagesUpdated(aCallback) {
  let page = {
    update: function () {
      NewTabUtils.allPages.unregister(this);
      executeSoon(aCallback || TestRunner.next);
    }
  };

  NewTabUtils.allPages.register(page);
  registerCleanupFunction(function () {
    NewTabUtils.allPages.unregister(page);
  });
}
