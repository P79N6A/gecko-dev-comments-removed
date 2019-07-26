




const tabs = require("sdk/tabs"); 
const windowUtils = require("sdk/deprecated/window-utils");
const { getTabForWindow } = require('sdk/tabs/helpers');


var primaryTab;


var auxTab;


var iframeWin;

exports["test GetTabForWindow"] = function(assert, done) {

  assert.equal(getTabForWindow(windowUtils.activeWindow), null,
    "getTabForWindow return null on topwindow");
  assert.equal(getTabForWindow(windowUtils.activeBrowserWindow), null,
    "getTabForWindow return null on topwindow");

  let subSubDocument = encodeURIComponent(
    'Sub iframe<br/>'+
    '<iframe id="sub-sub-iframe" src="data:text/html;charset=utf-8,SubSubIframe" />');
  let subDocument = encodeURIComponent(
    'Iframe<br/>'+
    '<iframe id="sub-iframe" src="data:text/html;charset=utf-8,'+subSubDocument+'" />');
  let url = 'data:text/html;charset=utf-8,' + encodeURIComponent(
    'Content<br/><iframe id="iframe" src="data:text/html;charset=utf-8,'+subDocument+'" />');

  
  
  
  
  tabs.open({
    inBackground: true,
    url: "about:mozilla",
    onReady: function(tab) { auxTab = tab; step2(url, assert);},
    onActivate: function(tab) { step3(assert, done); }
  });
};

function step2(url, assert) {

  tabs.open({
    url: url,
    onReady: function(tab) {
      primaryTab = tab;
      let window = windowUtils.activeBrowserWindow.content;

      let matchedTab = getTabForWindow(window);
      assert.equal(matchedTab, tab,
        "We are able to find the tab with his content window object");

      let timer = require("sdk/timers");
      function waitForFrames() {
        let iframe = window.document.getElementById("iframe");
        if (!iframe) {
          timer.setTimeout(waitForFrames, 100);
          return;
        }
        iframeWin = iframe.contentWindow;
        let subIframe = iframeWin.document.getElementById("sub-iframe");
        if (!subIframe) {
          timer.setTimeout(waitForFrames, 100);
          return;
        }
        let subIframeWin = subIframe.contentWindow;
        let subSubIframe = subIframeWin.document.getElementById("sub-sub-iframe");
        if (!subSubIframe) {
          timer.setTimeout(waitForFrames, 100);
          return;
        }
        let subSubIframeWin = subSubIframe.contentWindow;

        matchedTab = getTabForWindow(iframeWin);
        assert.equal(matchedTab, tab,
          "We are able to find the tab with an iframe window object");

        matchedTab = getTabForWindow(subIframeWin);
        assert.equal(matchedTab, tab,
          "We are able to find the tab with a sub-iframe window object");

        matchedTab = getTabForWindow(subSubIframeWin);
        assert.equal(matchedTab, tab,
          "We are able to find the tab with a sub-sub-iframe window object");

        
        
        auxTab.activate();
      }
      waitForFrames();
    }
  });
}

function step3(assert, done) {

  let matchedTab = getTabForWindow(iframeWin);
  assert.equal(matchedTab, primaryTab,
    "We get the correct tab even when it's in the background");

  primaryTab.close(function () {
      auxTab.close(function () { done();});
    });
}

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
        
        tab.destroy();
        tab.destroy();

        done();
      });
    }
  });
};

if (require("sdk/system/xul-app").is("Fennec")) {
  module.exports = {
    "test Unsupported Test": function UnsupportedTest (assert) {
        assert.pass(
          "Skipping this test until Fennec support is implemented." +
          "See Bug 809362");
    }
  }
}

require("test").run(exports);
