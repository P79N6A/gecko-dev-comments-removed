







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const testPageURL = "http://localhost:8888/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_windowtitle_page.html";
  waitForExplicitFinish();

  
  let test_title = "Test title";
  let app_name = document.documentElement.getAttribute("title");
  const isOSX = ("nsILocalFileMac" in Ci);
  let page_with_title;
  let page_without_title;
  let about_pb_title;
  let pb_page_with_title;
  let pb_page_without_title;
  let pb_about_pb_title;
  if (isOSX) {
    page_with_title = test_title;
    page_without_title = app_name;
    about_pb_title = "Would you like to start Private Browsing?";
    pb_page_with_title = test_title + " - (Private Browsing)";
    pb_page_without_title = app_name + " - (Private Browsing)";
    pb_about_pb_title = pb_page_without_title;
  }
  else {
    page_with_title = test_title + " - " + app_name;
    page_without_title = app_name;
    about_pb_title = "Would you like to start Private Browsing?" + " - " + app_name;
    pb_page_with_title = test_title + " - " + app_name + " (Private Browsing)";
    pb_page_without_title = app_name + " (Private Browsing)";
    pb_about_pb_title = "Private Browsing - " + app_name + " (Private Browsing)";
  }

  function testTabTitle(url, insidePB, expected_title, funcNext) {
    pb.privateBrowsingEnabled = insidePB;

    let tab = gBrowser.addTab();
    gBrowser.selectedTab = tab;
    let browser = gBrowser.getBrowserForTab(tab);
    browser.addEventListener("load", function() {
      browser.removeEventListener("load", arguments.callee, true);

      
      setTimeout(function() {
        setTimeout(function() {
          is(document.title, expected_title, "The window title for " + url +
             " is correct (" + (insidePB ? "inside" : "outside") +
             " private browsing mode)");

          let win = gBrowser.replaceTabWithWindow(tab);
          win.addEventListener("load", function() {
            win.removeEventListener("load", arguments.callee, false);

            
            setTimeout(function() {
              setTimeout(function() {
                is(win.document.title, expected_title, "The window title for " + url +
                   " detahced tab is correct (" + (insidePB ? "inside" : "outside") +
                   " private browsing mode)");
                win.close();

                funcNext();
              }, 0);
            }, 0);
          }, false);
        }, 0);
      }, 0);
    }, true);
    browser.loadURI(url);
  }

  function cleanup() {
    pb.privateBrowsingEnabled = false;
    prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
    finish();
  }

  testTabTitle("about:blank", false, page_without_title, function() {
    testTabTitle(testPageURL, false, page_with_title, function() {
      testTabTitle("about:privatebrowsing", false, about_pb_title, function() {
        testTabTitle("about:blank", true, pb_page_without_title, function() {
          testTabTitle(testPageURL, true, pb_page_with_title, function() {
            testTabTitle("about:privatebrowsing", true, pb_about_pb_title, cleanup);
          });
        });
      });
    });
  });
  return;
}
