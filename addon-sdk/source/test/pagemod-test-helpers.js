



"use strict";

const {Cc,Ci} = require("chrome");
const timer = require("sdk/timers");
const xulApp = require("sdk/system/xul-app");
const { Loader } = require("sdk/test/loader");
const { openTab, getBrowserForTab, closeTab } = require("sdk/tabs/utils");
const self = require("sdk/self");
const { merge } = require("sdk/util/object");





exports.testPageMod = function testPageMod(assert, done, testURL, pageModOptions,
                                           testCallback, timeout) {
  if (!xulApp.versionInRange(xulApp.platformVersion, "1.9.3a3", "*") &&
      !xulApp.versionInRange(xulApp.platformVersion, "1.9.2.7", "1.9.2.*")) {
    assert.pass("Note: not testing PageMod, as it doesn't work on this platform version");
    return null;
  }

  var wm = Cc['@mozilla.org/appshell/window-mediator;1']
           .getService(Ci.nsIWindowMediator);
  var browserWindow = wm.getMostRecentWindow("navigator:browser");
  if (!browserWindow) {
    assert.pass("page-mod tests: could not find the browser window, so " +
              "will not run. Use -a firefox to run the pagemod tests.")
    return null;
  }

  let loader = Loader(module, null, null, {
    modules: {
      "sdk/self": merge({}, self, {
        data: merge({}, self.data, require("./fixtures"))
      })
    }
  });
  let pageMod = loader.require("sdk/page-mod");

  var pageMods = [new pageMod.PageMod(opts) for each(opts in pageModOptions)];

  let newTab = openTab(browserWindow, testURL, {
    inBackground: false
  });
  var b = getBrowserForTab(newTab);

  function onPageLoad() {
    b.removeEventListener("load", onPageLoad, true);
    
    
    
    
    timer.setTimeout(testCallback, 0,
      b.contentWindow.wrappedJSObject, 
      function () {
        pageMods.forEach(function(mod) mod.destroy());
        
        closeTab(newTab);
        loader.unload();
        done();
      }
    );
  }
  b.addEventListener("load", onPageLoad, true);

  return pageMods;
}





exports.handleReadyState = function(url, contentScriptWhen, callbacks) {
  const { PageMod } = Loader(module).require('sdk/page-mod');

  let pagemod = PageMod({
    include: url,
    attachTo: ['existing', 'top'],
    contentScriptWhen: contentScriptWhen,
    contentScript: "self.postMessage(document.readyState)",
    onAttach: worker => {
      let { tab } = worker;
      worker.on('message', readyState => {
        pagemod.destroy();
        
        let type = 'on' + readyState[0].toUpperCase() + readyState.substr(1);

        if (type in callbacks)
          callbacks[type](tab); 
      })
    }
  });
}
