




































var EXPORTED_SYMBOLS = ['PepAPI'];
var results = {}; Components.utils.import('resource://pep/results.js', results);
var log = {};     Components.utils.import('resource://pep/logger.js', log);
var utils = {};   Components.utils.import('resource://pep/utils.js', utils);
var mozmill = {}; Components.utils.import('resource://mozmill/driver/mozmill.js', mozmill);
var securableModule = {};
Components.utils.import('resource://mozmill/stdlib/securable-module.js', securableModule);

const wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
const ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);






function PepAPI(test) {
  this.test = test;
  this.log = new Log(this.test.name);
  this.resultHandler = new results.ResultHandler(this.test.name);

  this.file = Components.classes["@mozilla.org/file/local;1"]
                        .createInstance(Components.interfaces.nsILocalFile);
  this.file.initWithPath(this.test.path);
}



PepAPI.prototype.performAction = function(actionName, func) {
  this.resultHandler.startAction(actionName);
  func();
  this.resultHandler.endAction();
};



PepAPI.prototype.getWindow = function(windowType) {
  if (windowType === undefined) {
    windowType = "navigator:browser";
  }
  return wm.getMostRecentWindow(windowType);
};




PepAPI.prototype.require = function(module) {
  let loader = new securableModule.Loader({
    rootPaths: [ios.newFileURI(this.file.parent).spec],
    defaultPrincipal: "system",
    globals: { Cc: Components.classes,
               Ci: Components.interfaces,
               Cr: Components.results,
               Cu: Components.utils,
               
               
               mozmill: mozmill,
               
               elementslib: mozmill.findElement,
               findElement: mozmill.findElement,
               persisted: {},
             },
  });
  return loader.require(module);
};




PepAPI.prototype.sleep = function(milliseconds) {
  utils.sleep(milliseconds);
};




function Log(testName) {
  this.testName = testName;
}
Log.prototype.debug = function(msg) {
  log.debug(this.testName + ' | ' + msg);
};
Log.prototype.info = function(msg) {
  log.info(this.testName + ' | ' + msg);
};
Log.prototype.warning = function(msg) {
  log.warning(this.testName + ' | ' + msg);
};
Log.prototype.error = function(msg) {
  log.error(this.testName + ' | ' + msg);
};
