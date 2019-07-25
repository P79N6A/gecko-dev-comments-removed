




































const cmdLineHandler =
        Cc["@mozilla.org/commandlinehandler/general-startup;1?type=pep"]
        .getService(Ci.nsICommandLineHandler);


var log = {};       
var utils = {};     
var broker = {};    

Components.utils.import('resource://pep/testsuite.js');
Components.utils.import('resource://pep/logger.js', log);
Components.utils.import('resource://pep/utils.js', utils);
Components.utils.import('resource://mozmill/driver/msgbroker.js', broker);

var APPCONTENT;





function initialize() {
  window.removeEventListener("load", initialize, false);
  let cmd = cmdLineHandler.wrappedJSObject;
  
  
  if (cmd.firstRun) {
    cmd.firstRun = false;
    try {
      
      let manifest = cmd.manifest;
      let data = utils.readFile(manifest);
      let obj = JSON.parse(data.join(' '));

      
      broker.addObject(new MozmillMsgListener());

      
      APPCONTENT = document.getElementById('appcontent');
      function runTests() {
        APPCONTENT.removeEventListener('pageshow', runTests);
        suite = new TestSuite(obj.tests);
        suite.run();
        goQuitApplication();
      };
      APPCONTENT.addEventListener('pageshow', runTests);
    } catch(e) {
      log.error(e.toString());
      log.debug('Traceback:');
      lines = e.stack.split('\n');
      for (let i = 0; i < lines.length - 1; ++i) {
        log.debug('\t' + lines[i]);
      }
      goQuitApplication();
    }
  }
};




function MozmillMsgListener() {}
MozmillMsgListener.prototype.pass = function(obj) {
  log.debug('MOZMILL pass ' + JSON.stringify(obj) + '\n');
};
MozmillMsgListener.prototype.fail = function(obj) {
  
  log.warning('MOZMILL fail ' + JSON.stringify(obj) + '\n');
};
MozmillMsgListener.prototype.log = function(obj) {
  log.debug('MOZMILL log ' + JSON.stringify(obj) + '\n');
};


window.addEventListener("load", initialize, false);
