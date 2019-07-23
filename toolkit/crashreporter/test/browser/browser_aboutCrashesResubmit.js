
var scriptLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                             .getService(Components.interfaces.mozIJSSubScriptLoader);
scriptLoader.loadSubScript("chrome://mochikit/content/browser/toolkit/crashreporter/test/browser/aboutcrashes_utils.js", this);

function cleanup_and_finish() {
  try {
    cleanup_fake_appdir();
  } catch(ex) {}
  let prefs = Components.classes["@mozilla.org/preferences-service;1"]
    .getService(Components.interfaces.nsIPrefService);
  prefs.clearUserPref("breakpad.reportURL");
  gBrowser.removeTab(gBrowser.selectedTab);
  finish();
}







function check_crash_list(tab, crashes) {
  let doc = gBrowser.getBrowserForTab(tab).contentDocument;
  let crashlinks = doc.getElementById("tbody").getElementsByTagName("a");
  is(crashlinks.length, crashes.length,
    "about:crashes lists correct number of crash reports");
  
  if (crashlinks.length == crashes.length) {
    for(let i=0; i<crashes.length; i++) {
      is(crashlinks[i].id, crashes[i].id, i + ": crash ID is correct");
      if (crashes[i].pending) {
        
        is(crashlinks[i].getAttribute("href"),
          "http://example.com/browser/toolkit/crashreporter/about/throttling",
          "pending URL links to the correct static page");
      }
    }
  }
}









function check_submit_pending(tab, crashes) {
  let browser = gBrowser.getBrowserForTab(tab);
  let SubmittedCrash = null;
  let CrashID = null;
  let CrashURL = null;
  function csp_onload() {
    if (browser.contentWindow.location != 'about:crashes') {
      browser.removeEventListener("load", csp_onload, true);
      
      ok(true, 'got submission onload');
      
      CrashID = browser.contentWindow.location.search.split("=")[1];
      CrashURL = browser.contentWindow.location.toString();
      
      let result = JSON.parse(browser.contentDocument.documentElement.textContent);
      is(result.upload_file_minidump, "MDMP", "minidump file sent properly");
      is(result.Throttleable, 0, "correctly sent as non-throttleable");
      
      
      delete result.upload_file_minidump;
      delete result.Throttleable;
      
      delete SubmittedCrash.extra.ServerURL;

      for(let x in result) {
        if (x in SubmittedCrash.extra)
          is(result[x], SubmittedCrash.extra[x],
             "submitted value for " + x + " matches expected");
        else
          ok(false, "property " + x + " missing from submitted data!");
      }
      for(let y in SubmittedCrash.extra) {
        if (!(y in result))
          ok(false, "property " + y + " missing from result data!");
      }
      executeSoon(function() {
                    browser.addEventListener("pageshow", csp_pageshow, true);
                    
                    browser.goBack();
                  });
    }
  }
  browser.addEventListener("load", csp_onload, true);
  function csp_pageshow() {
    browser.removeEventListener("pageshow", csp_pageshow, true);
    executeSoon(function () {
                  is(browser.contentWindow.location, "about:crashes", "navigated back successfully");
                  let link = browser.contentDocument.getElementById(CrashID);
                  isnot(link, null, "crash report link changed correctly");
                  if (link)
                    is(link.href, CrashURL, "crash report link points to correct href");
                  cleanup_and_finish();
                });
  }

  
  for each(let crash in crashes) {
    if (crash.pending) {
      SubmittedCrash = crash;
      break;
    }
  }
  EventUtils.sendMouseEvent({type:'click'}, SubmittedCrash.id,
                            browser.contentWindow);
}

function test() {
  waitForExplicitFinish();
  let appD = make_fake_appdir();
  let crD = appD.clone();
  crD.append("Crash Reports");
  let crashes = add_fake_crashes(crD, 1);
  
  crashes.push(addPendingCrashreport(crD, {'ServerURL': 'http://example.com/browser/toolkit/crashreporter/test/browser/crashreport.sjs',
                                           'ProductName': 'Test App'
                                          }));
  crashes.sort(function(a,b) b.date - a.date);

  
  let prefs = Components.classes["@mozilla.org/preferences-service;1"]
    .getService(Components.interfaces.nsIPrefService);

  prefs.setCharPref("breakpad.reportURL", "http://example.com/browser/toolkit/crashreporter/test/browser/crashreport.sjs?id=");

  let tab = gBrowser.selectedTab = gBrowser.addTab("about:blank");
  let browser = gBrowser.getBrowserForTab(tab);
  browser.addEventListener("load", function test_load() {
                             browser.removeEventListener("load", test_load, true);
                             executeSoon(function () {
                                           check_crash_list(tab, crashes);
                                           check_submit_pending(tab, crashes);
                                         });
                          }, true);
  browser.loadURI("about:crashes", null, null);
}
