







































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
    var page_with_title;
    var page_without_title;
    var pb_page_with_title;
    var pb_page_without_title;
    var about_pb_title;
  if (isOSX) {
    page_with_title = test_title;
    page_without_title = app_name;
    pb_page_with_title = test_title + " - (Private Browsing)";
    pb_page_without_title = app_name + " - (Private Browsing)";
    about_pb_title = pb_page_without_title;
  }
  else {
    page_with_title = test_title + " - " + app_name;
    page_without_title = app_name;
    pb_page_with_title = test_title + " - " + app_name + " (Private Browsing)";
    pb_page_without_title = app_name + " (Private Browsing)";
    about_pb_title = "Private Browsing - " + app_name + " (Private Browsing)";
  }

  
  let blankTab = gBrowser.addTab();
  gBrowser.selectedTab = blankTab;
  is(document.title, page_without_title, "The window title for a page without a title matches " +
    "(outside private browsing mode)");
  gBrowser.removeTab(blankTab);

  let pageTab = gBrowser.addTab();
  gBrowser.selectedTab = pageTab;
  let pageBrowser = gBrowser.getBrowserForTab(pageTab);
  pageBrowser.addEventListener("load", function () {
    pageBrowser.removeEventListener("load", arguments.callee, true);

    
    is(document.title, page_with_title, "The window title for a page with a title matches " +
      "(outside private browsing mode)");

    gBrowser.removeTab(pageTab);

    
    pb.privateBrowsingEnabled = true;

    
    blankTab = gBrowser.addTab();
    gBrowser.selectedTab = blankTab;
    is(document.title, pb_page_without_title, "The window title for a page without a title matches " +
      "(inside private browsing mode)");
    gBrowser.removeTab(blankTab);

    pageTab = gBrowser.addTab();
    gBrowser.selectedTab = pageTab;
    pageBrowser = gBrowser.getBrowserForTab(pageTab);
    pageBrowser.addEventListener("load", function () {
      pageBrowser.removeEventListener("load", arguments.callee, true);

      
      is(document.title, pb_page_with_title, "The window title for a page with a title matches " +
        "(inside private browsing mode)");

      gBrowser.removeTab(pageTab);

      let aboutPBTab = gBrowser.addTab();
      gBrowser.selectedTab = aboutPBTab;
      let aboutPBBrowser = gBrowser.getBrowserForTab(aboutPBTab);
      aboutPBBrowser.addEventListener("load", function() {
        aboutPBBrowser.removeEventListener("load", arguments.callee, true);

        
        is(document.title, about_pb_title, "The window title for about:privatebrowsing matches " +
          "(inside private browsing mode)");

        gBrowser.removeTab(aboutPBTab);

        
        pb.privateBrowsingEnabled = false;
        prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
        finish();
      }, true);
      aboutPBBrowser.contentWindow.location = "about:privatebrowsing";
    }, true);
    pageBrowser.contentWindow.location = testPageURL;
  }, true);
  pageBrowser.contentWindow.location = testPageURL;
}
