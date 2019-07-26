







function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let favIcon16Location =
    "http://example.org/tests/toolkit/components/places/tests/browser/favicon-normal16.png";
  let favIcon32Location =
    "http://example.org/tests/toolkit/components/places/tests/browser/favicon-normal32.png";
  let favIcon16URI = NetUtil.newURI(favIcon16Location);
  let favIcon32URI = NetUtil.newURI(favIcon32Location);
  let lastPageURI = NetUtil.newURI("http://example.com/verification");
  
  
  let favIconErrorPageURI =
    NetUtil.newURI("chrome://global/skin/icons/warning-16.png");
  let favIconsResultCount = 0;
  let pageURI;

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

  function checkFavIconsDBCount(aCallback) {
    let stmt = DBConn().createAsyncStatement("SELECT url FROM moz_favicons");
    stmt.executeAsync({
      handleResult: function final_handleResult(aResultSet) {
        for (let row; (row = aResultSet.getNextRow()); ) {
          favIconsResultCount++;
        }
      },
      handleError: function final_handleError(aError) {
        throw("Unexpected error (" + aError.result + "): " + aError.message);
      },
      handleCompletion: function final_handleCompletion(aReason) {
        
        info("Previous records in moz_favicons: " + favIconsResultCount);
        if (aCallback) {
          aCallback();
        }
      }
    });
    stmt.finalize();
  }

  function testNullPageURI(aWindow, aCallback) {
    try {
      aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(null, favIcon16URI,
        true, aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
      throw("Exception expected because aPageURI is null.");
    } catch (ex) {
      
      ok(true, "Exception expected because aPageURI is null");
    }

    if (aCallback) {
      aCallback();
    }
  }

  function testNullFavIconURI(aWindow, aCallback) {
    try {
      aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(
        NetUtil.newURI("http://example.com/null_faviconURI"), null, true,
          aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
      throw("Exception expected because aFaviconURI is null.");
    } catch (ex) {
      
      ok(true, "Exception expected because aFaviconURI is null.");
    }

    if (aCallback) {
      aCallback();
    }
  }

  function testAboutURI(aWindow, aCallback) {
    aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(
      NetUtil.newURI("about:testAboutURI"), favIcon16URI, true,
        aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);

    if (aCallback) {
      aCallback();
    }
  }

  function testPrivateBrowsingNonBookmarkedURI(aWindow, aCallback) {
    let pageURI = NetUtil.newURI("http://example.com/privateBrowsing");
    addVisits({ uri: pageURI, transitionType: TRANSITION_TYPED }, aWindow,
      function () {
        aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI,
          favIcon16URI, true, aWindow.PlacesUtils.favicons.FAVICON_LOAD_PRIVATE);

        if (aCallback) {
          aCallback();
        }
    });
  }

  function testDisabledHistory(aWindow, aCallback) {
    let pageURI = NetUtil.newURI("http://example.com/disabledHistory");
    addVisits({ uri: pageURI, transition: TRANSITION_TYPED }, aWindow,
      function () {
        aWindow.Services.prefs.setBoolPref("places.history.enabled", false);

        aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI,
          favIcon16URI, true,
            aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);

        
        
        
        aWindow.Services.prefs.setBoolPref("places.history.enabled", true);

        if (aCallback) {
          aCallback();
        }
    });
  }

  function testErrorIcon(aWindow, aCallback) {
    let pageURI = NetUtil.newURI("http://example.com/errorIcon");
    let places = [{ uri: pageURI, transition: TRANSITION_TYPED }];
    addVisits({ uri: pageURI, transition: TRANSITION_TYPED }, aWindow,
      function () {
        aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI,
          favIconErrorPageURI, true,
            aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);

      if (aCallback) {
        aCallback();
      }
    });
  }

  function testNonExistingPage(aWindow, aCallback) {
    aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(
      NetUtil.newURI("http://example.com/nonexistingPage"), favIcon16URI, true,
        aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);

    if (aCallback) {
      aCallback();
    }
  }

  function testFinalVerification(aWindow, aCallback) {
    
    
    waitForFaviconChanged(lastPageURI, favIcon32URI, aWindow,
      function final_callback() {
        
        let resultCount = 0;
        let stmt = DBConn().createAsyncStatement("SELECT url FROM moz_favicons");
        stmt.executeAsync({
          handleResult: function final_handleResult(aResultSet) {

            
            
            
            if (favIconsResultCount > 0) {
              for (let row; (row = aResultSet.getNextRow()); ) {
                if (favIcon32URI.spec === row.getResultByIndex(0)) {
                  is(favIcon32URI.spec, row.getResultByIndex(0),
                    "Check equal favicons");
                  resultCount++;
                }
              }
            } else {
              for (let row; (row = aResultSet.getNextRow()); ) {
                is(favIcon32URI.spec, row.getResultByIndex(0),
                  "Check equal favicons");
                resultCount++;
              }
            }
          },
          handleError: function final_handleError(aError) {
            throw("Unexpected error (" + aError.result + "): " + aError.message);
          },
          handleCompletion: function final_handleCompletion(aReason) {
            is(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason,
              "Check reasons are equal");
            is(1, resultCount, "Check result count");
            if (aCallback) {
              aCallback();
            }
          }
        });
        stmt.finalize();
    });

    
    
    
    addVisits({ uri: lastPageURI, transition: TRANSITION_TYPED }, aWindow,
      function () {
        aWindow.PlacesUtils.favicons.setAndFetchFaviconForPage(lastPageURI,
          favIcon32URI, true,
            aWindow.PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
    });
  }

  checkFavIconsDBCount(function () {
    testOnWindow({}, function(aWin) {
      testNullPageURI(aWin, function () {
        testOnWindow({}, function(aWin) {
          testNullFavIconURI(aWin, function() {
            testOnWindow({}, function(aWin) {
              testAboutURI(aWin, function() {
                testOnWindow({private: true}, function(aWin) {
                  testPrivateBrowsingNonBookmarkedURI(aWin, function () {
                    testOnWindow({}, function(aWin) {
                      testDisabledHistory(aWin, function () {
                        testOnWindow({}, function(aWin) {
                          testErrorIcon(aWin, function() {
                            testOnWindow({}, function(aWin) {
                              testNonExistingPage(aWin, function() {
                                testOnWindow({}, function(aWin) {
                                  testFinalVerification(aWin, function() {
                                    finish();
                                  });
                                });
                              });
                            });
                          });
                        });
                      });
                    });
                  });
                });
              });
            });
          });
        });
      });
    });
  });
}
