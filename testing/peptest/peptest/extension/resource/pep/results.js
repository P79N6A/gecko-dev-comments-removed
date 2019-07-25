




































var EXPORTED_SYMBOLS = ['ResultHandler'];

var log = {}; Components.utils.import('resource://pep/logger.js', log);
var utils = {}; Components.utils.import('resource://pep/utils.js', utils);

function ResultHandler(testName) {
  this.results = [];
  this.currentAction = {};
  this.testName = testName;
}

ResultHandler.prototype.startAction = function(actionName) {
  this.currentAction = {};
  this.currentAction['test_name'] = this.testName
  this.currentAction['action_name'] = actionName;
  this.currentAction['start_time'] = Date.now();
  log.log('ACTION-START', this.testName + ' ' + this.currentAction['action_name']);
}

ResultHandler.prototype.endAction = function() {
  if (this.currentAction['start_time']) {
    
    
    
    
    
    utils.sleep(200);
    this.currentAction['end_time'] = Date.now();
    this.results.push(this.currentAction);
    log.log('ACTION-END', this.testName + ' ' + this.currentAction['action_name']);
  }
}

ResultHandler.prototype.getResults = function() {
  return this.results
}
