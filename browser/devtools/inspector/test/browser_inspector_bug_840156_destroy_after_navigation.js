


let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let Toolbox = devtools.Toolbox;
let TargetFactory = devtools.TargetFactory;

function test() {
  const URL_1 = "data:text/plain;charset=UTF-8,abcde";
  const URL_2 = "data:text/plain;charset=UTF-8,12345";

  let target, toolbox;

  
  let tab = gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let deferred = promise.defer();
  let browser = gBrowser.getBrowserForTab(tab);
  function onTabLoad() {
    browser.removeEventListener("load", onTabLoad, true);
    deferred.resolve(null);
  }
  browser.addEventListener("load", onTabLoad, true);
  browser.loadURI(URL_1);

  
  deferred.promise
    .then(function () gDevTools.showToolbox(target, null, Toolbox.HostType.BOTTOM))
    .then(function (aToolbox) { toolbox = aToolbox; })

  
    .then(function () toolbox.selectTool("inspector"))

  
    .then(function () {
      let deferred = promise.defer();
      toolbox.getPanel("inspector").once("inspector-updated", deferred.resolve);
      return deferred.promise;
    })

  
    .then(function () {
      let deferred = promise.defer();
      target.once("navigate", function () deferred.resolve());
      browser.loadURI(URL_2);
      return deferred.promise;
    })

  
    .then(function () toolbox.destroy())

  
  

    .then(function cleanUp() {
      gBrowser.removeCurrentTab();
      finish();
    });
}
