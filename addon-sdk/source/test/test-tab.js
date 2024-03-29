


"use strict";

const tabs = require("sdk/tabs");
const windowUtils = require("sdk/deprecated/window-utils");
const windows = require("sdk/windows").browserWindows;
const app = require("sdk/system/xul-app");
const { viewFor } = require("sdk/view/core");
const { modelFor } = require("sdk/model/core");
const { getTabId, isTab } = require("sdk/tabs/utils");
const { defer } = require("sdk/lang/functional");

function tabExistenceInTabs(assert, found, tab, tabs) {
  let tabFound = false;

  for each (let t in tabs) {
    assert.notEqual(t.title, undefined, 'tab title is not undefined');
    assert.notEqual(t.url, undefined, 'tab url is not undefined');
    assert.notEqual(t.index, undefined, 'tab index is not undefined');

    if (t === tab) {
      tabFound = true;
      break;
    }
  }

  
  if (found)
    assert.ok(tabFound, 'the tab was found as expected');
  else
    assert.ok(!tabFound, 'the tab was not found as expected');
}

exports.testBehaviorOnCloseAfterReady = function(assert, done) {
  tabs.open({
    url: "about:mozilla",
    onReady: function(tab) {
      assert.equal(tab.url, "about:mozilla", "Tab has the expected url");
      
      assert.ok(tab.index >= 1, "Tab has the expected index, a value greater than 0");

      let testTabFound = tabExistenceInTabs.bind(null, assert, true, tab);
      let testTabNotFound = tabExistenceInTabs.bind(null, assert, false, tab);

      
      testTabFound(tabs);
      
      let activeWindowTabs = windows.activeWindow.tabs;
      testTabFound(activeWindowTabs);

      tab.close(function () {
        assert.equal(tab.url, undefined,
                     "After being closed, tab attributes are undefined (url)");
        assert.equal(tab.index, undefined,
                     "After being closed, tab attributes are undefined (index)");

        tab.destroy();

        testTabNotFound(tabs);
        testTabNotFound(windows.activeWindow.tabs);
        testTabNotFound(activeWindowTabs);

        
        tab.destroy();

        testTabNotFound(tabs);
        testTabNotFound(windows.activeWindow.tabs);
        testTabNotFound(activeWindowTabs);

        done();
      });
    }
  });
};

exports.testBug844492 = function(assert, done) {
  const activeWindowTabs = windows.activeWindow.tabs;
  let openedTabs = 0;

  tabs.on('open', function onOpenTab(tab) {
    openedTabs++;

    let testTabFound = tabExistenceInTabs.bind(null, assert, true, tab);
    let testTabNotFound = tabExistenceInTabs.bind(null, assert, false, tab);

    testTabFound(tabs);
    testTabFound(windows.activeWindow.tabs);
    testTabFound(activeWindowTabs);

    tab.close();

    testTabNotFound(tabs);
    testTabNotFound(windows.activeWindow.tabs);
    testTabNotFound(activeWindowTabs);

    if (openedTabs >= 2) {
      tabs.removeListener('open', onOpenTab);
      done();
    }
  });

  tabs.open({ url: 'about:mozilla' });
  tabs.open({ url: 'about:mozilla' });
};

exports.testBehaviorOnCloseAfterOpen = function(assert, done) {
  tabs.open({
    url: "about:mozilla",
    onOpen: function(tab) {
      assert.notEqual(tab.url, undefined, "Tab has a url");
      assert.ok(tab.index >= 1, "Tab has the expected index");

      let testTabFound = tabExistenceInTabs.bind(null, assert, true, tab);
      let testTabNotFound = tabExistenceInTabs.bind(null, assert, false, tab);

      
      testTabFound(tabs);
      
      let activeWindowTabs = windows.activeWindow.tabs;
      testTabFound(activeWindowTabs);

      tab.close(function () {
        assert.equal(tab.url, undefined,
                     "After being closed, tab attributes are undefined (url)");
        assert.equal(tab.index, undefined,
                     "After being closed, tab attributes are undefined (index)");

        tab.destroy();

        if (app.is("Firefox")) {
          
          
          tab.destroy();
          tab.destroy();
        }

        testTabNotFound(tabs);
        testTabNotFound(windows.activeWindow.tabs);
        testTabNotFound(activeWindowTabs);

        
        tab.destroy();

        testTabNotFound(tabs);
        testTabNotFound(windows.activeWindow.tabs);
        testTabNotFound(activeWindowTabs);

        done();
      });
    }
  });
};

exports["test viewFor(tab)"] = (assert, done) => {
  
  
  
  tabs.once("open", defer(tab => {
    const view = viewFor(tab);
    assert.ok(view, "view is returned");
    assert.equal(getTabId(view), tab.id, "tab has a same id");

    tab.close(defer(done));
  }));

  tabs.open({ url: "about:mozilla" });
};


exports["test modelFor(xulTab)"] = (assert, done) => {
  tabs.open({
    url: "about:mozilla",
    onReady: tab => {
      const view = viewFor(tab);
      assert.ok(view, "view is returned");
      assert.ok(isTab(view), "view is underlaying tab");
      assert.equal(getTabId(view), tab.id, "tab has a same id");
      assert.equal(modelFor(view), tab, "modelFor(view) is SDK tab");

      tab.close(defer(done));
    }
  });
};

exports["test tab.readyState"] = (assert, done) => {
  tabs.open({
    url: "data:text/html;charset=utf-8,test_readyState",
    onOpen: (tab) => {
      assert.notEqual(["uninitialized", "loading"].indexOf(tab.readyState), -1,
        "tab is either uninitialized or loading when onOpen");
    },
    onReady: (tab) => {
      assert.notEqual(["interactive", "complete"].indexOf(tab.readyState), -1,
        "tab is either interactive or complete when onReady");
    },
    onLoad: (tab) => {
      assert.equal(tab.readyState, "complete", "tab is complete onLoad");
      tab.close(defer(done));
    }
  });
}

require("sdk/test").run(module.exports);
