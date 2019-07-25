


const PREF_NEWTAB_ENABLED = "browser.newtabpage.enabled";

Services.prefs.setBoolPref(PREF_NEWTAB_ENABLED, true);

let tmp = {};
Cu.import("resource:///modules/NewTabUtils.jsm", tmp);
let NewTabUtils = tmp.NewTabUtils;

registerCleanupFunction(function () {
  reset();

  while (gBrowser.tabs.length > 1)
    gBrowser.removeTab(gBrowser.tabs[1]);

  Services.prefs.clearUserPref(PREF_NEWTAB_ENABLED);
});




let cw;
let cells;




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
      finish();
    }
  }
};










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




function reset() {
  NewTabUtils.reset();

  
  NewTabUtils.links._provider = originalProvider;
}




function addNewTabPageTab() {
  let tab = gBrowser.selectedTab = gBrowser.addTab("about:newtab");
  let browser = tab.linkedBrowser;

  
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);

    cw = browser.contentWindow;

    if (NewTabUtils.allPages.enabled) {
      
      NewTabUtils.links.populateCache(function () {
        cells = cw.gGrid.cells;
        executeSoon(TestRunner.next);
      });
    } else {
      TestRunner.next();
    }

  }, true);
}











function checkGrid(aSitesPattern, aSites) {
  let valid = true;

  aSites = aSites || cw.gGrid.sites;

  aSitesPattern.split(/\s*,\s*/).forEach(function (id, index) {
    let site = aSites[index];
    let match = id.match(/^\d+/);

    
    if (!match) {
      if (site) {
        valid = false;
        ok(false, "expected cell#" + index + " to be empty");
      }

      return;
    }

    
    if (!site) {
      valid = false;
      ok(false, "didn't expect cell#" + index + " to be empty");

      return;
    }

    let num = match[0];

    
    if (site.url != "about:blank#" + num) {
      valid = false;
      is(site.url, "about:blank#" + num, "cell#" + index + " has the wrong url");
    }

    let shouldBePinned = /p$/.test(id);
    let cellContainsPinned = site.isPinned();
    let cssClassPinned = site.node && site.node.querySelector(".newtab-control-pin").hasAttribute("pinned");

    
    if (shouldBePinned) {
      if (!cellContainsPinned) {
        valid = false;
        ok(false, "expected cell#" + index + " to be pinned");
      } else if (!cssClassPinned) {
        valid = false;
        ok(false, "expected cell#" + index + " to have css class 'pinned'");
      }
    } else {
      if (cellContainsPinned) {
        valid = false;
        ok(false, "didn't expect cell#" + index + " to be pinned");
      } else if (cssClassPinned) {
        valid = false;
        ok(false, "didn't expect cell#" + index + " to have css class 'pinned'");
      }
    }
  });

  
  if (valid)
    ok(true, "grid status = " + aSitesPattern);
}





function blockCell(aCell) {
  aCell.site.block(function () executeSoon(TestRunner.next));
}






function pinCell(aCell, aIndex) {
  aCell.site.pin(aIndex);
}





function unpinCell(aCell) {
  aCell.site.unpin(function () executeSoon(TestRunner.next));
}






function simulateDrop(aDropTarget, aDragSource) {
  let event = {
    clientX: 0,
    clientY: 0,
    dataTransfer: {
      mozUserCancelled: false,
      setData: function () null,
      setDragImage: function () null,
      getData: function () "about:blank#99\nblank"
    }
  };

  if (aDragSource)
    cw.gDrag.start(aDragSource.site, event);

  cw.gDrop.drop(aDropTarget, event, function () executeSoon(TestRunner.next));

  if (aDragSource)
    cw.gDrag.end(aDragSource.site);
}




function whenPagesUpdated() {
  NewTabUtils.allPages.register({
    update: function () {
      NewTabUtils.allPages.unregister(this);
      executeSoon(TestRunner.next);
    }
  });
}
