




































var EXPORTED_SYMBOLS = ['PepAPI'];
var results = {}; Components.utils.import('resource://pep/results.js', results);
var log = {}; Components.utils.import('resource://pep/logger.js', log);
var utils = {}; Components.utils.import('resource://pep/utils.js', utils);

const wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                            .getService(Components.interfaces.nsIWindowMediator);





function PepAPI(test) {
  this.test = test;
  this.log = new Log(this.test.name);
  this.resultHandler = new results.ResultHandler(this.test.name);
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
