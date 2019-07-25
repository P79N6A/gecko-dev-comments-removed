
function run_test() {
  let cps = Cc["@mozilla.org/content-pref/service;1"].
            createInstance(Ci.nsIContentPrefService);

  

  let messageHandler = cps;
  
  
  
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

  

  
  
  
  
  var mockStorage = {};
  cps.wrappedJSObject.setPref = function(aGroup, aName, aValue) {
    mockStorage[aGroup+':'+aName] = aValue;
  }
  cps.wrappedJSObject.getPref = function(aGroup, aName) {
    return mockStorage[aGroup+':'+aName];
  }

  
  
  var mM = Cc["@mozilla.org/parentprocessmessagemanager;1"].
           getService(Ci.nsIFrameMessageManager);
  mM.addMessageListener("ContentPref:setPref", messageHandler);
  mM.addMessageListener("ContentPref:getPref", messageHandler);

  run_test_in_child("test_contentPrefs_childipc.js");
}

