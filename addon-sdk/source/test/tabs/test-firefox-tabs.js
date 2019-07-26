


'use strict';

const { Cc, Ci } = require('chrome');
const { Loader } = require('sdk/test/loader');
const timer = require('sdk/timers');
const { StringBundle } = require('sdk/deprecated/app-strings');


exports.testActiveTab_getter = function(test) {
  test.waitUntilDone();

  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");

    let url = "data:text/html;charset=utf-8,<html><head><title>foo</title></head></html>";
    require("sdk/deprecated/tab-browser").addTab(
      url,
      {
        onLoad: function(e) {
          test.assert(tabs.activeTab);
          test.assertEqual(tabs.activeTab.url, url);
          test.assertEqual(tabs.activeTab.title, "foo");
          closeBrowserWindow(window, function() test.done());
        }
      }
    );
  });
};


exports.testBug682681_aboutURI = function(test) {
  test.waitUntilDone();

  let tabStrings = StringBundle('chrome://browser/locale/tabbrowser.properties');

  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");

    tabs.on('ready', function onReady(tab) {
      tabs.removeListener('ready', onReady);

      test.assertEqual(tab.title,
                       tabStrings.get('tabs.emptyTabTitle'),
                       "title of about: tab is not blank");

      
      closeBrowserWindow(window, function() test.done());
    });

    
    tabs.open({
      url: "about:blank",
      inBackground: true
    });
  });
};


exports.testTitleForDataURI = function(test) {
  test.waitUntilDone();

  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");

    tabs.on('ready', function onReady(tab) {
      tabs.removeListener('ready', onReady);

      test.assertEqual(tab.title, "tab", "data: title is not Connecting...");

      
      closeBrowserWindow(window, function() test.done());
    });

    
    tabs.open({
      url: "data:text/html;charset=utf-8,<title>tab</title>",
      inBackground: true
    });
  });
};



exports.testBrowserWindowCreationOnActivate = function(test) {
  test.waitUntilDone();

  let windows = require("sdk/windows").browserWindows;
  let tabs = require("sdk/tabs");

  let gotActivate = false;

  tabs.once('activate', function onActivate(eventTab) {
    test.assert(windows.activeWindow, "Is able to fetch activeWindow");
    gotActivate = true;
  });

  openBrowserWindow(function(window, browser) {
    test.assert(gotActivate, "Received activate event before openBrowserWindow's callback is called");
    closeBrowserWindow(window, function () test.done());
  });
}


exports.testActiveTab_setter = function(test) {
  test.waitUntilDone();

  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,<html><head><title>foo</title></head></html>";

    tabs.on('ready', function onReady(tab) {
      tabs.removeListener('ready', onReady);
      test.assertEqual(tabs.activeTab.url, "about:blank", "activeTab url has not changed");
      test.assertEqual(tab.url, url, "url of new background tab matches");
      tabs.on('activate', function onActivate(eventTab) {
        tabs.removeListener('activate', onActivate);
        test.assertEqual(tabs.activeTab.url, url, "url after activeTab setter matches");
        test.assertEqual(eventTab, tab, "event argument is the activated tab");
        test.assertEqual(eventTab, tabs.activeTab, "the tab is the active one");
        closeBrowserWindow(window, function() test.done());
      });
      tab.activate();
    })

    tabs.open({
      url: url,
      inBackground: true
    });
  });
};


exports.testAutomaticDestroy = function(test) {
  test.waitUntilDone();

  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");

    
    let called = false;

    let loader = Loader(module);
    let tabs2 = loader.require("sdk/tabs");
    tabs2.on('open', function onOpen(tab) {
      called = true;
    });

    loader.unload();

    
    tabs.once('open', function () {
      timer.setTimeout(function () {
        test.assert(!called, "Unloaded tab module is destroyed and inactive");
        closeBrowserWindow(window, function() test.done());
      }, 0);
    });

    tabs.open("data:text/html;charset=utf-8,foo");
  });
};


exports.testTabProperties = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs= require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,<html><head><title>foo</title></head><body>foo</body></html>";
    tabs.open({
      url: url,
      onReady: function(tab) {
        test.assertEqual(tab.title, "foo", "title of the new tab matches");
        test.assertEqual(tab.url, url, "URL of the new tab matches");
        test.assert(tab.favicon, "favicon of the new tab is not empty");
        test.assertEqual(tab.style, null, "style of the new tab matches");
        test.assertEqual(tab.index, 1, "index of the new tab matches");
        test.assertNotEqual(tab.getThumbnail(), null, "thumbnail of the new tab matches");
        test.assertNotEqual(tab.id, null, "a tab object always has an id property.");
        closeBrowserWindow(window, function() test.done());
      }
    });
  });
};


exports.testTabContentTypeAndReload = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,<html><head><title>foo</title></head><body>foo</body></html>";
    let urlXML = "data:text/xml;charset=utf-8,<foo>bar</foo>";
    tabs.open({
      url: url,
      onReady: function(tab) {
        if (tab.url === url) {
          test.assertEqual(tab.contentType, "text/html");
          tab.url = urlXML;
        } else {
          test.assertEqual(tab.contentType, "text/xml");
          closeBrowserWindow(window, function() test.done());
        }
      }
    });
  });
};


exports.testTabsIteratorAndLength = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let startCount = 0;
    for each (let t in tabs) startCount++;
    test.assertEqual(startCount, tabs.length, "length property is correct");
    let url = "data:text/html;charset=utf-8,default";
    tabs.open(url);
    tabs.open(url);
    tabs.open({
      url: url,
      onOpen: function(tab) {
        let count = 0;
        for each (let t in tabs) count++;
        test.assertEqual(count, startCount + 3, "iterated tab count matches");
        test.assertEqual(startCount + 3, tabs.length, "iterated tab count matches length property");
        closeBrowserWindow(window, function() test.done());
      }
    });
  });
};


exports.testTabLocation = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url1 = "data:text/html;charset=utf-8,foo";
    let url2 = "data:text/html;charset=utf-8,bar";

    tabs.on('ready', function onReady(tab) {
      if (tab.url != url2)
        return;
      tabs.removeListener('ready', onReady);
      test.pass("tab.load() loaded the correct url");
      closeBrowserWindow(window, function() test.done());
    });

    tabs.open({
      url: url1,
      onOpen: function(tab) {
        tab.url = url2
      }
    });
  });
};


exports.testTabClose = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,foo";

    test.assertNotEqual(tabs.activeTab.url, url, "tab is not the active tab");
    tabs.on('ready', function onReady(tab) {
      tabs.removeListener('ready', onReady);
      test.assertEqual(tabs.activeTab.url, tab.url, "tab is now the active tab");
      tab.close(function() {
        closeBrowserWindow(window, function() {
          test.assertNotEqual(tabs.activeTab.url, url, "tab is no longer the active tab");
          test.done()
        });
      });
      test.assertNotEqual(tabs.activeTab.url, url, "tab is no longer the active tab");
    });

    tabs.open(url);
  });
};


exports.testTabReload = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,<!doctype%20html><title></title>";

    tabs.open({ url: url, onReady: function onReady(tab) {
      tab.removeListener("ready", onReady);

      browser.addEventListener(
        "load",
        function onLoad() {
          browser.removeEventListener("load", onLoad, true);

          browser.addEventListener(
            "load",
            function onReload() {
              browser.removeEventListener("load", onReload, true);
              test.pass("the tab was loaded again");
              test.assertEqual(tab.url, url, "the tab has the same URL");
              closeBrowserWindow(window, function() test.done());
            },
            true
          );
          tab.reload();
        },
        true
      );
    }});
  });
};


exports.testTabMove = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,foo";

    tabs.open({
      url: url,
      onOpen: function(tab) {
        test.assertEqual(tab.index, 1, "tab index before move matches");
        tab.index = 0;
        test.assertEqual(tab.index, 0, "tab index after move matches");
        closeBrowserWindow(window, function() test.done());
      }
    });
  });
};


exports.testOpen = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,default";
    tabs.open({
      url: url,
      onReady: function(tab) {
        test.assertEqual(tab.url, url, "URL of the new tab matches");
        test.assertEqual(window.content.location, url, "URL of active tab in the current window matches");
        test.assertEqual(tab.isPinned, false, "The new tab is not pinned");

        closeBrowserWindow(window, function() test.done());
      }
    });
  });
};


exports.testOpenPinned = function(test) {
  const xulApp = require("sdk/system/xul-app");
  if (xulApp.versionInRange(xulApp.platformVersion, "2.0b2", "*")) {
    
    test.waitUntilDone();
    openBrowserWindow(function(window, browser) {
      let tabs = require("sdk/tabs");
      let url = "data:text/html;charset=utf-8,default";
      tabs.open({
        url: url,
        isPinned: true,
        onOpen: function(tab) {
          test.assertEqual(tab.isPinned, true, "The new tab is pinned");
          closeBrowserWindow(window, function() test.done());
        }
      });
    });
  }
  else {
    test.pass("Pinned tabs are not supported in this application.");
  }
};


exports.testPinUnpin = function(test) {
  const xulApp = require("sdk/system/xul-app");
  if (xulApp.versionInRange(xulApp.platformVersion, "2.0b2", "*")) {
    test.waitUntilDone();
    openBrowserWindow(function(window, browser) {
      let tabs = require("sdk/tabs");
      let url = "data:text/html;charset=utf-8,default";
      tabs.open({
        url: url,
        onOpen: function(tab) {
          tab.pin();
          test.assertEqual(tab.isPinned, true, "The tab was pinned correctly");
          tab.unpin();
          test.assertEqual(tab.isPinned, false, "The tab was unpinned correctly");
          closeBrowserWindow(window, function() test.done());
        }
      });
    });
  }
  else {
    test.pass("Pinned tabs are not supported in this application.");
  }
};


exports.testInBackground = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let activeUrl = tabs.activeTab.url;
    let url = "data:text/html;charset=utf-8,background";
    test.assertEqual(activeWindow, window, "activeWindow matches this window");
    tabs.on('ready', function onReady(tab) {
      tabs.removeListener('ready', onReady);
      test.assertEqual(tabs.activeTab.url, activeUrl, "URL of active tab has not changed");
      test.assertEqual(tab.url, url, "URL of the new background tab matches");
      test.assertEqual(activeWindow, window, "a new window was not opened");
      test.assertNotEqual(tabs.activeTab.url, url, "URL of active tab is not the new URL");
      closeBrowserWindow(window, function() test.done());
    });
    tabs.open({
      url: url,
      inBackground: true
    });
  });
};


exports.testOpenInNewWindow = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");

    let cache = [];
    let windowUtils = require("sdk/deprecated/window-utils");
    let wt = new windowUtils.WindowTracker({
      onTrack: function(win) {
        cache.push(win);
      },
      onUntrack: function(win) {
        cache.splice(cache.indexOf(win), 1)
      }
    });
    let startWindowCount = cache.length;

    let url = "data:text/html;charset=utf-8,newwindow";
    tabs.open({
      url: url,
      inNewWindow: true,
      onReady: function(tab) {
        let newWindow = cache[cache.length - 1];
        test.assertEqual(cache.length, startWindowCount + 1, "a new window was opened");
        test.assertEqual(activeWindow, newWindow, "new window is active");
        test.assertEqual(tab.url, url, "URL of the new tab matches");
        test.assertEqual(newWindow.content.location, url, "URL of new tab in new window matches");
        test.assertEqual(tabs.activeTab.url, url, "URL of activeTab matches");
        for (var i in cache) cache[i] = null;
        wt.unload();
        closeBrowserWindow(newWindow, function() {
          closeBrowserWindow(window, function() test.done());
        });
      }
    });
  });
};


exports.testTabsEvent_onOpen = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,1";
    let eventCount = 0;

    
    function listener1(tab) {
      eventCount++;
    };
    tabs.on('open', listener1);

    
    tabs.on('open', function listener2(tab) {
      test.assertEqual(++eventCount, 2, "both listeners notified");
      tabs.removeListener('open', listener1);
      tabs.removeListener('open', listener2);
      closeBrowserWindow(window, function() test.done());
    });

    tabs.open(url);
  });
};


exports.testTabsEvent_onClose = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,onclose";
    let eventCount = 0;

    
    function listener1(tab) {
      eventCount++;
    }
    tabs.on('close', listener1);

    
    tabs.on('close', function listener2(tab) {
      test.assertEqual(++eventCount, 2, "both listeners notified");
      tabs.removeListener('close', listener1);
      tabs.removeListener('close', listener2);
      closeBrowserWindow(window, function() test.done());
    });

    tabs.on('ready', function onReady(tab) {
      tabs.removeListener('ready', onReady);
      tab.close();
    });

    tabs.open(url);
  });
};


exports.testTabsEvent_onCloseWindow = function(test) {
  test.waitUntilDone();

  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");

    let closeCount = 0, individualCloseCount = 0;
    function listener() {
      closeCount++;
    }
    tabs.on('close', listener);

    
    let openTabs = 1;
    function testCasePossiblyLoaded() {
      if (++openTabs == 4) {
        beginCloseWindow();
      }
    }

    tabs.open({
      url: "data:text/html;charset=utf-8,tab2",
      onOpen: function() testCasePossiblyLoaded(),
      onClose: function() individualCloseCount++
    });

    tabs.open({
      url: "data:text/html;charset=utf-8,tab3",
      onOpen: function() testCasePossiblyLoaded(),
      onClose: function() individualCloseCount++
    });

    tabs.open({
      url: "data:text/html;charset=utf-8,tab4",
      onOpen: function() testCasePossiblyLoaded(),
      onClose: function() individualCloseCount++
    });

    function beginCloseWindow() {
      closeBrowserWindow(window, function testFinished() {
        tabs.removeListener("close", listener);

        test.assertEqual(closeCount, 4, "Correct number of close events received");
        test.assertEqual(individualCloseCount, 3,
                         "Each tab with an attached onClose listener received a close " +
                         "event when the window was closed");

        test.done();
      });
    }

  });
}


exports.testTabsEvent_onReady = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,onready";
    let eventCount = 0;

    
    function listener1(tab) {
      eventCount++;
    };
    tabs.on('ready', listener1);

    
    tabs.on('ready', function listener2(tab) {
      test.assertEqual(++eventCount, 2, "both listeners notified");
      tabs.removeListener('ready', listener1);
      tabs.removeListener('ready', listener2);
      closeBrowserWindow(window, function() test.done());
    });

    tabs.open(url);
  });
};


exports.testTabsEvent_onActivate = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,onactivate";
    let eventCount = 0;

    
    function listener1(tab) {
      eventCount++;
    };
    tabs.on('activate', listener1);

    
    tabs.on('activate', function listener2(tab) {
      test.assertEqual(++eventCount, 2, "both listeners notified");
      tabs.removeListener('activate', listener1);
      tabs.removeListener('activate', listener2);
      closeBrowserWindow(window, function() test.done());
    });

    tabs.open(url);
  });
};


exports.testTabsEvent_onDeactivate = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,ondeactivate";
    let eventCount = 0;

    
    function listener1(tab) {
      eventCount++;
    };
    tabs.on('deactivate', listener1);

    
    tabs.on('deactivate', function listener2(tab) {
      test.assertEqual(++eventCount, 2, "both listeners notified");
      tabs.removeListener('deactivate', listener1);
      tabs.removeListener('deactivate', listener2);
      closeBrowserWindow(window, function() test.done());
    });

    tabs.on('open', function onOpen(tab) {
      tabs.removeListener('open', onOpen);
      tabs.open("data:text/html;charset=utf-8,foo");
    });

    tabs.open(url);
  });
};


exports.testTabsEvent_pinning = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let url = "data:text/html;charset=utf-8,1";

    tabs.on('open', function onOpen(tab) {
      tabs.removeListener('open', onOpen);
      tab.pin();
    });

    tabs.on('pinned', function onPinned(tab) {
      tabs.removeListener('pinned', onPinned);
      test.assert(tab.isPinned, "notified tab is pinned");
      tab.unpin();
    });

    tabs.on('unpinned', function onUnpinned(tab) {
      tabs.removeListener('unpinned', onUnpinned);
      test.assert(!tab.isPinned, "notified tab is not pinned");
      closeBrowserWindow(window, function() test.done());
    });

    tabs.open(url);
  });
};


exports.testPerTabEvents = function(test) {
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    var tabs = require("sdk/tabs");
    let eventCount = 0;

    tabs.open({
      url: "data:text/html;charset=utf-8,foo",
      onOpen: function(tab) {
        
        function listener1() {
          eventCount++;
        };
        tab.on('ready', listener1);

        
        tab.on('ready', function listener2() {
          test.assertEqual(eventCount, 1, "both listeners notified");
          tab.removeListener('ready', listener1);
          tab.removeListener('ready', listener2);
          closeBrowserWindow(window, function() test.done());
        });
      }
    });
  });
};

exports.testAttachOnOpen = function (test) {
  
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");

    tabs.open({
      url: "data:text/html;charset=utf-8,foobar",
      onOpen: function (tab) {
        let worker = tab.attach({
          contentScript: 'self.postMessage(document.location.href); ',
          onMessage: function (msg) {
            test.assertEqual(msg, "about:blank",
              "Worker document url is about:blank on open");
            worker.destroy();
            closeBrowserWindow(window, function() test.done());
          }
        });
      }
    });

  });
}

exports.testAttachOnMultipleDocuments = function (test) {
  
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let firstLocation = "data:text/html;charset=utf-8,foobar";
    let secondLocation = "data:text/html;charset=utf-8,bar";
    let thirdLocation = "data:text/html;charset=utf-8,fox";
    let onReadyCount = 0;
    let worker1 = null;
    let worker2 = null;
    let detachEventCount = 0;
    tabs.open({
      url: firstLocation,
      onReady: function (tab) {
        onReadyCount++;
        if (onReadyCount == 1) {
          worker1 = tab.attach({
            contentScript: 'self.on("message", ' +
                           '  function () self.postMessage(document.location.href)' +
                           ');',
            onMessage: function (msg) {
              test.assertEqual(msg, firstLocation,
                               "Worker url is equal to the 1st document");
              tab.url = secondLocation;
            },
            onDetach: function () {
              detachEventCount++;
              test.pass("Got worker1 detach event");
              test.assertRaises(function () {
                  worker1.postMessage("ex-1");
                },
                /Couldn't find the worker/,
                "postMessage throw because worker1 is destroyed");
              checkEnd();
            }
          });
          worker1.postMessage("new-doc-1");
        }
        else if (onReadyCount == 2) {

          worker2 = tab.attach({
            contentScript: 'self.on("message", ' +
                           '  function () self.postMessage(document.location.href)' +
                           ');',
            onMessage: function (msg) {
              test.assertEqual(msg, secondLocation,
                               "Worker url is equal to the 2nd document");
              tab.url = thirdLocation;
            },
            onDetach: function () {
              detachEventCount++;
              test.pass("Got worker2 detach event");
              test.assertRaises(function () {
                  worker2.postMessage("ex-2");
                },
                /Couldn't find the worker/,
                "postMessage throw because worker2 is destroyed");
              checkEnd();
            }
          });
          worker2.postMessage("new-doc-2");
        }
        else if (onReadyCount == 3) {
          tab.close();
        }
      }
    });

    function checkEnd() {
      if (detachEventCount != 2)
        return;

      test.pass("Got all detach events");

      closeBrowserWindow(window, function() test.done());
    }

  });
}


exports.testAttachWrappers = function (test) {
  
  test.waitUntilDone();
  openBrowserWindow(function(window, browser) {
    let tabs = require("sdk/tabs");
    let document = "data:text/html;charset=utf-8,<script>var globalJSVar = true; " +
                   "                       document.getElementById = 3;</script>";
    let count = 0;

    tabs.open({
      url: document,
      onReady: function (tab) {
        let worker = tab.attach({
          contentScript: 'try {' +
                         '  self.postMessage(!("globalJSVar" in window));' +
                         '  self.postMessage(typeof window.globalJSVar == "undefined");' +
                         '} catch(e) {' +
                         '  self.postMessage(e.message);' +
                         '}',
          onMessage: function (msg) {
            test.assertEqual(msg, true, "Worker has wrapped objects ("+count+")");
            if (count++ == 1)
              closeBrowserWindow(window, function() test.done());
          }
        });
      }
    });

  });
}

































exports['test window focus changes active tab'] = function(test) {
  test.waitUntilDone();
  let win1 = openBrowserWindow(function() {
    let win2 = openBrowserWindow(function() {
      let tabs = require("sdk/tabs");
      tabs.on("activate", function onActivate() {
        tabs.removeListener("activate", onActivate);
        test.pass("activate was called on windows focus change.");
        closeBrowserWindow(win1, function() {
          closeBrowserWindow(win2, function() { test.done(); });
        });
      });
      win1.focus();
    }, "data:text/html;charset=utf-8,test window focus changes active tab</br><h1>Window #2");
  }, "data:text/html;charset=utf-8,test window focus changes active tab</br><h1>Window #1");
};

exports['test ready event on new window tab'] = function(test) {
  test.waitUntilDone();
  let uri = encodeURI("data:text/html;charset=utf-8,Waiting for ready event!");

  require("sdk/tabs").on("ready", function onReady(tab) {
    if (tab.url === uri) {
      require("sdk/tabs").removeListener("ready", onReady);
      test.pass("ready event was emitted");
      closeBrowserWindow(window, function() {
        test.done();
      });
    }
  });

  let window = openBrowserWindow(function(){}, uri);
};

exports['test unique tab ids'] = function(test) {
  test.waitUntilDone();

  var windows = require('windows').browserWindows,
    tabIds = {}, win1, win2;

  let steps = [
    function (index) {
      win1 = windows.open({
          url: "data:text/html;charset=utf-8,foo",
          onOpen: function(window) {
            tabIds['tab1'] = window.tabs.activeTab.id;
            next(index);
          }
      });
    },
    function (index) {
      win2 = windows.open({
          url: "data:text/html;charset=utf-8,foo",
          onOpen: function(window) {
            tabIds['tab2'] = window.tabs.activeTab.id;
            next(index);
          }
      });
    },
    function (index) {
      test.assertNotEqual(tabIds.tab1, tabIds.tab2, "Tab ids should be unique.");
      win1.close();
      win2.close();
      test.done();
    }
  ];

  function next(index) {
    if (index === steps.length) {
      return;
    }
    let fn = steps[index];
    index++
    fn(index);
  }

  
  next(0);
}




this.__defineGetter__("activeWindow", function activeWindow() {
  return Cc["@mozilla.org/appshell/window-mediator;1"].
         getService(Ci.nsIWindowMediator).
         getMostRecentWindow("navigator:browser");
});


function openBrowserWindow(callback, url) {
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let urlString = Cc["@mozilla.org/supports-string;1"].
                  createInstance(Ci.nsISupportsString);
  urlString.data = url;
  let window = ww.openWindow(null, "chrome://browser/content/browser.xul",
                             "_blank", "chrome,all,dialog=no", urlString);

  if (callback) {
    window.addEventListener("load", function onLoad(event) {
      if (event.target && event.target.defaultView == window) {
        window.removeEventListener("load", onLoad, true);
        let browsers = window.document.getElementsByTagName("tabbrowser");
        try {
          timer.setTimeout(function () {
            callback(window, browsers[0]);
          }, 10);
        }
        catch (e) {
          console.exception(e);
        }
      }
    }, true);
  }

  return window;
}


function closeBrowserWindow(window, callback) {
  window.addEventListener("unload", function unload() {
    window.removeEventListener("unload", unload, false);
    callback();
  }, false);
  window.close();
}
