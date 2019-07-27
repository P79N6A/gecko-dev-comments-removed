




function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "https://www.mozilla.org/en-US/";
  let favIconLocation =
    "http://example.org/tests/toolkit/components/places/tests/browser/favicon-normal32.png";
  let favIconURI = NetUtil.newURI(favIconLocation);
  let favIconMimeType= "image/png";
  let pageURI;
  let favIconData;

  function testOnWindow(aOptions, aCallback) {
    whenNewWindowLoaded(aOptions, function(aWin) {
      windowsToClose.push(aWin);
      executeSoon(function() aCallback(aWin));
    });
  };

  
  registerCleanupFunction(function() {
    windowsToClose.forEach(function(aWin) {
      aWin.close();
    });
  });

  function getIconFile(aCallback) {
    NetUtil.asyncFetch2(
      favIconLocation,
      function(inputStream, status) {
        if (!Components.isSuccessCode(status)) {
          ok(false, "Could not get the icon file");
          
          return;
        }

        
        let size = inputStream.available();
        favIconData = NetUtil.readInputStreamToString(inputStream, size);
        is(size, favIconData.length, "Check correct icon size");
        
        is(favIconData.length, 344, "Check correct icon length (344)");

        if (aCallback) {
          aCallback();
        } else {
          finish();
        }
      },
      null,      
      Services.scriptSecurityManager.getSystemPrincipal(),
      null,      
      Ci.nsILoadInfo.SEC_NORMAL,
      Ci.nsIContentPolicy.TYPE_IMAGE);
  }

  function testNormal(aWindow, aCallback) {
    pageURI = NetUtil.newURI("http://example.com/normal");
    waitForFaviconChanged(pageURI, favIconURI, aWindow,
      function testNormalCallback() {
        checkFaviconDataForPage(pageURI, favIconMimeType, favIconData, aWindow,
          aCallback);
      }
    );

    addVisits({uri: pageURI, transition: TRANSITION_TYPED}, aWindow,
      function () {
        aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI,
          favIconURI, true, aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
      }
    );
  }

  function testAboutURIBookmarked(aWindow, aCallback) {
    pageURI = NetUtil.newURI("about:testAboutURI_bookmarked");
    waitForFaviconChanged(pageURI, favIconURI, aWindow,
      function testAboutURIBookmarkedCallback() {
        checkFaviconDataForPage(pageURI, favIconMimeType, favIconData, aWindow,
          aCallback);
      }
    );

    aWindow.PlacesUtils.bookmarks.insertBookmark(
      aWindow.PlacesUtils.unfiledBookmarksFolderId, pageURI,
      aWindow.PlacesUtils.bookmarks.DEFAULT_INDEX, pageURI.spec);
    aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI, favIconURI,
      true, aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
  }

  function testPrivateBrowsingBookmarked(aWindow, aCallback) {
    pageURI = NetUtil.newURI("http://example.com/privateBrowsing_bookmarked");
    waitForFaviconChanged(pageURI, favIconURI, aWindow,
      function testPrivateBrowsingBookmarkedCallback() {
        checkFaviconDataForPage(pageURI, favIconMimeType, favIconData, aWindow,
          aCallback);
      }
    );

    aWindow.PlacesUtils.bookmarks.insertBookmark(
      aWindow.PlacesUtils.unfiledBookmarksFolderId, pageURI,
      aWindow.PlacesUtils.bookmarks.DEFAULT_INDEX, pageURI.spec);
    aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI, favIconURI,
      true, aWindow.PlacesUtils.favicons.FAVICON_LOAD_PRIVATE);
  }

  function testDisabledHistoryBookmarked(aWindow, aCallback) {
    pageURI = NetUtil.newURI("http://example.com/disabledHistory_bookmarked");
    waitForFaviconChanged(pageURI, favIconURI, aWindow,
      function testDisabledHistoryBookmarkedCallback() {
        checkFaviconDataForPage(pageURI, favIconMimeType, favIconData, aWindow,
          aCallback);
      }
    );

    
    aWindow.Services.prefs.setBoolPref("places.history.enabled", false);

    aWindow.PlacesUtils.bookmarks.insertBookmark(
      aWindow.PlacesUtils.unfiledBookmarksFolderId, pageURI,
      aWindow.PlacesUtils.bookmarks.DEFAULT_INDEX, pageURI.spec);
    aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI, favIconURI,
      true, aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);

    
    
    
    aWindow.Services.prefs.setBoolPref("places.history.enabled", true);
  }

  getIconFile(function () {
    testOnWindow({}, function(aWin) {
      testNormal(aWin, function () {
        testOnWindow({}, function(aWin) {
          testAboutURIBookmarked(aWin, function () {
            testOnWindow({private: true}, function(aWin) {
              testPrivateBrowsingBookmarked(aWin, function () {
                testOnWindow({}, function(aWin) {
                  testDisabledHistoryBookmarked(aWin, finish);
                });
              });
            });
          });
        });
      });
    });
  });
}
