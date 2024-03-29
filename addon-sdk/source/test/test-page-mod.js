


"use strict";

const { Cc, Ci, Cu } = require("chrome");
const { PageMod } = require("sdk/page-mod");
const { testPageMod, handleReadyState, openNewTab,
        contentScriptWhenServer, createLoader } = require("./page-mod/helpers");
const { Loader } = require("sdk/test/loader");
const tabs = require("sdk/tabs");
const { setTimeout } = require("sdk/timers");
const system = require("sdk/system/events");
const { open, getFrames, getMostRecentBrowserWindow, getInnerId } = require("sdk/window/utils");
const { getTabContentWindow, getActiveTab, setTabURL, openTab, closeTab,
        getBrowserForTab } = require("sdk/tabs/utils");
const xulApp = require("sdk/system/xul-app");
const { isPrivateBrowsingSupported } = require("sdk/self");
const { isPrivate } = require("sdk/private-browsing");
const { openWebpage } = require("./private-browsing/helper");
const { isTabPBSupported, isWindowPBSupported } = require("sdk/private-browsing/utils");
const promise = require("sdk/core/promise");
const { pb } = require("./private-browsing/helper");
const { URL } = require("sdk/url");
const { defer, all, resolve } = require("sdk/core/promise");
const { waitUntil } = require("sdk/test/utils");
const data = require("./fixtures");
const { cleanUI, after } = require("sdk/test/utils");

const testPageURI = data.url("test.html");

function Isolate(worker) {
  return "(" + worker + ")()";
}



exports.testPageMod1 = function*(assert) {
  let modAttached = defer();
  let mod = PageMod({
    include: /about:/,
    contentScriptWhen: "end",
    contentScript: "new " + function WorkerScope() {
      window.document.body.setAttribute("JEP-107", "worked");

      self.port.once("done", () => {
        self.port.emit("results", window.document.body.getAttribute("JEP-107"))
      });
    },
    onAttach: function(worker) {
      assert.equal(this, mod, "The 'this' object is the page mod.");
      mod.port.once("results", modAttached.resolve)
      mod.port.emit("done");
    }
  });

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: "about:",
      inBackground: true,
      onReady: resolve
    })
  });
  assert.pass("test tab was opened.");

  let worked = yield modAttached.promise;
  assert.pass("test mod was attached.");

  mod.destroy();
  assert.pass("test mod was destroyed.");

  assert.equal(worked, "worked", "PageMod.onReady test");
};

exports.testPageMod2 = function*(assert) {
  let modAttached = defer();
  let mod = PageMod({
    include: testPageURI,
    contentScriptWhen: "end",
    contentScript: [
      'new ' + function contentScript() {
        window.AUQLUE = function() { return 42; }
        try {
          window.AUQLUE()
        }
        catch(e) {
          throw new Error("PageMod scripts executed in order");
        }
        document.documentElement.setAttribute("first", "true");
      },
      'new ' + function contentScript() {
        document.documentElement.setAttribute("second", "true");

        self.port.once("done", () => {
          self.port.emit("results", {
            "first": window.document.documentElement.getAttribute("first"),
            "second": window.document.documentElement.getAttribute("second"),
            "AUQLUE": unsafeWindow.getAUQLUE()
          });
        });
      }
    ],
    onAttach: modAttached.resolve
  });

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: testPageURI,
      inBackground: true,
      onReady: resolve
    })
  });
  assert.pass("test tab was opened.");

  let worker = yield modAttached.promise;
  assert.pass("test mod was attached.");

  let results = yield new Promise(resolve => {
    worker.port.once("results", resolve)
    worker.port.emit("done");
  });

  mod.destroy();
  assert.pass("test mod was destroyed.");

  assert.equal(results["first"],
               "true",
               "PageMod test #2: first script has run");
  assert.equal(results["second"],
               "true",
               "PageMod test #2: second script has run");
  assert.equal(results["AUQLUE"], false,
               "PageMod test #2: scripts get a wrapped window");
};

exports.testPageModIncludes = function*(assert) {
  var modsAttached = [];
  var modNumber = 0;
  var modAttached = defer();
  let includes = [
    "*",
    "*.google.com",
    "resource:*",
    "resource:",
    testPageURI
  ];
  let expected = [
    false,
    false,
    true,
    false,
    true
  ]

  let mod = PageMod({
    include: testPageURI,
    contentScript: 'new ' + function() {
      self.port.on("get-local-storage", () => {
        let result = {};
        self.options.forEach(include => {
          result[include] = !!window.localStorage[include]
        });

        self.port.emit("got-local-storage", result);

        window.localStorage.clear();
      });
    },
    contentScriptOptions: includes,
    onAttach: modAttached.resolve
  });

  function createPageModTest(include, expectedMatch) {
    var modIndex = modNumber++;

    let attached = defer();
    modsAttached.push(expectedMatch ? attached.promise : resolve());

    
    return PageMod({
      include: include,
      contentScript: 'new ' + function() {
        self.on("message", function(msg) {
          window.localStorage[msg] = true
          self.port.emit('done');
        });
      },
      
      
      
      contentScriptWhen: 'start',
      onAttach: function(worker) {
        assert.pass("mod " + modIndex + " was attached");

        worker.port.once("done", () => {
          assert.pass("mod " + modIndex + " is done");
          attached.resolve(worker);
        });
        worker.postMessage(this.include[0]);
      }
    });
  }

  let mods = [
    createPageModTest("*", false),
    createPageModTest("*.google.com", false),
    createPageModTest("resource:*", true),
    createPageModTest("resource:", false),
    createPageModTest(testPageURI, true)
  ];

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: testPageURI,
      inBackground: true,
      onReady: resolve
    });
  });
  assert.pass("tab was opened");

  yield all(modsAttached);
  assert.pass("all mods were attached.");

  mods.forEach(mod => mod.destroy());
  assert.pass("all mods were destroyed.");

  yield modAttached.promise;
  assert.pass("final test mod was attached.");

  yield new Promise(resolve => {
    mod.port.on("got-local-storage", (storage) => {
      includes.forEach((include, i) => {
        assert.equal(storage[include], expected[i], "localStorage is correct for " + include);
      });
      resolve();
    });
    mod.port.emit("get-local-storage");
  });
  assert.pass("final test of localStorage is complete.");

  mod.destroy();
  assert.pass("final test mod was destroyed.");
};

exports.testPageModExcludes = function(assert, done) {
  var asserts = [];
  function createPageModTest(include, exclude, expectedMatch) {
    
    asserts.push(function(test, win) {
      var matches = JSON.stringify([include, exclude]) in win.localStorage;
      assert.ok(expectedMatch ? matches : !matches,
          "[include, exclude] = [" + include + ", " + exclude +
          "] match test, expected: " + expectedMatch);
    });
    
    return {
      include: include,
      exclude: exclude,
      contentScript: 'new ' + function() {
        self.on("message", function(msg) {
          
          window.localStorage[JSON.stringify(msg)] = true;
        });
      },
      
      
      
      contentScriptWhen: 'start',
      onAttach: function(worker) {
        worker.postMessage([this.include[0], this.exclude[0]]);
      }
    };
  }

  testPageMod(assert, done, testPageURI, [
      createPageModTest("*", testPageURI, false),
      createPageModTest(testPageURI, testPageURI, false),
      createPageModTest(testPageURI, "resource://*", false),
      createPageModTest(testPageURI, "*.google.com", true)
    ],
    function (win, done) {
      waitUntil(() => win.localStorage[JSON.stringify([testPageURI, "*.google.com"])],
          testPageURI + " page-mod to be executed")
        .then(() => {
          asserts.forEach(fn => fn(assert, win));
          win.localStorage.clear();
          done();
        });
    });
};

exports.testPageModValidationAttachTo = function(assert) {
  [{ val: 'top', type: 'string "top"' },
   { val: 'frame', type: 'string "frame"' },
   { val: ['top', 'existing'], type: 'array with "top" and "existing"' },
   { val: ['frame', 'existing'], type: 'array with "frame" and "existing"' },
   { val: ['top'], type: 'array with "top"' },
   { val: ['frame'], type: 'array with "frame"' },
   { val: undefined, type: 'undefined' }].forEach((attachTo) => {
    new PageMod({ attachTo: attachTo.val, include: '*.validation111' });
    assert.pass("PageMod() does not throw when attachTo is " + attachTo.type);
  });

  [{ val: 'existing', type: 'string "existing"' },
   { val: ['existing'], type: 'array with "existing"' },
   { val: 'not-legit', type: 'string with "not-legit"' },
   { val: ['not-legit'], type: 'array with "not-legit"' },
   { val: {}, type: 'object' }].forEach((attachTo) => {
    assert.throws(() =>
      new PageMod({ attachTo: attachTo.val, include: '*.validation111' }),
      /The `attachTo` option/,
      "PageMod() throws when 'attachTo' option is " + attachTo.type + ".");
  });
};

exports.testPageModValidationInclude = function(assert) {
  [{ val: undefined, type: 'undefined' },
   { val: {}, type: 'object' },
   { val: [], type: 'empty array'},
   { val: [/regexp/, 1], type: 'array with non string/regexp' },
   { val: 1, type: 'number' }].forEach((include) => {
    assert.throws(() => new PageMod({ include: include.val }),
      /The `include` option must always contain atleast one rule/,
      "PageMod() throws when 'include' option is " + include.type + ".");
  });

  [{ val: '*.validation111', type: 'string' },
   { val: /validation111/, type: 'regexp' },
   { val: ['*.validation111'], type: 'array with length > 0'}].forEach((include) => {
    new PageMod({ include: include.val });
    assert.pass("PageMod() does not throw when include option is " + include.type);
  });
};

exports.testPageModValidationExclude = function(assert) {
  let includeVal = '*.validation111';

  [{ val: {}, type: 'object' },
   { val: [], type: 'empty array'},
   { val: [/regexp/, 1], type: 'array with non string/regexp' },
   { val: 1, type: 'number' }].forEach((exclude) => {
    assert.throws(() => new PageMod({ include: includeVal, exclude: exclude.val }),
      /If set, the `exclude` option must always contain at least one rule as a string, regular expression, or an array of strings and regular expressions./,
      "PageMod() throws when 'exclude' option is " + exclude.type + ".");
  });

  [{ val: undefined, type: 'undefined' },
   { val: '*.validation111', type: 'string' },
   { val: /validation111/, type: 'regexp' },
   { val: ['*.validation111'], type: 'array with length > 0'}].forEach((exclude) => {
    new PageMod({ include: includeVal, exclude: exclude.val });
    assert.pass("PageMod() does not throw when exclude option is " + exclude.type);
  });
};


exports.testCommunication1 = function*(assert) {
  let workerDone = defer();

  let mod = PageMod({
    include: "about:*",
    contentScriptWhen: "end",
    contentScript: 'new ' + function WorkerScope() {
      self.on('message', function(msg) {
        document.body.setAttribute('JEP-107', 'worked');
        self.postMessage(document.body.getAttribute('JEP-107'));
      });
      self.port.on('get-jep-107', () => {
        self.port.emit('got-jep-107', document.body.getAttribute('JEP-107'));
      });
    },
    onAttach: function(worker) {
      worker.on('error', function(e) {
        assert.fail('Errors where reported');
      });
      worker.on('message', function(value) {
        assert.equal(
          "worked",
          value,
          "test comunication"
        );
        workerDone.resolve();
      });
      worker.postMessage("do it!")
    }
  });

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: "about:",
      onReady: resolve
    });
  });
  assert.pass("opened tab");

  yield workerDone.promise;
  assert.pass("the worker has made a change");

  let value = yield new Promise(resolve => {
    mod.port.once("got-jep-107", resolve);
    mod.port.emit("get-jep-107");
  });

  assert.equal("worked", value, "attribute should be modified");

  mod.destroy();
  assert.pass("the worker was destroyed");
};

exports.testCommunication2 = function*(assert) {
  let workerDone = defer();
  let url = data.url("test.html");

  let mod = PageMod({
    include: url,
    contentScriptWhen: 'start',
    contentScript: 'new ' + function WorkerScope() {
      document.documentElement.setAttribute('AUQLUE', 42);

      window.addEventListener('load', function listener() {
        self.postMessage({
          msg: 'onload',
          AUQLUE: document.documentElement.getAttribute('AUQLUE')
        });
      }, false);

      self.on("message", function(msg) {
        if (msg == "get window.test") {
          unsafeWindow.changesInWindow();
        }

        self.postMessage({
          msg: document.documentElement.getAttribute("test")
        });
      });
    },
    onAttach: function(worker) {
      worker.on('error', function(e) {
        assert.fail('Errors where reported');
      });
      worker.on('message', function({ msg, AUQLUE }) {
        if ('onload' == msg) {
          assert.equal('42', AUQLUE, 'PageMod scripts executed in order');
          worker.postMessage('get window.test');
        }
        else {
          assert.equal('changes in window', msg, 'PageMod test #2: second script has run');
          workerDone.resolve();
        }
      });
    }
  });

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: url,
      inBackground: true,
      onReady: resolve
    });
  });
  assert.pass("opened tab");

  yield workerDone.promise;

  mod.destroy();
  assert.pass("the worker was destroyed");
};

exports.testEventEmitter = function(assert, done) {
  let workerDone = false,
      callbackDone = null;

  testPageMod(assert, done, "about:", [{
      include: "about:*",
      contentScript: 'new ' + function WorkerScope() {
        self.port.on('addon-to-content', function(data) {
          self.port.emit('content-to-addon', data);
        });
      },
      onAttach: function(worker) {
        worker.on('error', function(e) {
          assert.fail('Errors were reported : '+e);
        });
        worker.port.on('content-to-addon', function(value) {
          assert.equal(
            "worked",
            value,
            "EventEmitter API works!"
          );
          if (callbackDone)
            callbackDone();
          else
            workerDone = true;
        });
        worker.port.emit('addon-to-content', 'worked');
      }
    }],
    function(win, done) {
      if (workerDone)
        done();
      else
        callbackDone = done;
    }
  );
};



exports.testMixedContext = function(assert, done) {
  let doneCallback = null;
  let messages = 0;
  let modObject = {
    include: "data:text/html;charset=utf-8,",
    contentScript: 'new ' + function WorkerScope() {
      
      
      let isContextShared = "sharedAttribute" in document;
      self.postMessage(isContextShared);
      document.sharedAttribute = true;
    },
    onAttach: function(w) {
      w.on("message", function (isContextShared) {
        if (isContextShared) {
          assert.fail("Page mod contexts are mixed.");
          doneCallback();
        }
        else if (++messages == 2) {
          assert.pass("Page mod contexts are different.");
          doneCallback();
        }
      });
    }
  };
  testPageMod(assert, done, "data:text/html;charset=utf-8,", [modObject, modObject],
    function(win, done) {
      doneCallback = done;
    }
  );
};

exports.testHistory = function(assert, done) {
  
  
  
  let url = data.url("test-page-mod.html");
  let callbackDone = null;
  testPageMod(assert, done, url, [{
      include: url,
      contentScriptWhen: 'end',
      contentScript: 'new ' + function WorkerScope() {
        history.pushState({}, "", "#");
        history.replaceState({foo: "bar"}, "", "#");
        self.postMessage(history.state);
      },
      onAttach: function(worker) {
        worker.on('message', function (data) {
          assert.equal(JSON.stringify(data), JSON.stringify({foo: "bar"}),
                           "History API works!");
          callbackDone();
        });
      }
    }],
    function(win, done) {
      callbackDone = done;
    }
  );
};

exports.testRelatedTab = function(assert, done) {
  let tab;
  let pageMod = new PageMod({
    include: "about:*",
    onAttach: function(worker) {
      assert.ok(!!worker.tab, "Worker.tab exists");
      assert.equal(tab, worker.tab, "Worker.tab is valid");
      pageMod.destroy();
      tab.close(done);
    }
  });

  tabs.open({
    url: "about:",
    onOpen: function onOpen(t) {
      tab = t;
    }
  });
};

exports.testRelatedTabNoRequireTab = function(assert, done) {
  let loader = Loader(module);
  let tab;
  let url = "data:text/html;charset=utf-8," + encodeURI("Test related worker tab 2");
  let { PageMod } = loader.require("sdk/page-mod");
  let pageMod = new PageMod({
    include: url,
    onAttach: function(worker) {
      assert.equal(worker.tab.url, url, "Worker.tab.url is valid");
      worker.tab.close(function() {
        pageMod.destroy();
        loader.unload();
        done();
      });
    }
  });

  tabs.open(url);
};

exports.testRelatedTabNoOtherReqs = function(assert, done) {
  let loader = Loader(module);
  let { PageMod } = loader.require("sdk/page-mod");
  let pageMod = new PageMod({
    include: "about:blank?testRelatedTabNoOtherReqs",
    onAttach: function(worker) {
      assert.ok(!!worker.tab, "Worker.tab exists");
      pageMod.destroy();
      worker.tab.close(function() {
        worker.destroy();
        loader.unload();
        done();
      });
    }
  });

  tabs.open({
    url: "about:blank?testRelatedTabNoOtherReqs"
  });
};

exports.testWorksWithExistingTabs = function(assert, done) {
  let url = "data:text/html;charset=utf-8," + encodeURI("Test unique document");
  let { PageMod } = require("sdk/page-mod");
  tabs.open({
    url: url,
    onReady: function onReady(tab) {
      let pageModOnExisting = new PageMod({
        include: url,
        attachTo: ["existing", "top", "frame"],
        onAttach: function(worker) {
          assert.ok(!!worker.tab, "Worker.tab exists");
          assert.equal(tab, worker.tab, "A worker has been created on this existing tab");

          worker.on('pageshow', () => {
            assert.fail("Should not have seen pageshow for an already loaded page");
          });

          setTimeout(function() {
            pageModOnExisting.destroy();
            pageModOffExisting.destroy();
            tab.close(done);
          }, 0);
        }
      });

      let pageModOffExisting = new PageMod({
        include: url,
        onAttach: function(worker) {
          assert.fail("pageModOffExisting page-mod should not have attached to anything");
        }
      });
    }
  });
};

exports.testExistingFrameDoesntMatchInclude = function(assert, done) {
  let iframeURL = 'data:text/html;charset=utf-8,UNIQUE-TEST-STRING-42';
  let iframe = '<iframe src="' + iframeURL + '" />';
  let url = 'data:text/html;charset=utf-8,' + encodeURIComponent(iframe);
  tabs.open({
    url: url,
    onReady: function onReady(tab) {
      let pagemod = new PageMod({
        include: url,
        attachTo: ['existing', 'frame'],
        onAttach: function() {
          assert.fail("Existing iframe URL doesn't match include, must not attach to anything");
        }
      });
      setTimeout(function() {
        assert.pass("PageMod didn't attach to anything")
        pagemod.destroy();
        tab.close(done);
      }, 250);
    }
  });
};

exports.testExistingOnlyFrameMatchesInclude = function(assert, done) {
  let iframeURL = 'data:text/html;charset=utf-8,UNIQUE-TEST-STRING-43';
  let iframe = '<iframe src="' + iframeURL + '" />';
  let url = 'data:text/html;charset=utf-8,' + encodeURIComponent(iframe);
  tabs.open({
    url: url,
    onReady: function onReady(tab) {
      let pagemod = new PageMod({
        include: iframeURL,
        attachTo: ['existing', 'frame'],
        onAttach: function(worker) {
          assert.equal(iframeURL, worker.url,
              "PageMod attached to existing iframe when only it matches include rules");
          pagemod.destroy();
          tab.close(done);
        }
      });
    }
  });
};

exports.testAttachOnlyOncePerDocument = function(assert, done) {
  let iframeURL = 'data:text/html;charset=utf-8,testAttachOnlyOncePerDocument';
  let iframe = '<iframe src="' + iframeURL + '" />';
  let url = 'data:text/html;charset=utf-8,' + encodeURIComponent(iframe);
  let count = 0;

  tabs.open({
    url: url,
    onReady: function onReady(tab) {
      let pagemod = new PageMod({
        include: iframeURL,
        attachTo: ['existing', 'frame'],
        onAttach: (worker) => {
          count++;
          assert.equal(iframeURL, worker.url,
            "PageMod attached to existing iframe");
          assert.equal(count, 1, "PageMod attached only once");
          setTimeout(_ => {
            assert.equal(count, 1, "PageMod attached only once");
            pagemod.destroy();
            tab.close(done);
          }, 1);
        }
      });
    }
  });
}

exports.testContentScriptWhenDefault = function(assert) {
  let pagemod = PageMod({include: '*'});

  assert.equal(pagemod.contentScriptWhen, 'end', "Default contentScriptWhen is 'end'");
  pagemod.destroy();
}



exports.testContentScriptWhenForNewTabs = function(assert, done) {
  let srv = contentScriptWhenServer();
  let url = srv.URL + '?ForNewTabs';
  let count = 0;

  handleReadyState(url, 'start', {
    onLoading: (tab) => {
      assert.pass("PageMod is attached while document is loading");
      checkDone(++count, tab, srv, done);
    },
    onInteractive: () => assert.fail("onInteractive should not be called with 'start'."),
    onComplete: () => assert.fail("onComplete should not be called with 'start'."),
  });

  handleReadyState(url, 'ready', {
    onInteractive: (tab) => {
      assert.pass("PageMod is attached while document is interactive");
      checkDone(++count, tab, srv, done);
    },
    onLoading: () => assert.fail("onLoading should not be called with 'ready'."),
    onComplete: () => assert.fail("onComplete should not be called with 'ready'."),
  });

  handleReadyState(url, 'end', {
    onComplete: (tab) => {
      assert.pass("PageMod is attached when document is complete");
      checkDone(++count, tab, srv, done);
    },
    onLoading: () => assert.fail("onLoading should not be called with 'end'."),
    onInteractive: () => assert.fail("onInteractive should not be called with 'end'."),
  });

  tabs.open(url);
}



exports.testContentScriptWhenOnTabOpen = function(assert, done) {
  let srv = contentScriptWhenServer();
  let url = srv.URL + '?OnTabOpen';
  let count = 0;

  tabs.open({
    url: url,
    onOpen: function(tab) {

      handleReadyState(url, 'start', {
        onLoading: () => {
          assert.pass("PageMod is attached while document is loading");
          checkDone(++count, tab, srv, done);
        },
        onInteractive: () => assert.fail("onInteractive should not be called with 'start'."),
        onComplete: () => assert.fail("onComplete should not be called with 'start'."),
      });

      handleReadyState(url, 'ready', {
        onInteractive: () => {
          assert.pass("PageMod is attached while document is interactive");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'ready'."),
        onComplete: () => assert.fail("onComplete should not be called with 'ready'."),
      });

      handleReadyState(url, 'end', {
        onComplete: () => {
          assert.pass("PageMod is attached when document is complete");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'end'."),
        onInteractive: () => assert.fail("onInteractive should not be called with 'end'."),
      });

    }
  });
}



exports.testContentScriptWhenOnTabReady = function(assert, done) {
  let srv = contentScriptWhenServer();
  let url = srv.URL + '?OnTabReady';
  let count = 0;

  tabs.open({
    url: url,
    onReady: function(tab) {

      handleReadyState(url, 'start', {
        onInteractive: () => {
          assert.pass("PageMod is attached while document is interactive");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'start'."),
        onComplete: () => assert.fail("onComplete should not be called with 'start'."),
      });

      handleReadyState(url, 'ready', {
        onInteractive: () => {
          assert.pass("PageMod is attached while document is interactive");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'ready'."),
        onComplete: () => assert.fail("onComplete should not be called with 'ready'."),
      });

      handleReadyState(url, 'end', {
        onComplete: () => {
          assert.pass("PageMod is attached when document is complete");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'end'."),
        onInteractive: () => assert.fail("onInteractive should not be called with 'end'."),
      });

    }
  });
}



exports.testContentScriptWhenOnTabLoad = function(assert, done) {
  let srv = contentScriptWhenServer();
  let url = srv.URL + '?OnTabLoad';
  let count = 0;

  tabs.open({
    url: url,
    onLoad: function(tab) {

      handleReadyState(url, 'start', {
        onComplete: () => {
          assert.pass("PageMod is attached when document is complete");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'start'."),
        onInteractive: () => assert.fail("onInteractive should not be called with 'start'."),
      });

      handleReadyState(url, 'ready', {
        onComplete: () => {
          assert.pass("PageMod is attached when document is complete");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'ready'."),
        onInteractive: () => assert.fail("onInteractive should not be called with 'ready'."),
      });

      handleReadyState(url, 'end', {
        onComplete: () => {
          assert.pass("PageMod is attached when document is complete");
          checkDone(++count, tab, srv, done);
        },
        onLoading: () => assert.fail("onLoading should not be called with 'end'."),
        onInteractive: () => assert.fail("onInteractive should not be called with 'end'."),
      });

    }
  });
}

function checkDone(count, tab, srv, done) {
  if (count === 3)
    tab.close(_ => srv.stop(done));
}

exports.testTabWorkerOnMessage = function(assert, done) {
  let { browserWindows } = require("sdk/windows");
  let tabs = require("sdk/tabs");
  let { PageMod } = require("sdk/page-mod");

  let url1 = "data:text/html;charset=utf-8,<title>tab1</title><h1>worker1.tab</h1>";
  let url2 = "data:text/html;charset=utf-8,<title>tab2</title><h1>worker2.tab</h1>";
  let worker1 = null;

  let mod = PageMod({
    include: "data:text/html*",
    contentScriptWhen: "ready",
    contentScript: "self.postMessage('#1');",
    onAttach: function onAttach(worker) {
      worker.on("message", function onMessage() {
        this.tab.attach({
          contentScriptWhen: "ready",
          contentScript: "self.postMessage({ url: window.location.href, title: document.title });",
          onMessage: function onMessage(data) {
            assert.equal(this.tab.url, data.url, "location is correct");
            assert.equal(this.tab.title, data.title, "title is correct");
            if (this.tab.url === url1) {
              worker1 = this;
              tabs.open({ url: url2, inBackground: true });
            }
            else if (this.tab.url === url2) {
              mod.destroy();
              worker1.tab.close(function() {
                worker1.destroy();
                worker.tab.close(function() {
                  worker.destroy();
                  done();
                });
              });
            }
          }
        });
      });
    }
  });

  tabs.open(url1);
};

exports.testAutomaticDestroy = function(assert, done) {
  let loader = Loader(module);

  let pageMod = loader.require("sdk/page-mod").PageMod({
    include: "about:*",
    contentScriptWhen: "start",
    onAttach: function(w) {
      assert.fail("Page-mod should have been detroyed during module unload");
    }
  });

  
  loader.unload();

  
  let tabs = require("sdk/tabs");
  tabs.open({
    url: "about:",
    onReady: function onReady(tab) {
      assert.pass("check automatic destroy");
      tab.close(done);
    }
  });
};

exports.testAttachToTabsOnly = function(assert, done) {
  let { PageMod } = require('sdk/page-mod');
  let openedTab = null; 
  let workerCount = 0;

  let mod = PageMod({
    include: 'data:text/html*',
    contentScriptWhen: 'start',
    contentScript: '',
    onAttach: function onAttach(worker) {
      if (worker.tab === openedTab) {
        if (++workerCount == 3) {
          assert.pass('Succesfully applied to tab documents and its iframe');
          worker.destroy();
          mod.destroy();
          openedTab.close(done);
        }
      }
      else {
        assert.fail('page-mod attached to a non-tab document');
      }
    }
  });

  function openHiddenFrame() {
    assert.pass('Open iframe in hidden window');
    let hiddenFrames = require('sdk/frame/hidden-frame');
    let hiddenFrame = hiddenFrames.add(hiddenFrames.HiddenFrame({
      onReady: function () {
        let element = this.element;
        element.addEventListener('DOMContentLoaded', function onload() {
          element.removeEventListener('DOMContentLoaded', onload, false);
          hiddenFrames.remove(hiddenFrame);

          if (!xulApp.is("Fennec")) {
            openToplevelWindow();
          }
          else {
            openBrowserIframe();
          }
        }, false);
        element.setAttribute('src', 'data:text/html;charset=utf-8,foo');
      }
    }));
  }

  function openToplevelWindow() {
    assert.pass('Open toplevel window');
    let win = open('data:text/html;charset=utf-8,bar');
    win.addEventListener('DOMContentLoaded', function onload() {
      win.removeEventListener('DOMContentLoaded', onload, false);
      win.close();
      openBrowserIframe();
    }, false);
  }

  function openBrowserIframe() {
    assert.pass('Open iframe in browser window');
    let window = require('sdk/deprecated/window-utils').activeBrowserWindow;
    let document = window.document;
    let iframe = document.createElement('iframe');
    iframe.setAttribute('type', 'content');
    iframe.setAttribute('src', 'data:text/html;charset=utf-8,foobar');
    iframe.addEventListener('DOMContentLoaded', function onload() {
      iframe.removeEventListener('DOMContentLoaded', onload, false);
      iframe.parentNode.removeChild(iframe);
      openTabWithIframes();
    }, false);
    document.documentElement.appendChild(iframe);
  }

  
  function openTabWithIframes() {
    assert.pass('Open iframes in a tab');
    let subContent = '<iframe src="data:text/html;charset=utf-8,sub frame" />'
    let content = '<iframe src="data:text/html;charset=utf-8,' +
                  encodeURIComponent(subContent) + '" />';
    require('sdk/tabs').open({
      url: 'data:text/html;charset=utf-8,' + encodeURIComponent(content),
      onOpen: function onOpen(tab) {
        openedTab = tab;
      }
    });
  }

  openHiddenFrame();
};

exports['test111 attachTo [top]'] = function(assert, done) {
  let { PageMod } = require('sdk/page-mod');

  let subContent = '<iframe src="data:text/html;charset=utf-8,sub frame" />'
  let content = '<iframe src="data:text/html;charset=utf-8,' +
                encodeURIComponent(subContent) + '" />';
  let topDocumentURL = 'data:text/html;charset=utf-8,' + encodeURIComponent(content)

  let workerCount = 0;

  let mod = PageMod({
    include: 'data:text/html*',
    contentScriptWhen: 'start',
    contentScript: 'self.postMessage(document.location.href);',
    attachTo: ['top'],
    onAttach: function onAttach(worker) {
      if (++workerCount == 1) {
        worker.on('message', function (href) {
          assert.equal(href, topDocumentURL,
                           "worker on top level document only");
          let tab = worker.tab;
          worker.destroy();
          mod.destroy();
          tab.close(done);
        });
      }
      else {
        assert.fail('page-mod attached to a non-top document');
      }
    }
  });

  require('sdk/tabs').open(topDocumentURL);
};

exports['test111 attachTo [frame]'] = function(assert, done) {
  let { PageMod } = require('sdk/page-mod');

  let subFrameURL = 'data:text/html;charset=utf-8,subframe';
  let subContent = '<iframe src="' + subFrameURL + '" />';
  let frameURL = 'data:text/html;charset=utf-8,' + encodeURIComponent(subContent);
  let content = '<iframe src="' + frameURL + '" />';
  let topDocumentURL = 'data:text/html;charset=utf-8,' + encodeURIComponent(content)

  let workerCount = 0, messageCount = 0;

  function onMessage(href) {
    if (href == frameURL)
      assert.pass("worker on first frame");
    else if (href == subFrameURL)
      assert.pass("worker on second frame");
    else
      assert.fail("worker on unexpected document: " + href);
    this.destroy();
    if (++messageCount == 2) {
      mod.destroy();
      require('sdk/tabs').activeTab.close(done);
    }
  }
  let mod = PageMod({
    include: 'data:text/html*',
    contentScriptWhen: 'start',
    contentScript: 'self.postMessage(document.location.href);',
    attachTo: ['frame'],
    onAttach: function onAttach(worker) {
      if (++workerCount <= 2) {
        worker.on('message', onMessage);
      }
      else {
        assert.fail('page-mod attached to a non-frame document');
      }
    }
  });

  require('sdk/tabs').open(topDocumentURL);
};

exports.testContentScriptOptionsOption = function(assert, done) {
  let callbackDone = null;
  testPageMod(assert, done, "about:", [{
      include: "about:*",
      contentScript: "self.postMessage( [typeof self.options.d, self.options] );",
      contentScriptWhen: "end",
      contentScriptOptions: {a: true, b: [1,2,3], c: "string", d: function(){ return 'test'}},
      onAttach: function(worker) {
        worker.on('message', function(msg) {
          assert.equal( msg[0], 'undefined', 'functions are stripped from contentScriptOptions' );
          assert.equal( typeof msg[1], 'object', 'object as contentScriptOptions' );
          assert.equal( msg[1].a, true, 'boolean in contentScriptOptions' );
          assert.equal( msg[1].b.join(), '1,2,3', 'array and numbers in contentScriptOptions' );
          assert.equal( msg[1].c, 'string', 'string in contentScriptOptions' );
          callbackDone();
        });
      }
    }],
    function(win, done) {
      callbackDone = done;
    }
  );
};

exports.testPageModCss = function(assert, done) {
  let [pageMod] = testPageMod(assert, done,
    'data:text/html;charset=utf-8,<div style="background: silver">css test</div>', [{
      include: ["*", "data:*"],
      contentStyle: "div { height: 100px; }",
      contentStyleFile: [data.url("include-file.css"), "./border-style.css"]
    }],
    function(win, done) {
      let div = win.document.querySelector("div");

      assert.equal(div.clientHeight, 100,
        "PageMod contentStyle worked");

      assert.equal(div.offsetHeight, 120,
        "PageMod contentStyleFile worked");

      assert.equal(win.getComputedStyle(div).borderTopStyle, "dashed",
        "PageMod contentStyleFile with relative path worked");

      done();
    }
  );
};

exports.testPageModCssList = function*(assert) {
  const URL = 'data:text/html;charset=utf-8,<div style="width:320px; max-width: 480px!important">css test</div>';
  let modAttached = defer();

  let pageMod = PageMod({
    include: "data:*",
    contentStyleFile: [
      
      "data:text/css;charset=utf-8,div { border: 1px solid black; }",
      "data:text/css;charset=utf-8,div { border: 10px solid black; }",
      
      "data:text/css;charset=utf-8s,div { height: 1000px; }",
      
      "data:text/css;charset=utf-8,div { width: 200px; max-width: 640px!important}",
    ],
    contentStyle: [
      "div { height: 10px; }",
      "div { height: 100px; }"
    ],
    contentScript:  'new ' + function WorkerScope() {
      self.port.on('get-results', () => {
        let div = window.document.querySelector('div');
        let style = window.getComputedStyle(div);

        self.port.emit("results", {
          clientHeight: div.clientHeight,
          offsetHeight: div.offsetHeight,
          width: style.width,
          maxWidth: style.maxWidth
        });
      })
    },
    onAttach: modAttached.resolve
  });

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: URL,
      onReady: resolve
    });
  });
  assert.pass("the tab was opened");

  yield modAttached.promise;
  assert.pass("the mod has been attached");

  let results = yield new Promise(resolve => {
    pageMod.port.on("results", resolve);
    pageMod.port.emit("get-results");
  })

  assert.equal(
   results.clientHeight,
    100,
    "PageMod contentStyle list works and is evaluated after contentStyleFile"
  );

  assert.equal(
    results.offsetHeight,
    120,
    "PageMod contentStyleFile list works"
  );

  assert.equal(
    results.width,
    "320px",
    "PageMod add-on author/page author style sheet precedence works"
  );

  assert.equal(
    results.maxWidth,
    "480px",
    "PageMod add-on author/page author style sheet precedence with !important works"
  );

  pageMod.destroy();
  assert.pass("the page mod was destroyed");
};

exports.testPageModCssDestroy = function(assert, done) {
  let loader = Loader(module);

  tabs.open({
    url: "data:text/html;charset=utf-8,<div style='width:200px'>css test</div>",

    onReady: function onReady(tab) {
      let browserWindow = getMostRecentBrowserWindow();
      let win = getTabContentWindow(getActiveTab(browserWindow));

      let div = win.document.querySelector("div");
      let style = win.getComputedStyle(div);

      assert.equal(
        style.width,
        "200px",
        "PageMod contentStyle is current before page-mod applies"
      );

      let pageMod = loader.require("sdk/page-mod").PageMod({
        include: "data:*",
        contentStyle: "div { width: 100px!important; }",
        attachTo: ["top", "existing"],
        onAttach: function(worker) {
          assert.equal(
            style.width,
            "100px",
            "PageMod contentStyle worked"
          );

          worker.once('detach', () => {
            assert.equal(
              style.width,
              "200px",
              "PageMod contentStyle is removed after page-mod destroy"
            );

            tab.close(done);
          });

          pageMod.destroy();
        }
      });
    }
  });
};

exports.testPageModCssAutomaticDestroy = function(assert, done) {
 let loader = Loader(module);

  tabs.open({
    url: "data:text/html;charset=utf-8,<div style='width:200px'>css test</div>",

    onReady: function onReady(tab) {
      let browserWindow = getMostRecentBrowserWindow();
      let win = getTabContentWindow(getActiveTab(browserWindow));

      let div = win.document.querySelector("div");
      let style = win.getComputedStyle(div);

      assert.equal(
        style.width,
        "200px",
        "PageMod contentStyle is current before page-mod applies"
      );

      let pageMod = loader.require("sdk/page-mod").PageMod({
        include: "data:*",
        contentStyle: "div { width: 100px!important; }",
        attachTo: ["top", "existing"],
        onAttach: function(worker) {
          assert.equal(
            style.width,
            "100px",
            "PageMod contentStyle worked"
          );

          
          
          let pageMod2 = PageMod({
            include: "data:*",
            contentStyle: "div { width: 100px!important; }",
            attachTo: ["top", "existing"],
            onAttach: function(worker) {
              assert.equal(
                style.width,
                "200px",
                "PageMod contentStyle is removed after page-mod destroy"
              );

              pageMod2.destroy();
              tab.close(done);
            }
          });

          loader.unload();
        }
      });
    }
  });
};

exports.testPageModContentScriptFile = function(assert, done) {
  let loader = createLoader();
  let { PageMod } = loader.require("sdk/page-mod");

  tabs.open({
    url: "about:license",
    onReady: function(tab) {
      let mod = PageMod({
        include: "about:*",
        attachTo: ["existing", "top"],
        contentScriptFile: "./test-contentScriptFile.js",
        onMessage: message => {
          assert.equal(message, "msg from contentScriptFile",
            "PageMod contentScriptFile with relative path worked");
          tab.close(function() {
            mod.destroy();
            loader.unload();
            done();
          });
        }
      });
    }
  })
};

exports.testPageModTimeout = function(assert, done) {
  let tab = null
  let loader = Loader(module);
  let { PageMod } = loader.require("sdk/page-mod");

  let mod = PageMod({
    include: "data:*",
    contentScript: Isolate(function() {
      var id = setTimeout(function() {
        self.port.emit("fired", id)
      }, 10)
      self.port.emit("scheduled", id);
    }),
    onAttach: function(worker) {
      worker.port.on("scheduled", function(id) {
        assert.pass("timer was scheduled")
        worker.port.on("fired", function(data) {
          assert.equal(id, data, "timer was fired")
          tab.close(function() {
            worker.destroy()
            loader.unload()
            done()
          });
        })
      })
    }
  });

  tabs.open({
    url: "data:text/html;charset=utf-8,timeout",
    onReady: function($) { tab = $ }
  })
}


exports.testPageModcancelTimeout = function(assert, done) {
  let tab = null
  let loader = Loader(module);
  let { PageMod } = loader.require("sdk/page-mod");

  let mod = PageMod({
    include: "data:*",
    contentScript: Isolate(function() {
      var id1 = setTimeout(function() {
        self.port.emit("failed")
      }, 10)
      var id2 = setTimeout(function() {
        self.port.emit("timeout")
      }, 100)
      clearTimeout(id1)
    }),
    onAttach: function(worker) {
      worker.port.on("failed", function() {
        assert.fail("cancelled timeout fired")
      })
      worker.port.on("timeout", function(id) {
        assert.pass("timer was scheduled")
        tab.close(function() {
          worker.destroy();
          mod.destroy();
          loader.unload();
          done();
        });
      })
    }
  });

  tabs.open({
    url: "data:text/html;charset=utf-8,cancell timeout",
    onReady: function($) { tab = $ }
  })
}

exports.testExistingOnFrames = function(assert, done) {
  let subFrameURL = 'data:text/html;charset=utf-8,testExistingOnFrames-sub-frame';
  let subIFrame = '<iframe src="' + subFrameURL + '" />'
  let iFrameURL = 'data:text/html;charset=utf-8,' + encodeURIComponent(subIFrame)
  let iFrame = '<iframe src="' + iFrameURL + '" />';
  let url = 'data:text/html;charset=utf-8,' + encodeURIComponent(iFrame);

  
  
  let urls = [url, iFrameURL, subFrameURL];

  let counter = 0;
  let tab = openTab(getMostRecentBrowserWindow(), url);

  function wait4Iframes() {
    let window = getTabContentWindow(tab);
    if (window.document.readyState != "complete" ||
        getFrames(window).length != 2) {
      return;
    }

    let pagemodOnExisting = PageMod({
      include: ["*", "data:*"],
      attachTo: ["existing", "frame"],
      contentScriptWhen: 'ready',
      onAttach: function(worker) {
        
        
        if (urls.indexOf(worker.url) == -1)
          return;

        assert.notEqual(url,
                            worker.url,
                            'worker should not be attached to the top window');

        if (++counter < 2) {
          
          
          assert.equal(iFrameURL, worker.url, '1st attach is for top frame');
        }
        else if (counter > 2) {
          assert.fail('applied page mod too many times');
        }
        else {
          assert.equal(subFrameURL, worker.url, '2nd attach is for sub frame');
          
          setTimeout(function() {
            pagemodOnExisting.destroy();
            pagemodOffExisting.destroy();
            closeTab(tab);
            done();
          }, 0);
        }
      }
    });

    let pagemodOffExisting = PageMod({
      include: ["*", "data:*"],
      attachTo: ["frame"],
      contentScriptWhen: 'ready',
      onAttach: function(mod) {
        assert.fail('pagemodOffExisting page-mod should not have been attached');
      }
    });
  }

  getBrowserForTab(tab).addEventListener("load", wait4Iframes, true);
};

exports.testIFramePostMessage = function(assert, done) {
  let count = 0;

  tabs.open({
    url: data.url("test-iframe.html"),
    onReady: function(tab) {
      var worker = tab.attach({
        contentScriptFile: data.url('test-iframe.js'),
        contentScript: 'var iframePath = \'' + data.url('test-iframe-postmessage.html') + '\'',
        onMessage: function(msg) {
          assert.equal(++count, 1);
          assert.equal(msg.first, 'a string');
          assert.ok(msg.second[1], "array");
          assert.equal(typeof msg.third, 'object');

          worker.destroy();
          tab.close(done);
        }
      });
    }
  });
};

exports.testEvents = function*(assert) {
  let modAttached = defer();
  let content = "<script>\n new " + function DocumentScope() {
    window.addEventListener("ContentScriptEvent", function () {
      window.document.body.setAttribute("receivedEvent", "ok");
    }, false);
  } + "\n</script>";
  let url = "data:text/html;charset=utf-8," + encodeURIComponent(content);

  let mod = PageMod({
    include: "data:*",
    contentScript: 'new ' + function WorkerScope() {
      let evt = document.createEvent("Event");
      evt.initEvent("ContentScriptEvent", true, true);
      document.body.dispatchEvent(evt);

      self.port.on("get-result", () => {
        self.port.emit("result", {
          receivedEvent: window.document.body.getAttribute("receivedEvent")
        });
      });
    },
    onAttach: modAttached.resolve
  });

  let tab = yield new Promise(resolve => {
    tabs.open({
      url: url,
      onReady: resolve
    });
  });
  assert.pass("the tab is ready");

  yield modAttached.promise;
  assert.pass("the mod was attached")

  let result = yield new Promise(resolve => {
    mod.port.once("result", resolve);
    mod.port.emit("get-result");
  });

  assert.equal(result.receivedEvent, "ok",
               "Content script sent an event and document received it");
};

exports["test page-mod on private tab"] = function (assert, done) {
  let fail = assert.fail.bind(assert);

  let privateUri = "data:text/html;charset=utf-8," +
                   "<iframe src=\"data:text/html;charset=utf-8,frame\" />";
  let nonPrivateUri = "data:text/html;charset=utf-8,non-private";

  let pageMod = new PageMod({
    include: "data:*",
    onAttach: function(worker) {
      if (isTabPBSupported || isWindowPBSupported) {
        
        
        assert.equal(worker.tab.url,
                         nonPrivateUri,
                         "page-mod should only attach to the non-private tab");
      }

      assert.ok(!isPrivate(worker),
                  "The worker is really non-private");
      assert.ok(!isPrivate(worker.tab),
                  "The document is really non-private");
      pageMod.destroy();

      page1.close().
        then(page2.close).
        then(done, fail);
    }
  });

  let page1, page2;
  page1 = openWebpage(privateUri, true);
  page1.ready.then(function() {
    page2 = openWebpage(nonPrivateUri, false);
  }, fail);
}


exports.testWorkerTabClose = function(assert, done) {
  let callbackDone;
  testPageMod(assert, done, "about:", [{
      include: "about:",
      contentScript: '',
      onAttach: function(worker) {
        assert.pass("The page-mod was attached");

        worker.tab.close(function () {
          
          
          
          setTimeout(function () {
            assert.ok(!worker.tab,
                        "worker.tab should be null right after tab.close()");
            callbackDone();
          }, 0);
        });
      }
    }],
    function(win, done) {
      callbackDone = done;
    }
  );
};

exports.testDetachOnDestroy = function(assert, done) {
  let tab;
  const TEST_URL = 'data:text/html;charset=utf-8,detach';
  const loader = Loader(module);
  const { PageMod } = loader.require('sdk/page-mod');

  let mod1 = PageMod({
    include: TEST_URL,
    contentScript: Isolate(function() {
      self.port.on('detach', function(reason) {
        window.document.body.innerHTML += '!' + reason;
      });
    }),
    onAttach: worker => {
      assert.pass('attach[1] happened');

      worker.on('detach', _ => setTimeout(_ => {
        assert.pass('detach happened');

        let mod2 = PageMod({
          attachTo: [ 'existing', 'top' ],
          include: TEST_URL,
          contentScript: Isolate(function() {
            self.port.on('test', _ => {
              self.port.emit('result', { result: window.document.body.innerHTML});
            });
          }),
          onAttach: worker => {
            assert.pass('attach[2] happened');
            worker.port.once('result', ({ result }) => {
              assert.equal(result, 'detach!', 'the body.innerHTML is as expected');
              mod1.destroy();
              mod2.destroy();
              loader.unload();
              tab.close(done);
            });
            worker.port.emit('test');
          }
        });
      }));

      worker.destroy();
    }
  });

  tabs.open({
    url: TEST_URL,
    onOpen: t => tab = t
  })
}

exports.testDetachOnUnload = function(assert, done) {
  let tab;
  const TEST_URL = 'data:text/html;charset=utf-8,detach';
  const loader = Loader(module);
  const { PageMod } = loader.require('sdk/page-mod');

  let mod1 = PageMod({
    include: TEST_URL,
    contentScript: Isolate(function() {
      self.port.on('detach', function(reason) {
        window.document.body.innerHTML += '!' + reason;
      });
    }),
    onAttach: worker => {
      assert.pass('attach[1] happened');

      worker.on('detach', _ => setTimeout(_ => {
        assert.pass('detach happened');

        let mod2 = require('sdk/page-mod').PageMod({
          attachTo: [ 'existing', 'top' ],
          include: TEST_URL,
          contentScript: Isolate(function() {
            self.port.on('test', _ => {
              self.port.emit('result', { result: window.document.body.innerHTML});
            });
          }),
          onAttach: worker => {
            assert.pass('attach[2] happened');
            worker.port.once('result', ({ result }) => {
              assert.equal(result, 'detach!shutdown', 'the body.innerHTML is as expected');
              mod2.destroy();
              tab.close(done);
            });
            worker.port.emit('test');
          }
        });
      }));

      loader.unload('shutdown');
    }
  });

  tabs.open({
    url: TEST_URL,
    onOpen: t => tab = t
  })
}

exports.testConsole = function(assert, done) {
  let innerID;
  const TEST_URL = 'data:text/html;charset=utf-8,console';

  let seenMessage = false;

  system.on('console-api-log-event', onMessage);

  function onMessage({ subject: { wrappedJSObject: msg }}) {
    if (msg.arguments[0] !== "Hello from the page mod")
      return;
    seenMessage = true;
    innerID = msg.innerID;
  }

  let mod = PageMod({
    include: TEST_URL,
    contentScriptWhen: "ready",
    contentScript: Isolate(function() {
      console.log("Hello from the page mod");
      self.port.emit("done");
    }),
    onAttach: function(worker) {
      worker.port.on("done", function() {
        let window = getTabContentWindow(tab);
        let id = getInnerId(window);
        assert.ok(seenMessage, "Should have seen the console message");
        assert.equal(innerID, id, "Should have seen the right inner ID");

        system.off('console-api-log-event', onMessage);
        mod.destroy();
        closeTab(tab);
        done();
      });
    },
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
}

exports.testSyntaxErrorInContentScript = function *(assert) {
  const url = "data:text/html;charset=utf-8,testSyntaxErrorInContentScript";
  const loader = createLoader();
  const { PageMod } = loader.require("sdk/page-mod");
  let attached = defer();
  let errored = defer();

  let mod = PageMod({
    include: url,
    contentScript: 'console.log(23',
    onAttach: attached.resolve,
    onError: errored.resolve
  });
  openNewTab(url);

  yield attached.promise;
  let hitError = yield errored.promise;

  assert.notStrictEqual(hitError, null, "The syntax error was reported.");
  assert.equal(hitError.name, "SyntaxError", "The error thrown should be a SyntaxError");

  loader.unload();
  yield cleanUI();
};

exports.testPageShowWhenStart = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';
  let sawWorkerPageShow = false;
  let sawInjected = false;
  let sawContentScriptPageShow = false;

  let mod = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'start',
    contentScript: Isolate(function() {
      self.port.emit("injected");
      self.on("pageshow", () => {
        self.port.emit("pageshow");
      });
    }),
    onAttach: worker => {
      worker.port.on("injected", () => {
        sawInjected = true;
      });

      worker.port.on("pageshow", () => {
        sawContentScriptPageShow = true;
        closeTab(tab);
      });

      worker.on("pageshow", () => {
        sawWorkerPageShow = true;
      });

      worker.on("detach", () => {
        assert.ok(sawWorkerPageShow, "Worker emitted pageshow");
        assert.ok(sawInjected, "Content script ran");
        assert.ok(sawContentScriptPageShow, "Content script saw pageshow");
        mod.destroy();
        done();
      });
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
};

exports.testPageShowWhenReady = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';
  let sawWorkerPageShow = false;
  let sawInjected = false;
  let sawContentScriptPageShow = false;

  let mod = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'ready',
    contentScript: Isolate(function() {
      self.port.emit("injected");
      self.on("pageshow", () => {
        self.port.emit("pageshow");
      });
    }),
    onAttach: worker => {
      worker.port.on("injected", () => {
        sawInjected = true;
      });

      worker.port.on("pageshow", () => {
        sawContentScriptPageShow = true;
        closeTab(tab);
      });

      worker.on("pageshow", () => {
        sawWorkerPageShow = true;
      });

      worker.on("detach", () => {
        assert.ok(sawWorkerPageShow, "Worker emitted pageshow");
        assert.ok(sawInjected, "Content script ran");
        assert.ok(sawContentScriptPageShow, "Content script saw pageshow");
        mod.destroy();
        done();
      });
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
};

exports.testPageShowWhenEnd = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';
  let sawWorkerPageShow = false;
  let sawInjected = false;
  let sawContentScriptPageShow = false;

  let mod = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'end',
    contentScript: Isolate(function() {
      self.port.emit("injected");
      self.on("pageshow", () => {
        self.port.emit("pageshow");
      });
    }),
    onAttach: worker => {
      worker.port.on("injected", () => {
        sawInjected = true;
      });

      worker.port.on("pageshow", () => {
        sawContentScriptPageShow = true;
        closeTab(tab);
      });

      worker.on("pageshow", () => {
        sawWorkerPageShow = true;
      });

      worker.on("detach", () => {
        assert.ok(sawWorkerPageShow, "Worker emitted pageshow");
        assert.ok(sawInjected, "Content script ran");
        assert.ok(sawContentScriptPageShow, "Content script saw pageshow");
        mod.destroy();
        done();
      });
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
};


exports.testDestroyKillsChild = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';

  let mod1 = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'end',
    contentScript: Isolate(function() {
      self.port.on("ping", detail => {
        let event = document.createEvent("CustomEvent");
        event.initCustomEvent("Test:Ping", true, true, detail);
        document.dispatchEvent(event);
        self.port.emit("pingsent");
      });

      let listener = function(event) {
        self.port.emit("pong", event.detail);
      };

      self.port.on("detach", () => {
        window.removeEventListener("Test:Pong", listener);
      });
      window.addEventListener("Test:Pong", listener);
    }),
    onAttach: worker1 => {
      let mod2 = PageMod({
        include: TEST_URL,
        attachTo: ["top", "existing"],
        contentScriptWhen: 'end',
        contentScript: Isolate(function() {
          let listener = function(event) {
            let newEvent = document.createEvent("CustomEvent");
            newEvent.initCustomEvent("Test:Pong", true, true, event.detail);
            document.dispatchEvent(newEvent);
          };
          self.port.on("detach", () => {
            window.removeEventListener("Test:Ping", listener);
          })
          window.addEventListener("Test:Ping", listener);
          self.postMessage();
        }),
        onAttach: worker2 => {
          worker1.port.emit("ping", "test1");
          worker1.port.once("pong", detail => {
            assert.equal(detail, "test1", "Saw the right message");
            worker1.port.once("pingsent", () => {
              assert.pass("The message was sent");

              mod2.destroy();

              worker1.port.emit("ping", "test2");
              worker1.port.once("pong", detail => {
                assert.fail("worker2 shouldn't have responded");
              })
              worker1.port.once("pingsent", () => {
                assert.pass("The message was sent");
                mod1.destroy();
                closeTab(tab);
                done();
              });
            });
          })
        }
      });
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
}


exports.testDestroyWontAttach = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';

  let badMod = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'start',
    contentScript: Isolate(function() {
      unsafeWindow.testProperty = "attached";
    })
  });
  badMod.destroy();

  let mod = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'end',
    contentScript: Isolate(function() {
      self.postMessage(unsafeWindow.testProperty);
    }),
    onMessage: property => {
      assert.equal(property, undefined, "Shouldn't have seen the test property set.");
      mod.destroy();
      closeTab(tab);
      done();
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
}


exports.testUnloadKillsChild = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';

  let mod1 = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'end',
    contentScript: Isolate(function() {
      self.port.on("ping", detail => {
        let event = document.createEvent("CustomEvent");
        event.initCustomEvent("Test:Ping", true, true, detail);
        document.dispatchEvent(event);
        self.port.emit("pingsent");
      });

      let listener = function(event) {
        self.port.emit("pong", event.detail);
      };

      self.port.on("detach", () => {
        window.removeEventListener("Test:Pong", listener);
      });
      window.addEventListener("Test:Pong", listener);
    }),
    onAttach: worker1 => {
      let loader = Loader(module);
      let mod2 = loader.require('sdk/page-mod').PageMod({
        include: TEST_URL,
        attachTo: ["top", "existing"],
        contentScriptWhen: 'end',
        contentScript: Isolate(function() {
          let listener = function(event) {
            let newEvent = document.createEvent("CustomEvent");
            newEvent.initCustomEvent("Test:Pong", true, true, event.detail);
            document.dispatchEvent(newEvent);
          };
          self.port.on("detach", () => {
            window.removeEventListener("Test:Ping", listener);
          })
          window.addEventListener("Test:Ping", listener);
          self.postMessage();
        }),
        onAttach: worker2 => {
          worker1.port.emit("ping", "test1");
          worker1.port.once("pong", detail => {
            assert.equal(detail, "test1", "Saw the right message");
            worker1.port.once("pingsent", () => {
              assert.pass("The message was sent");

              loader.unload();

              worker1.port.emit("ping", "test2");
              worker1.port.once("pong", detail => {
                assert.fail("worker2 shouldn't have responded");
              })
              worker1.port.once("pingsent", () => {
                assert.pass("The message was sent");
                mod1.destroy();
                closeTab(tab);
                done();
              });
            });
          })
        }
      });
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
}


exports.testUnloadWontAttach = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,detach';

  let loader = Loader(module);
  let badMod = loader.require('sdk/page-mod').PageMod({
    include: TEST_URL,
    contentScriptWhen: 'start',
    contentScript: Isolate(function() {
      unsafeWindow.testProperty = "attached";
    })
  });
  loader.unload();

  let mod = PageMod({
    include: TEST_URL,
    contentScriptWhen: 'end',
    contentScript: Isolate(function() {
      self.postMessage(unsafeWindow.testProperty);
    }),
    onMessage: property => {
      assert.equal(property, undefined, "Shouldn't have seen the test property set.");
      mod.destroy();
      closeTab(tab);
      done();
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
}


exports.testDontInjectConsole = function(assert, done) {
  const TEST_URL = 'data:text/html;charset=utf-8,consoleinject';

  let loader = Loader(module);

  let mod = PageMod({
    include: TEST_URL,
    contentScript: Isolate(function() {
      
      self.postMessage((typeof unsafeWindow.console.assert) == "function");
    }),
    onMessage: isNativeConsole => {
      assert.ok(isNativeConsole, "Shouldn't have injected the SDK console.");
      mod.destroy();
      closeTab(tab);
      done();
    }
  });

  let tab = openTab(getMostRecentBrowserWindow(), TEST_URL);
}

after(exports, function*(name, assert) {
  assert.pass("cleaning ui.");
  yield cleanUI();
});

require('sdk/test').run(exports);
