


let pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);

function test() {
  let thumbnailsSaved = false;

  waitForExplicitFinish();

  registerCleanupFunction(function () {
    ok(thumbnailsSaved, "thumbs have been saved before entering pb mode");
    pb.privateBrowsingEnabled = false;
  });

  afterAllTabsLoaded(function () {
    showTabView(function () {
      hideTabView(function () {
        let numConditions = 2;

        function check() {
          if (--numConditions)
            return;

          togglePrivateBrowsing(finish);
        }

        let tabItem = gBrowser.tabs[0]._tabViewTabItem;

        
        
        tabItem.saveThumbnail({synchronously: true});

        
        tabItem.tabCanvas.paint();

        tabItem.addSubscriber("savedCachedImageData", function onSaved() {
          tabItem.removeSubscriber("savedCachedImageData", onSaved);
          thumbnailsSaved = true;
          check();
        });

        togglePrivateBrowsing(check);
      });
    });
  });
}
