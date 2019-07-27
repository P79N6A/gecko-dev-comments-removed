var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/BrowserUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const baseURL = "http://mochi.test:8888/browser/" +
  "toolkit/components/addoncompat/tests/browser/";

function forEachWindow(f)
{
  let wins = Services.ww.getWindowEnumerator("navigator:browser");
  while (wins.hasMoreElements()) {
    let win = wins.getNext();
    if (win.gBrowser) {
      f(win);
    }
  }
}

function addLoadListener(target, listener)
{
  target.addEventListener("load", function handler(event) {
    target.removeEventListener("load", handler, true);
    return listener(event);
  }, true);
}

let gWin;
let gBrowser;
let ok, is, info;



function testContentWindow()
{
  return new Promise(function(resolve, reject) {
    const url = baseURL + "browser_addonShims_testpage.html";
    let tab = gBrowser.addTab(url);
    gBrowser.selectedTab = tab;
    let browser = tab.linkedBrowser;
    addLoadListener(browser, function handler() {
      ok(gWin.content, "content is defined on chrome window");
      ok(browser.contentWindow, "contentWindow is defined");
      ok(browser.contentDocument, "contentWindow is defined");
      is(gWin.content, browser.contentWindow, "content === contentWindow");

      ok(browser.contentDocument.getElementById("link"), "link present in document");

      
      

      gBrowser.removeTab(tab);
      resolve();
    });
  });
}



function testListeners()
{
  return new Promise(function(resolve, reject) {
    const url1 = baseURL + "browser_addonShims_testpage.html";
    const url2 = baseURL + "browser_addonShims_testpage2.html";

    let tab = gBrowser.addTab(url2);
    let browser = tab.linkedBrowser;
    addLoadListener(browser, function handler() {
      function dummyHandler() {}

      
      
      
      
      for (let i = 0; i < 5; i++) {
        gBrowser.addEventListener("load", dummyHandler, true);
        gBrowser.removeEventListener("load", dummyHandler, true);
      }

      
      
      let loadWithRemoveCount = 0;
      addLoadListener(browser, function handler1(event) {
        loadWithRemoveCount++;
        is(event.target.documentURI, url1, "only fire for first url");
      });

      
      
      
      
      let loadCount = 0;
      browser.addEventListener("load", function handler2(event) {
        loadCount++;
        if (loadCount == 1) {
          is(event.target.documentURI, url1, "first load is for first page loaded");
          browser.loadURI(url2);
        } else {
          gBrowser.removeEventListener("load", handler2, true);

          is(event.target.documentURI, url2, "second load is for second page loaded");
          is(loadWithRemoveCount, 1, "load handler is only called once");

          gBrowser.removeTab(tab);
          resolve();
        }
      }, true);

      browser.loadURI(url1);
    });
  });
}




function testCapturing()
{
  return new Promise(function(resolve, reject) {
    let capturingCount = 0;
    let nonCapturingCount = 0;

    function capturingHandler(event) {
      is(capturingCount, 0, "capturing handler called once");
      is(nonCapturingCount, 0, "capturing handler called before bubbling handler");
      capturingCount++;
    }

    function nonCapturingHandler(event) {
      is(capturingCount, 1, "bubbling handler called after capturing handler");
      is(nonCapturingCount, 0, "bubbling handler called once");
      nonCapturingCount++;
    }

    gBrowser.addEventListener("mousedown", capturingHandler, true);
    gBrowser.addEventListener("mousedown", nonCapturingHandler, false);

    const url = baseURL + "browser_addonShims_testpage.html";
    let tab = gBrowser.addTab(url);
    let browser = tab.linkedBrowser;
    addLoadListener(browser, function handler() {
      let win = browser.contentWindow;
      let event = win.document.createEvent("MouseEvents");
      event.initMouseEvent("mousedown", true, false, win, 1,
                           1, 0, 0, 0, 
                           false, false, false, false, 
                           0, null); 

      let element = win.document.getElementById("output");
      element.dispatchEvent(event);

      is(capturingCount, 1, "capturing handler fired");
      is(nonCapturingCount, 1, "bubbling handler fired");

      gBrowser.removeEventListener("mousedown", capturingHandler, true);
      gBrowser.removeEventListener("mousedown", nonCapturingHandler, false);

      gBrowser.removeTab(tab);
      resolve();
    });
  });
}



function testObserver()
{
  return new Promise(function(resolve, reject) {
    let observerFired = 0;

    function observer(subject, topic, data) {
      Services.obs.removeObserver(observer, "document-element-inserted");
      observerFired++;
    }
    Services.obs.addObserver(observer, "document-element-inserted", false);

    let count = 0;
    const url = baseURL + "browser_addonShims_testpage.html";
    let tab = gBrowser.addTab(url);
    let browser = tab.linkedBrowser;
    browser.addEventListener("load", function handler() {
      count++;
      if (count == 1) {
        browser.reload();
      } else {
        browser.removeEventListener("load", handler);

        is(observerFired, 1, "got observer notification");

        gBrowser.removeTab(tab);
        resolve();
      }
    }, true);
  });
}




function testSandbox()
{
  return new Promise(function(resolve, reject) {
    const url = baseURL + "browser_addonShims_testpage.html";
    let tab = gBrowser.addTab(url);
    let browser = tab.linkedBrowser;
    browser.addEventListener("load", function handler() {
      browser.removeEventListener("load", handler);

      let sandbox = Cu.Sandbox(browser.contentWindow,
                               {sandboxPrototype: browser.contentWindow,
                                wantXrays: false});
      Cu.evalInSandbox("const unsafeWindow = window;", sandbox);
      Cu.evalInSandbox("document.getElementById('output').innerHTML = 'hello';", sandbox);

      is(browser.contentDocument.getElementById("output").innerHTML, "hello",
         "sandbox code ran successfully");

      
      sandbox = Cu.Sandbox([browser.contentWindow],
                           {sandboxPrototype: browser.contentWindow,
                            wantXrays: false});
      Cu.evalInSandbox("const unsafeWindow = window;", sandbox);
      Cu.evalInSandbox("document.getElementById('output').innerHTML = 'hello2';", sandbox);

      is(browser.contentDocument.getElementById("output").innerHTML, "hello2",
         "EP sandbox code ran successfully");

      gBrowser.removeTab(tab);
      resolve();
    }, true);
  });
}



function testAddonContent()
{
  let chromeRegistry = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
    .getService(Components.interfaces.nsIChromeRegistry);
  let base = chromeRegistry.convertChromeURL(BrowserUtils.makeURI("chrome://addonshim1/content/"));

  let res = Services.io.getProtocolHandler("resource")
    .QueryInterface(Ci.nsIResProtocolHandler);
  res.setSubstitution("addonshim1", base);

  return new Promise(function(resolve, reject) {
    const url = "resource://addonshim1/page.html";
    let tab = gBrowser.addTab(url);
    let browser = tab.linkedBrowser;
    addLoadListener(browser, function handler() {
      gBrowser.removeTab(tab);
      res.setSubstitution("addonshim1", null);

      resolve();
    });
  });
}





function testAboutModuleRegistration()
{
  let Registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

  let modulesToUnregister = new Map();

  function TestChannel(uri, aLoadInfo, aboutName) {
    this.aboutName = aboutName;
    this.loadInfo = aLoadInfo;
    this.URI = this.originalURI = uri;
  }

  TestChannel.prototype = {
    asyncOpen: function(listener, context) {
      let stream = this.open();
      let runnable = {
        run: () => {
          try {
            listener.onStartRequest(this, context);
          } catch(e) {}
          try {
            listener.onDataAvailable(this, context, stream, 0, stream.available());
          } catch(e) {}
          try {
            listener.onStopRequest(this, context, Cr.NS_OK);
          } catch(e) {}
        }
      };
      Services.tm.currentThread.dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
    },

    open: function() {
      function getWindow(channel) {
        try
        {
          if (channel.notificationCallbacks)
            return channel.notificationCallbacks.getInterface(Ci.nsILoadContext).associatedWindow;
        } catch(e) {}

        try
        {
          if (channel.loadGroup && channel.loadGroup.notificationCallbacks)
            return channel.loadGroup.notificationCallbacks.getInterface(Ci.nsILoadContext).associatedWindow;
        } catch(e) {}

        return null;
      }

      let data = `<html><h1>${this.aboutName}</h1></html>`;
      let wnd = getWindow(this);
      if (!wnd)
        throw Cr.NS_ERROR_UNEXPECTED;

      let stream = Cc["@mozilla.org/io/string-input-stream;1"].createInstance(Ci.nsIStringInputStream);
      stream.setData(data, data.length);
      return stream;
    },

    isPending: function() {
      return false;
    },
    cancel: function() {
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    },
    suspend: function() {
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    },
    resume: function() {
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIChannel, Ci.nsIRequest])
  };

  













  let createAndRegisterAboutModule = function(aboutName, uuid) {

    let AboutModule = function() {};

    AboutModule.prototype = {
      classID: Components.ID(uuid),
      classDescription: `Testing About Module for about:${aboutName}`,
      contractID: `@mozilla.org/network/protocol/about;1?what=${aboutName}`,
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),

      newChannel: (aURI, aLoadInfo) => {
        return new TestChannel(aURI, aLoadInfo, aboutName);
      },

      getURIFlags: (aURI) => {
        return Ci.nsIAboutModule.URI_SAFE_FOR_UNTRUSTED_CONTENT |
               Ci.nsIAboutModule.ALLOW_SCRIPT;
      },
    };

    let factory = {
      createInstance: function(outer, iid) {
        if (outer) {
          throw Cr.NS_ERROR_NO_AGGREGATION;
        }
        return new AboutModule();
      },
    };

    Registrar.registerFactory(AboutModule.prototype.classID,
                              AboutModule.prototype.classDescription,
                              AboutModule.prototype.contractID,
                              factory);

    modulesToUnregister.set(AboutModule.prototype.classID,
                            factory);
  };

  



  let unregisterModules = () => {
    for (let [classID, factory] of modulesToUnregister) {
      Registrar.unregisterFactory(classID, factory);
    }
  };

  




















  let testAboutModulesWork = (browser) => {
    let testConnection = () => {
      let request = new content.XMLHttpRequest();
      try {
        request.open("GET", "about:test1", false);
        request.send(null);
        if (request.status != 200) {
          throw(`about:test1 response had status ${request.status} - expected 200`);
        }
        if (request.responseText.indexOf("test1") == -1) {
          throw(`about:test1 response had result ${request.responseText}`);
        }

        request = new content.XMLHttpRequest();
        request.open("GET", "about:test2", false);
        request.send(null);

        if (request.status != 200) {
          throw(`about:test2 response had status ${request.status} - expected 200`);
        }
        if (request.responseText.indexOf("test2") == -1) {
          throw(`about:test2 response had result ${request.responseText}`);
        }

        sendAsyncMessage("test:result", {
          pass: true,
        });
      } catch(e) {
        sendAsyncMessage("test:result", {
          pass: false,
          errorMsg: e.toString(),
        });
      }
    };

    return new Promise((resolve, reject) => {
      let mm = browser.messageManager;
      mm.addMessageListener("test:result", function onTestResult(message) {
        mm.removeMessageListener("test:result", onTestResult);
        if (message.data.pass) {
          ok(true, "Connections to about: pages were successful");
        } else {
          ok(false, message.data.errorMsg);
        }
        resolve();
      });
      mm.loadFrameScript("data:,(" + testConnection.toString() + ")();", false);
    });
  }

  
  return new Promise((resolve, reject) => {
    createAndRegisterAboutModule("test1", "5f3a921b-250f-4ac5-a61c-8f79372e6063");
    createAndRegisterAboutModule("test2", "d7ec0389-1d49-40fa-b55c-a1fc3a6dbf6f");

    
    
    
    
    let newTab = gBrowser.addTab("chrome://addonshim1/content/page.html");
    gBrowser.selectedTab = newTab;
    let browser = newTab.linkedBrowser;

    addLoadListener(browser, function() {
      testAboutModulesWork(browser).then(() => {
        gBrowser.removeTab(newTab);
        unregisterModules();
        resolve();
      });
    });
  });
}

function runTests(win, funcs)
{
  ok = funcs.ok;
  is = funcs.is;
  info = funcs.info;

  gWin = win;
  gBrowser = win.gBrowser;

  return testContentWindow().
    then(testListeners).
    then(testCapturing).
    then(testObserver).
    then(testSandbox).
    then(testAddonContent).
    then(testAboutModuleRegistration);
}





function startup(aData, aReason)
{
  forEachWindow(win => {
    win.runAddonShimTests = (funcs) => runTests(win, funcs);
  });
}

function shutdown(aData, aReason)
{
  forEachWindow(win => {
    delete win.runAddonShimTests;
  });
}

function install(aData, aReason)
{
}

function uninstall(aData, aReason)
{
}

