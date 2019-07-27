


"use strict";

const { Cc, Ci } = require("chrome");
const { setTimeout } = require("sdk/timers");
const { Loader } = require("sdk/test/loader");
const { openTab, getBrowserForTab, closeTab } = require("sdk/tabs/utils");
const { getMostRecentBrowserWindow } = require("sdk/window/utils");
const { merge } = require("sdk/util/object");
const httpd = require("./lib/httpd");
const { cleanUI } = require("sdk/test/utils");

const PORT = 8099;
const PATH = '/test-contentScriptWhen.html';

function createLoader () {
  let options = merge({}, require('@loader/options'),
                      { prefixURI: require('./fixtures').url() });
  return Loader(module, null, options);
}
exports.createLoader = createLoader;

function openNewTab(url) {
  return openTab(getMostRecentBrowserWindow(), url, {
    inBackground: false
  });
}
exports.openNewTab = openNewTab;



function testPageMod(assert, done, testURL, pageModOptions,
                                           testCallback, timeout) {
  let loader = createLoader();
  let { PageMod } = loader.require("sdk/page-mod");
  let pageMods = [new PageMod(opts) for each (opts in pageModOptions)];
  let newTab = openNewTab(testURL);
  let b = getBrowserForTab(newTab);

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
exports.testPageMod = testPageMod;





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



function contentScriptWhenServer() {
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
exports.contentScriptWhenServer = contentScriptWhenServer;
