




"use strict";




let TopSitesTestHelper = {
  get grid() {
    return Browser.selectedBrowser.contentDocument.getElementById("start-topsites-grid");
  },
  get document() {
    return Browser.selectedBrowser.contentDocument;
  },
  setup: function() {
    return Task.spawn(function(){
      if (BrowserUI.isStartTabVisible)
        return;

      yield addTab("about:start");

      yield waitForCondition(() => BrowserUI.isStartTabVisible);
    });
  },
  mockLinks: function th_mockLinks(aLinks) {
    
    
    let links = (typeof aLinks == "string") ?
                aLinks.split(/\s*,\s*/) : aLinks;

    links = links.map(function (id) {
      return (id) ? {url: "http://example.com/#" + id, title: id} : null;
    });
    return links;
  },
  siteFromNode: function th_siteFromNode(aNode) {
    return {
      url: aNode.getAttribute("value"),
      title: aNode.getAttribute("label")
    }
  },
  clearHistory: function th_clearHistory() {
    PlacesUtils.history.removeAllPages();
  },
  fillHistory: function th_fillHistory(aLinks) {
    return Task.spawn(function(){
      let numLinks = aLinks.length;
      let transitionLink = Ci.nsINavHistoryService.TRANSITION_LINK;

      let updateDeferred = Promise.defer();

      for (let link of aLinks.reverse()) {
        let place = {
          uri: Util.makeURI(link.url),
          title: link.title,
          visits: [{visitDate: Date.now() * 1000, transitionType: transitionLink}]
        };
        try {
          PlacesUtils.asyncHistory.updatePlaces(place, {
            handleError: function (aError) {
              ok(false, "couldn't add visit to history");
              throw new Task.Result(aError);
            },
            handleResult: function () {},
            handleCompletion: function () {
              if(--numLinks <= 0) {
                updateDeferred.resolve(true);
              }
            }
          });
        } catch(e) {
          ok(false, "because: " + e);
        }
      }
      return updateDeferred.promise;
    });
  },

  








  setPinnedLinks: function th_setPinnedLinks(aLinks) {
    let links = TopSitesTestHelper.mockLinks(aLinks);

    
    
    Array.forEach(NewTabUtils.pinnedLinks.links, function(aLink){
      if(aLink)
        NewTabUtils.pinnedLinks.unpin(aLink);
    });

    links.forEach(function(aLink, aIndex){
      if(aLink) {
        NewTabUtils.pinnedLinks.pin(aLink, aIndex);
      }
    });
    NewTabUtils.pinnedLinks.save();
  },

  












  setLinks: function th_setLinks(aLinks, aPinnedLinks) {
    let links = TopSitesTestHelper.mockLinks(aLinks);
    if (links.filter(function(aLink){
      return !aLink;
    }).length) {
      throw new Error("null link objects in setLinks");
    }

    return Task.spawn(function() {
      TopSitesTestHelper.clearHistory();

      yield Task.spawn(TopSitesTestHelper.fillHistory(links));

      if(aPinnedLinks) {
        TopSitesTestHelper.setPinnedLinks(aPinnedLinks);
      }

      
      yield TopSites.prepareCache(true);
    });
  },

  updatePagesAndWait: function th_updatePagesAndWait() {
    let deferredUpdate = Promise.defer();
    let updater = {
      update: function() {
        NewTabUtils.allPages.unregister(updater);
        deferredUpdate.resolve(true);
      }
    };
    NewTabUtils.allPages.register(updater);
    setTimeout(function() {
      NewTabUtils.allPages.update();
    }, 0);
    return deferredUpdate.promise;
  },

  tearDown: function th_tearDown() {
    TopSitesTestHelper.clearHistory();
  }
};





function test() {
  registerCleanupFunction(TopSitesTestHelper.tearDown);
  runTests();
}

gTests.push({
  desc: "TopSites dependencies",
  run: function() {
    ok(NewTabUtils, "NewTabUtils is truthy");
    ok(TopSites, "TopSites is truthy");
  }
});

gTests.push({
  desc: "load and display top sites",
  setUp: function() {
    yield TopSitesTestHelper.setup();
    let grid = TopSitesTestHelper.grid;

    
    yield TopSitesTestHelper.setLinks("brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad");

    let arrangedPromise = waitForEvent(grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    
    yield arrangedPromise;
  },
  run: function() {
    let grid = TopSitesTestHelper.grid;
    let items = grid.items;
    is(items.length, 8, "should be 8 topsites"); 
    if(items.length) {
      let firstitem = items[0];
      is(
        firstitem.getAttribute("label"),
        "brian",
        "first item label should be 'brian': " + firstitem.getAttribute("label")
      );
      is(
        firstitem.getAttribute("value"),
        "http://example.com/#brian",
        "first item url should be 'http://example.com/#brian': " + firstitem.getAttribute("url")
      );
    }
  }
});

gTests.push({
  desc: "pinned sites",
  pins: "dangermouse,zebedee,,,dougal",
  setUp: function() {
    yield TopSitesTestHelper.setup();
    
    yield TopSitesTestHelper.setLinks(
      "brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad",
      this.pins
    );
    
    let arrangedPromise = waitForEvent(TopSitesTestHelper.grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    yield arrangedPromise;
  },
  run: function() {
    
    let pins = this.pins.split(",");
    let items = TopSitesTestHelper.grid.items;
    is(items.length, 8, "should be 8 topsites in the grid");

    is(TopSitesTestHelper.document.querySelectorAll("#start-topsites-grid > [pinned]").length, 3, "should be 3 children with 'pinned' attribute");
    try {
      Array.forEach(items, function(aItem, aIndex){
        
        is(
            aItem.hasAttribute("pinned"), !!pins[aIndex],
            "site at index " + aIndex + " was " +aItem.hasAttribute("pinned")
            +", should agree with " + !!pins[aIndex]
        );
        if (pins[aIndex]) {
          is(
            aItem.getAttribute("label"),
            pins[aIndex],
            "pinned site has correct label: " + pins[aIndex] +"=="+ aItem.getAttribute("label")
          );
        }
      }, this);
    } catch(e) {
      ok(false, this.desc + ": Test of pinned state on items failed");
      info("because: " + e.message + "\n" + e.stack);
    }
  }
});

gTests.push({
  desc: "pin site",
  setUp: function() {
    yield TopSitesTestHelper.setup();
    
    yield TopSitesTestHelper.setLinks("sgtsam,train,zebedee,zeebad", []); 
    
    let arrangedPromise = waitForEvent(TopSitesTestHelper.grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    yield arrangedPromise;
  },
  run: function() {
    
    
    
    let grid = TopSitesTestHelper.grid,
        items = grid.items;
    is(items.length, 4, this.desc + ": should be 4 topsites");

    let tile = grid.items[2],
        url = tile.getAttribute("value"),
        title = tile.getAttribute("label");

    info(this.desc + ": pinning site at index 2");
    TopSites.pinSites([{
          url: url,
          title: title
        }], [2]);

    
    yield waitForCondition(function(){
      return !grid.controller.isUpdating;
    });

    let thirdTile = grid.items[2];
    ok( thirdTile.hasAttribute("pinned"), thirdTile.getAttribute("value")+ " should look pinned" );

    
    yield TopSitesTestHelper.fillHistory( TopSitesTestHelper.mockLinks("brian,dougal,dylan,ermintrude,florence,moose") );

    
    yield TopSites.prepareCache(true);
    
    let arrangedPromise = waitForEvent(grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    yield arrangedPromise;

    
    is( items[2].getAttribute("label"), "zebedee", "Pinned site remained at its index" );
    ok( items[2].hasAttribute("pinned"), "3rd site should still look pinned" );
  }
});

gTests.push({
  desc: "unpin site",
  pins: ",zebedee",
  setUp: function() {
    yield TopSitesTestHelper.setup();
    
    yield TopSitesTestHelper.setLinks(
      "brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad",
      this.pins
    );
    
    let arrangedPromise = waitForEvent(TopSitesTestHelper.grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    yield arrangedPromise;
  },
  run: function() {
    
    
    let grid = TopSitesTestHelper.grid,
        items = grid.items;
    is(items.length, 8, this.desc + ": should be 8 topsites");
    let site = {
      url: items[1].getAttribute("value"),
      title: items[1].getAttribute("label")
    };
    
    ok( NewTabUtils.pinnedLinks.isPinned(site), "2nd item is pinned" );
    ok( items[1].hasAttribute("pinned"), "2nd item has pinned attribute" );

    
    TopSites.unpinSites([site]);

    yield waitForCondition(function(){
      return !grid.controller.isUpdating;
    });

    let secondTile = grid.items[1];
    ok( !secondTile.hasAttribute("pinned"), "2nd item should no longer be marked as pinned" );
    ok( !NewTabUtils.pinnedLinks.isPinned(site), "2nd item should no longer be pinned" );
  }
});

gTests.push({
  desc: "block/unblock sites",
  setUp: function() {
    yield TopSitesTestHelper.setup();
    
    yield TopSitesTestHelper.setLinks(
      "brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad,basic,coral",
      ",dougal"
    );
    
    let arrangedPromise = waitForEvent(TopSitesTestHelper.grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    yield arrangedPromise;
  },
  run: function() {
    try {
      
      
      let grid = TopSitesTestHelper.grid,
          items = grid.items;
      is(items.length, 8, this.desc + ": should be 8 topsites");

      let brianSite = TopSitesTestHelper.siteFromNode(items[0]);
      let dougalSite = TopSitesTestHelper.siteFromNode(items[1]);
      let dylanSite = TopSitesTestHelper.siteFromNode(items[2]);

      let arrangedPromise = waitForEvent(grid, "arranged");
      
      TopSites.hideSites([brianSite]);
      
      yield arrangedPromise;

      
      ok( (new Site(brianSite)).blocked, "Site has truthy blocked property" );
      ok( NewTabUtils.blockedLinks.isBlocked(brianSite), "Site was blocked" );
      is( grid.querySelectorAll("[value='"+brianSite.url+"']").length, 0, "Blocked site was removed from grid");

      
      is(items.length, 8, this.desc + ": should be 8 topsites");

      arrangedPromise = waitForEvent(grid, "arranged");
      
      TopSites.hideSites([dougalSite, dylanSite]);
      
      yield arrangedPromise;

      
      ok( (new Site(dougalSite)).blocked, "Site has truthy blocked property" );
      ok( NewTabUtils.blockedLinks.isBlocked(dougalSite), "Site was blocked" );
      ok( !NewTabUtils.pinnedLinks.isPinned(dougalSite), "Blocked Site is no longer pinned" );
      is( grid.querySelectorAll("[value='"+dougalSite.url+"']").length, 0, "Blocked site was removed from grid");

      
      ok( (new Site(dylanSite)).blocked, "Site has truthy blocked property" );
      ok( NewTabUtils.blockedLinks.isBlocked(dylanSite), "Site was blocked" );
      ok( !NewTabUtils.pinnedLinks.isPinned(dylanSite), "Blocked Site is no longer pinned" );
      is( grid.querySelectorAll("[value='"+dylanSite.url+"']").length, 0, "Blocked site was removed from grid");

      
      is(items.length, 8, this.desc + ": should be 8 topsites");

      arrangedPromise = waitForEvent(grid, "arranged");
      TopSites.restoreSites([brianSite, dougalSite, dylanSite]);
      
      yield arrangedPromise;

      
      ok( !NewTabUtils.blockedLinks.isBlocked(brianSite), "site was unblocked" );
      is( grid.querySelectorAll("[value='"+brianSite.url+"']").length, 1, "Unblocked site is back in the grid");

      ok( !NewTabUtils.blockedLinks.isBlocked(dougalSite), "site was unblocked" );
      is( grid.querySelectorAll("[value='"+dougalSite.url+"']").length, 1, "Unblocked site is back in the grid");
      
      ok( NewTabUtils.pinnedLinks.isPinned(dougalSite), "Restoring previously pinned site makes it pinned again" );
      is( grid.items[1].getAttribute("value"), dougalSite.url, "Blocked Site restored to pinned index" );

      ok( !NewTabUtils.blockedLinks.isBlocked(dylanSite), "site was unblocked" );
      is( grid.querySelectorAll("[value='"+dylanSite.url+"']").length, 1, "Unblocked site is back in the grid");

    } catch(ex) {

      ok(false, this.desc+": Caught exception in test: " + ex);
      info("trace: " + ex.stack);
    }
  }
});

gTests.push({
  desc: "delete and restore site tiles",
  pins: "brian",
  setUp: function() {
    yield TopSitesTestHelper.setup();
    
    yield TopSitesTestHelper.setLinks(
      "brian,dougal,dylan,ermintrude",
      this.pins
    );
    
    let arrangedPromise = waitForEvent(TopSitesTestHelper.grid, "arranged");
    yield TopSitesTestHelper.updatePagesAndWait();
    yield arrangedPromise;
  },
  run: function() {
    
    
    let grid = TopSitesTestHelper.grid,
        items = grid.items;
    is(items.length, 4, this.desc + ": should be 4 topsites");

    let brianTile = grid.querySelector('richgriditem[value$="brian"]');
    let dougalTile = grid.querySelector('richgriditem[value$="dougal"]')

    
    ok( brianTile, "Tile for Brian was created");
    ok( dougalTile, "Tile for Dougal was created");

    let brianSite = TopSitesTestHelper.siteFromNode(brianTile);
    let dougalSite = TopSitesTestHelper.siteFromNode(dougalTile);
    ok( NewTabUtils.pinnedLinks.isPinned( brianSite ), "Brian tile is pinned" );

    
    grid.toggleItemSelection(brianTile);
    grid.toggleItemSelection(dougalTile);
    is(grid.selectedItems.length, 2, "2 tiles were selected");

    
    let arrangedPromise = waitForEvent(grid, "arranged");

    
    let event = document.createEvent("Events");
    event.action = "delete";
    event.initEvent("context-action", true, true); 
    grid.dispatchEvent(event);

    yield arrangedPromise;

    
    ok( !grid.querySelector('richgriditem[value="'+brianSite.value+'"]'));
    ok( !grid.querySelector('richgriditem[value="'+dougalSite.value+'"]'));
    ok( NewTabUtils.blockedLinks.isBlocked(brianSite), "Brian site was blocked" );
    ok( NewTabUtils.blockedLinks.isBlocked(dougalSite), "Dougal site was blocked" );
    
    is( grid.selectedItems.length, 0, "Gris selection is empty after deletion" );

    
    arrangedPromise = waitForEvent(grid, "arranged");
    event = document.createEvent("Events");
    event.action = "restore";
    event.initEvent("context-action", true, true); 
    grid.dispatchEvent(event);

    yield arrangedPromise;
    brianTile = grid.querySelector('richgriditem[value$="brian"]');
    dougalTile = grid.querySelector('richgriditem[value$="dougal"]');

    
    ok( brianTile, "First tile was restored to the grid" );
    ok( dougalTile, "2nd tile was restored to the grid" );

    is(grid.selectedItems.length, 2, "2 tiles are still selected");
    is( grid.selectedItems[0], brianTile, "Brian is still selected" );
    is( grid.selectedItems[1], dougalTile, "Dougal is still selected" );
    ok( NewTabUtils.pinnedLinks.isPinned( brianSite ), "Brian tile is still pinned" );
    ok( !NewTabUtils.blockedLinks.isBlocked(brianSite), "Brian site was unblocked" );
    ok( !NewTabUtils.blockedLinks.isBlocked(dougalSite), "Dougal site was unblocked" );

  }
});
