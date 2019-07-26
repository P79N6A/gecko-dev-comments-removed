




"use strict";




function mockLinks(aLinks) {
  
  
  let links = (typeof aLinks == "string") ?
              aLinks.split(/\s*,\s*/) : aLinks;

  links = links.map(function (id) {
    return (id) ? {url: "http://example.com/#" + id, title: id} : null;
  });
  return links;
}

function clearHistory() {
  PlacesUtils.history.removeAllPages();
}

function fillHistory(aLinks) {
  return Task.spawn(function(){
    let numLinks = aLinks.length;
    let transitionLink = Ci.nsINavHistoryService.TRANSITION_LINK;

    let updateDeferred = Promise.defer();

    for (let link of aLinks.reverse()) {
      info("fillHistory with link: " + JSON.stringify(link));
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
}










function setPinnedLinks(aLinks) {
  let links = mockLinks(aLinks);

  
  
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
}














function setLinks(aLinks, aPinnedLinks) {
  let links = mockLinks(aLinks);
  if (links.filter(function(aLink){
    return !aLink;
  }).length) {
    throw new Error("null link objects in setLinks");
  }

  return Task.spawn(function() {
    clearHistory();

    yield Task.spawn(fillHistory(links));

    if(aPinnedLinks) {
      setPinnedLinks(aPinnedLinks);
    }

    
    yield TopSites.prepareCache(true);
  });
}

function updatePagesAndWait() {
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
}



function test() {
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
    
    yield setLinks("brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad");
    let grid = document.getElementById("start-topsites-grid");

    yield updatePagesAndWait();
    
    yield waitForCondition(function(){
      return !grid.controller.isUpdating;
    });
  },
  run: function() {
    let grid = document.getElementById("start-topsites-grid");
    let items = grid.children;
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
    
    yield setLinks(
      "brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad",
      this.pins
    );
    yield updatePagesAndWait();
    
    yield waitForCondition(function(){
      let grid = document.getElementById("start-topsites-grid");
      return !grid.controller.isUpdating;
    });
  },
  run: function() {
    
    let pins = this.pins.split(",");
    let items = document.getElementById("start-topsites-grid").children;
    is(items.length, 8, "should be 8 topsites in the grid");

    is(document.querySelectorAll("#start-topsites-grid > [pinned]").length, 3, "should be 3 children with 'pinned' attribute");
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
    
    yield setLinks("sgtsam,train,zebedee,zeebad", []); 
    yield updatePagesAndWait();
    
    yield waitForCondition(function(){
      let grid = document.getElementById("start-topsites-grid");
      return !grid.controller.isUpdating;
    });
  },
  run: function() {
    
    
    
    let grid = document.getElementById("start-topsites-grid"),
        items = grid.children;
    is(items.length, 4, this.desc + ": should be 4 topsites");

    let tile = grid.children[2],
        url = tile.getAttribute("value"),
        title = tile.getAttribute("label");

    info(this.desc + ": pinning site at index 2");
    TopSites.pinSite({
      url: url,
      title: title
    }, 2);

    yield waitForCondition(function(){
      return !grid.controller.isUpdating;
    });

    let thirdTile = grid.children[2];
    ok( thirdTile.hasAttribute("pinned"), thirdTile.getAttribute("value")+ " should look pinned" );

    
    yield fillHistory( mockLinks("brian,dougal,dylan,ermintrude,florence,moose") );

    
    yield TopSites.prepareCache(true);
    yield updatePagesAndWait();

    
    yield waitForCondition(function(){
      return !grid.controller.isUpdating;
    });

    
    is( items[2].getAttribute("label"), "zebedee", "Pinned site remained at its index" );
    ok( items[2].hasAttribute("pinned"), "3rd site should still look pinned" );
  }
});

gTests.push({
  desc: "unpin site",
  pins: ",zebedee",
  setUp: function() {
    try {
      
      yield setLinks(
        "brian,dougal,dylan,ermintrude,florence,moose,sgtsam,train,zebedee,zeebad",
        this.pins
      );
      yield updatePagesAndWait();

      
      yield waitForCondition(function(){
        let grid = document.getElementById("start-topsites-grid");
        return !grid.controller.isUpdating;
      });
    } catch(e) {
      info("caught error in setUp: " + e);
      info("trace: " + e.stack);
    }
  },
  run: function() {
    
    
    let grid = document.getElementById("start-topsites-grid"),
        items = grid.children;
    is(items.length, 8, this.desc + ": should be 8 topsites");
    let site = {
      url: items[1].getAttribute("value"),
      title: items[1].getAttribute("label")
    };
    
    ok( NewTabUtils.pinnedLinks.isPinned(site), "2nd item is pinned" );
    ok( items[1].hasAttribute("pinned"), "2nd item has pinned attribute" );

    TopSites.unpinSite(site);

    yield waitForCondition(function(){
      return !grid.controller.isUpdating;
    });

    let secondTile = grid.children[1];
    ok( !secondTile.hasAttribute("pinned"), "2nd item should no longer be marked as pinned" );
    ok( !NewTabUtils.pinnedLinks.isPinned(site), "2nd item should no longer be pinned" );
  }
});
