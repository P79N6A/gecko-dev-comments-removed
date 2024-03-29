



const TEST_MSG = "ContentSearchTest";
const CONTENT_SEARCH_MSG = "ContentSearch";
const TEST_CONTENT_SCRIPT_BASENAME = "contentSearch.js";



const SUGGESTIONS_TIMEOUT = 10000;

var gMsgMan;

add_task(function* GetState() {
  yield addTab();
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "GetState",
  });
  let msg = yield waitForTestMsg("State");
  checkMsg(msg, {
    type: "State",
    data: yield currentStateObj(),
  });
});

add_task(function* SetCurrentEngine() {
  yield addTab();
  let newCurrentEngine = null;
  let oldCurrentEngine = Services.search.currentEngine;
  let engines = Services.search.getVisibleEngines();
  for (let engine of engines) {
    if (engine != oldCurrentEngine) {
      newCurrentEngine = engine;
      break;
    }
  }
  if (!newCurrentEngine) {
    info("Couldn't find a non-selected search engine, " +
         "skipping this part of the test");
    return;
  }
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "SetCurrentEngine",
    data: newCurrentEngine.name,
  });
  let deferred = Promise.defer();
  Services.obs.addObserver(function obs(subj, topic, data) {
    info("Test observed " + data);
    if (data == "engine-current") {
      ok(true, "Test observed engine-current");
      Services.obs.removeObserver(obs, "browser-search-engine-modified");
      deferred.resolve();
    }
  }, "browser-search-engine-modified", false);
  let searchPromise = waitForTestMsg("CurrentEngine");
  info("Waiting for test to observe engine-current...");
  yield deferred.promise;
  let msg = yield searchPromise;
  checkMsg(msg, {
    type: "CurrentEngine",
    data: yield currentEngineObj(newCurrentEngine),
  });

  Services.search.currentEngine = oldCurrentEngine;
  msg = yield waitForTestMsg("CurrentEngine");
  checkMsg(msg, {
    type: "CurrentEngine",
    data: yield currentEngineObj(oldCurrentEngine),
  });
});

add_task(function* modifyEngine() {
  yield addTab();
  let engine = Services.search.currentEngine;
  let oldAlias = engine.alias;
  engine.alias = "ContentSearchTest";
  let msg = yield waitForTestMsg("CurrentState");
  checkMsg(msg, {
    type: "CurrentState",
    data: yield currentStateObj(),
  });
  engine.alias = oldAlias;
  msg = yield waitForTestMsg("CurrentState");
  checkMsg(msg, {
    type: "CurrentState",
    data: yield currentStateObj(),
  });
});

add_task(function* search() {
  yield addTab();
  let engine = Services.search.currentEngine;
  let data = {
    engineName: engine.name,
    searchString: "ContentSearchTest",
    healthReportKey: "ContentSearchTest",
    searchPurpose: "ContentSearchTest",
  };
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "Search",
    data: data,
  });
  let submissionURL =
    engine.getSubmission(data.searchString, "", data.whence).uri.spec;
  yield waitForLoadAndStopIt(gBrowser.selectedBrowser, submissionURL);
});

add_task(function* searchInBackgroundTab() {
  
  
  
  
  yield addTab();
  let searchBrowser = gBrowser.selectedBrowser;
  let engine = Services.search.currentEngine;
  let data = {
    engineName: engine.name,
    searchString: "ContentSearchTest",
    healthReportKey: "ContentSearchTest",
    searchPurpose: "ContentSearchTest",
  };
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "Search",
    data: data,
  });

  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  registerCleanupFunction(() => gBrowser.removeTab(newTab));

  let submissionURL =
    engine.getSubmission(data.searchString, "", data.whence).uri.spec;
  yield waitForLoadAndStopIt(searchBrowser, submissionURL);
});

add_task(function* badImage() {
  yield addTab();
  
  
  
  let vals = yield waitForNewEngine("contentSearchBadImage.xml", 1);
  let engine = vals[0];
  let finalCurrentStateMsg = vals[vals.length - 1];
  let expectedCurrentState = yield currentStateObj();
  let expectedEngine =
    expectedCurrentState.engines.find(e => e.name == engine.name);
  ok(!!expectedEngine, "Sanity check: engine should be in expected state");
  ok(expectedEngine.iconBuffer === null,
     "Sanity check: icon array buffer of engine in expected state " +
     "should be null: " + expectedEngine.iconBuffer);
  checkMsg(finalCurrentStateMsg, {
    type: "CurrentState",
    data: expectedCurrentState,
  });
  
  
  Services.search.removeEngine(engine);
  yield waitForTestMsg("CurrentState");
});

add_task(function* GetSuggestions_AddFormHistoryEntry_RemoveFormHistoryEntry() {
  yield addTab();

  
  let vals = yield waitForNewEngine("contentSearchSuggestions.xml", 0);
  let engine = vals[0];

  let searchStr = "browser_ContentSearch.js-suggestions-";

  
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "AddFormHistoryEntry",
    data: searchStr + "form",
  });
  let deferred = Promise.defer();
  Services.obs.addObserver(function onAdd(subj, topic, data) {
    if (data == "formhistory-add") {
      Services.obs.removeObserver(onAdd, "satchel-storage-changed");
      executeSoon(() => deferred.resolve());
    }
  }, "satchel-storage-changed", false);
  yield deferred.promise;

  
  
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "GetSuggestions",
    data: {
      engineName: engine.name,
      searchString: searchStr,
      remoteTimeout: SUGGESTIONS_TIMEOUT,
    },
  });

  
  let msg = yield waitForTestMsg("Suggestions");
  checkMsg(msg, {
    type: "Suggestions",
    data: {
      engineName: engine.name,
      searchString: searchStr,
      formHistory: [searchStr + "form"],
      remote: [searchStr + "foo", searchStr + "bar"],
    },
  });

  
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "RemoveFormHistoryEntry",
    data: searchStr + "form",
  });
  deferred = Promise.defer();
  Services.obs.addObserver(function onRemove(subj, topic, data) {
    if (data == "formhistory-remove") {
      Services.obs.removeObserver(onRemove, "satchel-storage-changed");
      executeSoon(() => deferred.resolve());
    }
  }, "satchel-storage-changed", false);
  yield deferred.promise;

  
  gMsgMan.sendAsyncMessage(TEST_MSG, {
    type: "GetSuggestions",
    data: {
      engineName: engine.name,
      searchString: searchStr,
      remoteTimeout: SUGGESTIONS_TIMEOUT,
    },
  });

  
  msg = yield waitForTestMsg("Suggestions");
  checkMsg(msg, {
    type: "Suggestions",
    data: {
      engineName: engine.name,
      searchString: searchStr,
      formHistory: [],
      remote: [searchStr + "foo", searchStr + "bar"],
    },
  });

  
  Services.search.removeEngine(engine);
  yield waitForTestMsg("CurrentState");
});

function buffersEqual(actualArrayBuffer, expectedArrayBuffer) {
  let expectedView = new Int8Array(expectedArrayBuffer);
  let actualView = new Int8Array(actualArrayBuffer);
  for (let i = 0; i < expectedView.length; i++) {
    if (actualView[i] != expectedView[i]) {
      return false;
    }
  }
  return true;
}

function arrayBufferEqual(actualArrayBuffer, expectedArrayBuffer) {
  ok(actualArrayBuffer instanceof ArrayBuffer, "Actual value is ArrayBuffer.");
  ok(expectedArrayBuffer instanceof ArrayBuffer, "Expected value is ArrayBuffer.");
  Assert.equal(actualArrayBuffer.byteLength, expectedArrayBuffer.byteLength,
      "Array buffers have the same length.");
  ok(buffersEqual(actualArrayBuffer, expectedArrayBuffer), "Buffers are equal.");
}

function checkArrayBuffers(actual, expected) {
  if (actual instanceof ArrayBuffer) {
    arrayBufferEqual(actual, expected);
  }
  if (typeof actual == "object") {
    for (let i in actual) {
      checkArrayBuffers(actual[i], expected[i]);
    }
  }
}

function checkMsg(actualMsg, expectedMsgData) {
  let actualMsgData = actualMsg.data;
  SimpleTest.isDeeply(actualMsg.data, expectedMsgData, "Checking message");

  
  
  checkArrayBuffers(actualMsgData, expectedMsgData);
}

function waitForMsg(name, type) {
  let deferred = Promise.defer();
  info("Waiting for " + name + " message " + type + "...");
  gMsgMan.addMessageListener(name, function onMsg(msg) {
    info("Received " + name + " message " + msg.data.type + "\n");
    if (msg.data.type == type) {
      gMsgMan.removeMessageListener(name, onMsg);
      deferred.resolve(msg);
    }
  });
  return deferred.promise;
}

function waitForTestMsg(type) {
  return waitForMsg(TEST_MSG, type);
}

function waitForNewEngine(basename, numImages) {
  info("Waiting for engine to be added: " + basename);

  
  
  let expectedSearchEvents = ["CurrentState", "CurrentState"];
  
  for (let i = 0; i < numImages; i++) {
    expectedSearchEvents.push("CurrentState");
  }
  let eventPromises = expectedSearchEvents.map(e => waitForTestMsg(e));

  
  let addDeferred = Promise.defer();
  let url = getRootDirectory(gTestPath) + basename;
  Services.search.addEngine(url, Ci.nsISearchEngine.TYPE_MOZSEARCH, "", false, {
    onSuccess: function (engine) {
      info("Search engine added: " + basename);
      addDeferred.resolve(engine);
    },
    onError: function (errCode) {
      ok(false, "addEngine failed with error code " + errCode);
      addDeferred.reject();
    },
  });

  return Promise.all([addDeferred.promise].concat(eventPromises));
}

function waitForLoadAndStopIt(browser, expectedURL) {
  let deferred = Promise.defer();
  let listener = {
    onStateChange: function (webProg, req, flags, status) {
      if (req instanceof Ci.nsIChannel) {
        let url = req.originalURI.spec;
        info("onStateChange " + url);
        let docStart = Ci.nsIWebProgressListener.STATE_IS_DOCUMENT |
                       Ci.nsIWebProgressListener.STATE_START;
        if ((flags & docStart) && webProg.isTopLevel && url == expectedURL) {
          browser.removeProgressListener(listener);
          ok(true, "Expected URL loaded");
          req.cancel(Components.results.NS_ERROR_FAILURE);
          deferred.resolve();
        }
      }
    },
    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsIWebProgressListener,
      Ci.nsISupportsWeakReference,
    ]),
  };
  browser.addProgressListener(listener);
  info("Waiting for URL to load: " + expectedURL);
  return deferred.promise;
}

function addTab() {
  let deferred = Promise.defer();
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  tab.linkedBrowser.addEventListener("load", function load() {
    tab.linkedBrowser.removeEventListener("load", load, true);
    let url = getRootDirectory(gTestPath) + TEST_CONTENT_SCRIPT_BASENAME;
    gMsgMan = tab.linkedBrowser.messageManager;
    gMsgMan.sendAsyncMessage(CONTENT_SEARCH_MSG, {
      type: "AddToWhitelist",
      data: ["about:blank"],
    });
    waitForMsg(CONTENT_SEARCH_MSG, "AddToWhitelistAck").then(() => {
      gMsgMan.loadFrameScript(url, false);
      deferred.resolve();
    });
  }, true);
  registerCleanupFunction(() => gBrowser.removeTab(tab));
  return deferred.promise;
}

let currentStateObj = Task.async(function* () {
  let state = {
    engines: [],
    currentEngine: yield currentEngineObj(),
  };
  for (let engine of Services.search.getVisibleEngines()) {
    let uri = engine.getIconURLBySize(16, 16);
    state.engines.push({
      name: engine.name,
      iconBuffer: yield arrayBufferFromDataURI(uri),
    });
  }
  return state;
});

let currentEngineObj = Task.async(function* () {
  let engine = Services.search.currentEngine;
  let uri1x = engine.getIconURLBySize(65, 26);
  let uri2x = engine.getIconURLBySize(130, 52);
  let uriFavicon = engine.getIconURLBySize(16, 16);
  let bundle = Services.strings.createBundle("chrome://global/locale/autocomplete.properties");
  return {
    name: engine.name,
    placeholder: bundle.formatStringFromName("searchWithEngine", [engine.name], 1),
    logoBuffer: yield arrayBufferFromDataURI(uri1x),
    logo2xBuffer: yield arrayBufferFromDataURI(uri2x),
    iconBuffer: yield arrayBufferFromDataURI(uriFavicon),
  };
});

function arrayBufferFromDataURI(uri) {
  if (!uri) {
    return Promise.resolve(null);
  }
  let deferred = Promise.defer();
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
            createInstance(Ci.nsIXMLHttpRequest);
  xhr.open("GET", uri, true);
  xhr.responseType = "arraybuffer";
  xhr.onloadend = () => {
    deferred.resolve(xhr.response);
  };
  try {
    xhr.send();
  }
  catch (err) {
    return Promise.resolve(null);
  }
  return deferred.promise;
}
