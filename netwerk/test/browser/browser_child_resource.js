





const TEST_URL = "http://example.com/browser/netwerk/test/browser/dummy.html";

const expectedRemote = gMultiProcessBrowser ? "true" : "";

Components.utils.import("resource://gre/modules/Services.jsm");
const resProtocol = Cc["@mozilla.org/network/protocol;1?name=resource"]
                        .getService(Ci.nsIResProtocolHandler);

function getMinidumpDirectory() {
  var dir = Services.dirsvc.get('ProfD', Ci.nsIFile);
  dir.append("minidumps");
  return dir;
}



let CrashObserver = {
  observe: function(subject, topic, data) {
    is(topic, 'ipc:content-shutdown', 'Received correct observer topic.');
    ok(subject instanceof Ci.nsIPropertyBag2,
       'Subject implements nsIPropertyBag2.');
    
    
    if (!subject.hasKey("abnormal")) {
      info("This is a normal termination and isn't the one we are looking for...");
      return;
    }

    var dumpID;
    if ('nsICrashReporter' in Ci) {
      dumpID = subject.getPropertyAsAString('dumpID');
      ok(dumpID, "dumpID is present and not an empty string");
    }

    if (dumpID) {
      var minidumpDirectory = getMinidumpDirectory();
      let file = minidumpDirectory.clone();
      file.append(dumpID + '.dmp');
      file.remove(true);
      file = minidumpDirectory.clone();
      file.append(dumpID + '.extra');
      file.remove(true);
    }
  }
}
Services.obs.addObserver(CrashObserver, 'ipc:content-shutdown', false);

registerCleanupFunction(() => {
  Services.obs.removeObserver(CrashObserver, 'ipc:content-shutdown');
});

function frameScript() {
  Components.utils.import("resource://gre/modules/Services.jsm");
  let resProtocol = Components.classes["@mozilla.org/network/protocol;1?name=resource"]
                              .getService(Components.interfaces.nsIResProtocolHandler);

  addMessageListener("Test:ResolveURI", function({ data: uri }) {
    uri = Services.io.newURI(uri, null, null);
    try {
      let resolved = resProtocol.resolveURI(uri);
      sendAsyncMessage("Test:ResolvedURI", resolved);
    }
    catch (e) {
      sendAsyncMessage("Test:ResolvedURI", null);
    }
  });

  addMessageListener("Test:Crash", function() {
    dump("Crashing\n");
    privateNoteIntentionalCrash();
    Components.utils.import("resource://gre/modules/ctypes.jsm");
    let zero = new ctypes.intptr_t(8);
    let badptr = ctypes.cast(zero, ctypes.PointerType(ctypes.int32_t));
    badptr.contents
  });
}

function waitForEvent(obj, name, capturing, chromeEvent) {
  info("Waiting for " + name);
  return new Promise((resolve) => {
    function listener(event) {
      info("Saw " + name);
      obj.removeEventListener(name, listener, capturing, chromeEvent);
      resolve(event);
    }

    obj.addEventListener(name, listener, capturing, chromeEvent);
  });
}

function resolveURI(uri) {
  uri = Services.io.newURI(uri, null, null);
  try {
    return resProtocol.resolveURI(uri);
  }
  catch (e) {
    return null;
  }
}

function remoteResolveURI(uri) {
  return new Promise((resolve) => {
    let manager = gBrowser.selectedBrowser.messageManager;

    function listener({ data: resolved }) {
      manager.removeMessageListener("Test:ResolvedURI", listener);
      resolve(resolved);
    }

    manager.addMessageListener("Test:ResolvedURI", listener);
    manager.sendAsyncMessage("Test:ResolveURI", uri);
  });
}

let loadTestTab = Task.async(function*() {
  gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
  let browser = gBrowser.selectedBrowser;
  yield waitForEvent(browser, "load", true);
  browser.messageManager.loadFrameScript("data:,(" + frameScript.toString() + ")();", true);
  return browser;
});


let restart = Task.async(function*() {
  let browser = gBrowser.selectedBrowser;
  
  if (browser.getAttribute("remote") != "true")
    return browser;

  browser.messageManager.sendAsyncMessage("Test:Crash");
  yield waitForEvent(browser, "AboutTabCrashedLoad", false, true);

  browser.reload();

  yield BrowserTestUtils.browserLoaded(browser);
  is(browser.getAttribute("remote"), expectedRemote, "Browser should be in the right process");
  browser.messageManager.loadFrameScript("data:,(" + frameScript.toString() + ")();", true);
  return browser;
});


add_task(function*() {
  let browser = yield loadTestTab();

  
  is(browser.getAttribute("remote"), expectedRemote, "Browser should be in the right process");

  let local = resolveURI("resource://gre/modules/Services.jsm");
  let remote = yield remoteResolveURI("resource://gre/modules/Services.jsm");
  is(local, remote, "Services.jsm should resolve in both processes");

  gBrowser.removeCurrentTab();
});


add_task(function*() {
  let browser = yield loadTestTab();

  info("Set");
  resProtocol.setSubstitution("testing", Services.io.newURI("chrome://global/content", null, null));
  let local = resolveURI("resource://testing/test.js");
  let remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, "chrome://global/content/test.js", "Should resolve in main process");
  is(remote, "chrome://global/content/test.js", "Should resolve in child process");

  info("Change");
  resProtocol.setSubstitution("testing", Services.io.newURI("chrome://global/skin", null, null));
  local = resolveURI("resource://testing/test.js");
  remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, "chrome://global/skin/test.js", "Should resolve in main process");
  is(remote, "chrome://global/skin/test.js", "Should resolve in child process");

  info("Clear");
  resProtocol.setSubstitution("testing", null);
  local = resolveURI("resource://testing/test.js");
  remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, null, "Shouldn't resolve in main process");
  is(remote, null, "Shouldn't resolve in child process");

  gBrowser.removeCurrentTab();
});


add_task(function*() {
  let browser = yield loadTestTab();

  info("Set");
  resProtocol.setSubstitution("testing", Services.io.newURI("chrome://global/content", null, null));
  let local = resolveURI("resource://testing/test.js");
  let remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, "chrome://global/content/test.js", "Should resolve in main process");
  is(remote, "chrome://global/content/test.js", "Should resolve in child process");

  yield restart();

  local = resolveURI("resource://testing/test.js");
  remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, "chrome://global/content/test.js", "Should resolve in main process");
  is(remote, "chrome://global/content/test.js", "Should resolve in child process");

  info("Change");
  resProtocol.setSubstitution("testing", Services.io.newURI("chrome://global/skin", null, null));

  yield restart();

  local = resolveURI("resource://testing/test.js");
  remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, "chrome://global/skin/test.js", "Should resolve in main process");
  is(remote, "chrome://global/skin/test.js", "Should resolve in child process");

  info("Clear");
  resProtocol.setSubstitution("testing", null);

  yield restart();

  local = resolveURI("resource://testing/test.js");
  remote = yield remoteResolveURI("resource://testing/test.js");
  is(local, null, "Shouldn't resolve in main process");
  is(remote, null, "Shouldn't resolve in child process");

  gBrowser.removeCurrentTab();
});


add_task(function*() {
  let browser = yield loadTestTab();

  info("Set");
  resProtocol.setSubstitution("testing", Services.io.newURI("chrome://global/content", null, null));
  resProtocol.setSubstitution("testing2", Services.io.newURI("resource://testing", null, null));
  let local = resolveURI("resource://testing2/test.js");
  let remote = yield remoteResolveURI("resource://testing2/test.js");
  is(local, "chrome://global/content/test.js", "Should resolve in main process");
  is(remote, "chrome://global/content/test.js", "Should resolve in child process");

  info("Clear");
  resProtocol.setSubstitution("testing", null);
  local = resolveURI("resource://testing2/test.js");
  remote = yield remoteResolveURI("resource://testing2/test.js");
  is(local, "chrome://global/content/test.js", "Should resolve in main process");
  is(remote, "chrome://global/content/test.js", "Should resolve in child process");

  resProtocol.setSubstitution("testing2", null);
  local = resolveURI("resource://testing2/test.js");
  remote = yield remoteResolveURI("resource://testing2/test.js");
  is(local, null, "Shouldn't resolve in main process");
  is(remote, null, "Shouldn't resolve in child process");

  gBrowser.removeCurrentTab();
});
