


"use strict";

const { Cc, Ci } = require("chrome");
const { setTimeout } = require("sdk/timers");
const { Loader } = require("sdk/test/loader");
const { openTab, getBrowserForTab, closeTab } = require("sdk/tabs/utils");
const { merge } = require("sdk/util/object");
const httpd = require("./lib/httpd");

const PORT = 8099;
const PATH = '/test-contentScriptWhen.html';



exports.testPageMod = function testPageMod(assert, done, testURL, pageModOptions,
                                           testCallback, timeout) {

  var wm = Cc['@mozilla.org/appshell/window-mediator;1']
           .getService(Ci.nsIWindowMediator);
  var browserWindow = wm.getMostRecentWindow("navigator:browser");

  let options = merge({}, require('@loader/options'),
                      { prefixURI: require('./fixtures').url() });

  let loader = Loader(module, null, options);
  let pageMod = loader.require("sdk/page-mod");

  var pageMods = [new pageMod.PageMod(opts) for each(opts in pageModOptions)];

  let newTab = openTab(browserWindow, testURL, {
    inBackground: false
  });
  var b = getBrowserForTab(newTab);

  function onPageLoad() {
    b.removeEventListener("load", onPageLoad, true);
    
    
    
    
    setTimeout(testCallback, timeout,
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
  const loader = Loader(module);
  const { PageMod } = loader.require('sdk/page-mod');

  let pagemod = PageMod({
    include: url,
    attachTo: ['existing', 'top'],
    contentScriptWhen: contentScriptWhen,
    contentScript: "self.postMessage(document.readyState)",
    onAttach: worker => {
      let { tab } = worker;
      worker.on('message', readyState => {
        
        let type = 'on' + readyState[0].toUpperCase() + readyState.substr(1);

        if (type in callbacks)
          callbacks[type](tab); 

        pagemod.destroy();
        loader.unload();
      })
    }
  });
}



exports.contentScriptWhenServer = function() {
  const URL = 'http://localhost:' + PORT + PATH;

  const HTML = `/* polyglot js
    <script src="${URL}"></script>
    delay both the "DOMContentLoaded"
    <script async src="${URL}"></script>
    and "load" events */`;

  let srv = httpd.startServerAsync(PORT);

  srv.registerPathHandler(PATH, (_, response) => {
    response.processAsync();
    response.setHeader('Content-Type', 'text/html', false);
    setTimeout(_ => response.finish(), 500);
    response.write(HTML);
  })

  srv.URL = URL;
  return srv;
}
