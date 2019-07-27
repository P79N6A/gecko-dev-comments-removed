





"use strict";


requestLongerTimeout(2);









function crashBrowser(browser) {
  let kv = {};
  Cu.import("resource://gre/modules/KeyValueParser.jsm", kv);
  
  
  
  
  let frame_script = () => {
    const Cu = Components.utils;
    Cu.import("resource://gre/modules/ctypes.jsm");

    let dies = function() {
      privateNoteIntentionalCrash();
      let zero = new ctypes.intptr_t(8);
      let badptr = ctypes.cast(zero, ctypes.PointerType(ctypes.int32_t));
      let crash = badptr.contents;
    };

    dump("Et tu, Brute?");
    dies();
  };

  function checkSubject(subject, data) {
    return subject instanceof Ci.nsIPropertyBag2 &&
      subject.hasKey("abnormal");
  };
  let crashPromise = TestUtils.topicObserved('ipc:content-shutdown',
                                             checkSubject);
  let crashDataPromise = crashPromise.then(([subject, data]) => {
    ok(subject instanceof Ci.nsIPropertyBag2);

    let dumpID;
    if ('nsICrashReporter' in Ci) {
      dumpID = subject.getPropertyAsAString('dumpID');
      ok(dumpID, "dumpID is present and not an empty string");
    }

    let extra = null;
    if (dumpID) {
      let minidumpDirectory = getMinidumpDirectory();
      let extrafile = minidumpDirectory.clone();
      extrafile.append(dumpID + '.extra');
      ok(extrafile.exists(), 'found .extra file');
      extra = kv.parseKeyValuePairsFromFile(extrafile);
      removeFile(minidumpDirectory, dumpID + '.dmp');
      removeFile(minidumpDirectory, dumpID + '.extra');
    }

    return extra;
  });

  
  
  let mm = browser.messageManager;
  mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", false);
  return crashDataPromise;
}










function removeFile(directory, filename) {
  let file = directory.clone();
  file.append(filename);
  if (file.exists()) {
    file.remove(false);
  }
}






function getMinidumpDirectory() {
  let dir = Services.dirsvc.get('ProfD', Ci.nsIFile);
  dir.append("minidumps");
  return dir;
}




add_task(function* test_content_url_annotation() {
  let url = "https://example.com/browser/toolkit/content/tests/browser/file_redirect.html";
  let redirect_url = "https://example.com/browser/toolkit/content/tests/browser/file_redirect_to.html";

  yield BrowserTestUtils.withNewTab({
    gBrowser: gBrowser
  }, function* (browser) {
    ok(browser.isRemoteBrowser, "Should be a remote browser");

    
    let promise = ContentTask.spawn(browser, {}, function* () {
      dump('ContentTask starting...\n');
      yield new Promise((resolve) => {
        addEventListener("RedirectDone", function listener() {
          dump('Got RedirectDone\n');
          removeEventListener("RedirectDone", listener);
          resolve();
        }, true, true);
      });
    });
    browser.loadURI(url);
    yield promise;

    
    let annotations = yield crashBrowser(browser);

    ok("URL" in annotations, "annotated a URL");
    is(annotations.URL, redirect_url,
       "Should have annotated the URL after redirect");
  });
});
