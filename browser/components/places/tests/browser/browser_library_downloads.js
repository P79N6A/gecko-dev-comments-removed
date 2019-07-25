











































let now = Date.now();

function test() {
  waitForExplicitFinish();

  function windowObserver(aSubject, aTopic, aData) {
    if (aTopic != "domwindowopened")
      return;
    Services.ww.unregisterNotification(windowObserver);
    let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
    win.addEventListener("load", function onLoad(event) {
      win.removeEventListener("load", onLoad, false);
      executeSoon(function () {
        
        fastAddVisit("http://mozilla.com",
                     PlacesUtils.history.TRANSITION_TYPED);
        fastAddVisit("http://google.com",
                     PlacesUtils.history.TRANSITION_DOWNLOAD);
        fastAddVisit("http://en.wikipedia.org",
                     PlacesUtils.history.TRANSITION_TYPED);
        fastAddVisit("http://ubuntu.org",
                     PlacesUtils.history.TRANSITION_DOWNLOAD);

        
        isnot(win.PlacesOrganizer._places.selectedNode, null,
              "Downloads is present and selected");

        
        let tree = win.document.getElementById("placeContent");
        isnot(tree, null, "placeContent tree exists");

        
        var contentRoot = tree.result.root;
        var len = contentRoot.childCount;
        var testUris = ["http://ubuntu.org/", "http://google.com/"];
        for (var i = 0; i < len; i++) {
          is(contentRoot.getChild(i).uri, testUris[i],
             "Comparing downloads shown at index " + i);
        }

        win.close();
        waitForClearHistory(finish);
      });
    }, false);
  }

  Services.ww.registerNotification(windowObserver);
  PlacesCommandHook.showPlacesOrganizer("Downloads");
}

function fastAddVisit(uri, transition) {
  PlacesUtils.history.addVisit(PlacesUtils._uri(uri), now++ * 1000,
                               null, transition, false, 0);
}

function waitForClearHistory(aCallback) {
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(this, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
      aCallback();
    }
  };
  Services.obs.addObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  PlacesUtils.bhistory.removeAllPages();
}
