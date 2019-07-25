




































var EXPORTED_SYMBOLS = ['TestSuite'];

const gIOS = Components.classes['@mozilla.org/network/io-service;1']
                               .getService(Components.interfaces.nsIIOService);
const scriptLoader = Components.classes['@mozilla.org/moz/jssubscript-loader;1']
                       .getService(Components.interfaces.mozIJSSubScriptLoader);

var api = {}; 
var log = {};
var utils = {};
Components.utils.import('resource://pep/api.js', api);
Components.utils.import('resource://pep/logger.js', log);
Components.utils.import('resource://pep/utils.js', utils);








function TestSuite(tests) {
  this.tests = tests;
}

TestSuite.prototype.run = function() {
  for (let i = 0; i < this.tests.length; ++i) {
    this.loadTest(this.tests[i]);
    
    
    
    utils.sleep(1000);
  }
  log.info('Test Suite Finished');
};





TestSuite.prototype.loadTest = function(test) {
  let file = Components.classes['@mozilla.org/file/local;1']
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(test.path);
  let uri = gIOS.newFileURI(file).spec;

  try {
    let testScope = {
      pep: new api.PepAPI(test)
    };

    
    log.log('TEST-START', test.name);
    let startTime = Date.now();

    
    scriptLoader.loadSubScript(uri, testScope);

    
    let runTime = Date.now() - startTime;
    let fThreshold = test['failThreshold'] === undefined ?
                          '' : ' ' + test['failThreshold'];
    log.log('TEST-END', test.name + ' ' + runTime + fThreshold);
  } catch (e) {
    log.error(test.name + ' | ' + e);
    log.debug(test.name + ' | Traceback:');
    lines = e.stack.split('\n');
    for (let i = 0; i < lines.length - 1; ++i) {
      log.debug('\t' + lines[i]);
    }
  }
};
