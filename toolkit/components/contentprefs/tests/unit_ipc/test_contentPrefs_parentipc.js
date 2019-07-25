
function run_test() {
  

  var cps = Cc["@mozilla.org/content-pref/service;1"].
            createInstance(Ci.nsIContentPrefService).
            wrappedJSObject;

  var messageHandler = cps;
  
  
  
  messageHandler = cps.wrappedJSObject;
  

  
  do_check_false(messageHandler.receiveMessage({
    name: "ContentPref:getPref",
    json: { group: 'group2', name: 'name' } }).succeeded);

  
  messageHandler.receiveMessage({ name: "ContentPref:setPref",
    json: { group: 'group2', name: 'name', value: 'someValue' } });
  do_check_eq(cps.getPref('group', 'name'), undefined);

  
  do_check_true(messageHandler.receiveMessage({ name: "ContentPref:setPref",
    json: { group: 'group2', name: 'browser.upload.lastDir',
            value: 'someValue' } }).succeeded);
  do_check_eq(cps.getPref('group2', 'browser.upload.lastDir'), 'someValue');

  

  
  
  var messageProxy = {
    receiveMessage: function(aMessage) {
      if (aMessage.name == 'ContentPref:QUIT') {
        
        delete cps._mockStorage;
        delete cps._messageProxy;
        cps.setPref = cps.old_setPref;
        cps.getPref = cps.old_getPref;
        cps._dbInit = cps.old__dbInit;
        
        mM.removeMessageListener("ContentPref:setPref", messageProxy);
        mM.removeMessageListener("ContentPref:getPref", messageProxy);
        mM.removeMessageListener("ContentPref:QUIT", messageProxy);
        do_test_finished();
        return true;
      } else {
        return messageHandler.receiveMessage(aMessage);
      }
    },
  };

  var mM = Cc["@mozilla.org/parentprocessmessagemanager;1"].
           getService(Ci.nsIFrameMessageManager);
  mM.addMessageListener("ContentPref:setPref", messageProxy);
  mM.addMessageListener("ContentPref:getPref", messageProxy);
  mM.addMessageListener("ContentPref:QUIT", messageProxy);

  
  
  

  cps = cps.wrappedJSObject;
  cps._mockStorage = {};

  cps.old_setPref = cps.setPref;
  cps.setPref = function(aGroup, aName, aValue) {
    this._mockStorage[aGroup+':'+aName] = aValue;
  }

  cps.old_getPref = cps.getPref;
  cps.getPref = function(aGroup, aName) {
    return this._mockStorage[aGroup+':'+aName];
  }

  cps.old__dbInit = cps._dbInit;
  cps._dbInit = function(){};

  cps._messageProxy = messageProxy; 
  do_test_pending();

  run_test_in_child("contentPrefs_childipc.js");
}

