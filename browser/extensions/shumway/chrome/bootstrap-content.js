















'use strict';

(function contentScriptClosure() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  const Cm = Components.manager;
  const Cu = Components.utils;
  const Cr = Components.results;

  
  Cu.import('resource://gre/modules/Services.jsm');

  var isRemote = Services.appinfo.processType ===
                 Services.appinfo.PROCESS_TYPE_CONTENT;
  var isStarted = false;

  function startup() {
    if (isStarted) {
      return;
    }

    isStarted = true;
    Cu.import('resource://shumway/ShumwayBootstrapUtils.jsm');
    ShumwayBootstrapUtils.register();
  }

  function shutdown() {
    if (!isStarted) {
      return;
    }

    isStarted = false;
    ShumwayBootstrapUtils.unregister();
    Cu.unload('resource://shumway/ShumwayBootstrapUtils.jsm');
  }


  function updateSettings() {
    let mm = Cc["@mozilla.org/childprocessmessagemanager;1"]
               .getService(Ci.nsISyncMessageSender);
    var results = mm.sendSyncMessage('Shumway:Chrome:isEnabled');
    var isEnabled = results.some(function (item) {
      return item;
    });

    if (isEnabled) {
      startup();
    } else {
      shutdown();
    }
  }

  if (isRemote && typeof ShumwayBootstrapUtils !== 'undefined') {
    
    
    isRemote = false;
  }

  if (isRemote) {
    addMessageListener('Shumway:Child:refreshSettings', updateSettings);
    updateSettings();

    addMessageListener('Shumway:Child:shutdown', function shutdownListener(e) {
      removeMessageListener('Shumway:Child:refreshSettings', updateSettings);
      removeMessageListener('Shumway:Child:shutdown', shutdownListener);

      shutdown();
    });
  }
})();
