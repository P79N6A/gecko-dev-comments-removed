


"use strict";

const tabs = require("sdk/tabs"); 
const windowUtils = require("sdk/deprecated/window-utils");
const app = require("sdk/system/xul-app");
const { viewFor } = require("sdk/view/core");
const { modelFor } = require("sdk/model/core");
const { getTabId, isTab } = require("sdk/tabs/utils");
const { defer } = require("sdk/lang/functional");

exports["test behavior on close"] = function(assert, done) {
  tabs.open({
    url: "about:mozilla",
    onReady: function(tab) {
      assert.equal(tab.url, "about:mozilla", "Tab has the expected url");
      
      assert.ok(tab.index >= 1, "Tab has the expected index, a value greater than 0");
      tab.close(function () {
        assert.equal(tab.url, undefined,
                     "After being closed, tab attributes are undefined (url)");
        assert.equal(tab.index, undefined,
                     "After being closed, tab attributes are undefined (index)");
        if (app.is("Firefox")) {
          
          
          tab.destroy();
          tab.destroy();
        }

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


