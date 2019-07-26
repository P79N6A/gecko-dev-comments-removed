


const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
const { Services } = Cu.import('resource://gre/modules/Services.jsm');

var browser = Services.wm.getMostRecentWindow('navigator:browser');
var connection = browser.navigator.mozMobileConnections[0];


function enableDataConnection() {
  let setLock = browser.navigator.mozSettings.createLock();
  setLock.set({
    'ril.data.enabled': true,
    'ril.data.apnSettings': [
      [
        {'carrier':'T-Mobile US',
         'apn':'epc.tmobile.com',
         'mmsc':'http://mms.msg.eng.t-mobile.com/mms/wapenc',
         'types':['default','supl','mms']}
      ]
    ]
  });
}


function enableRadio() {
  if (connection.radioState !== 'enabled') {
    connection.setRadioEnabled(true);
  }
}


function disableRadio() {
  if (connection.radioState === 'enabled') {
    connection.setRadioEnabled(false);
  }
}

addMessageListener('prepare-network', function(message) {
  
  Services.obs.notifyObservers(null, 'system-message-listener-ready', null);

  connection.addEventListener('datachange', function onDataChange() {
    if (connection.data.connected) {
      connection.removeEventListener('datachange', onDataChange);
      Services.prefs.setIntPref('network.proxy.type', 2);
      sendAsyncMessage('network-ready', true);
    }
  });

  enableRadio();
  enableDataConnection();
});

addMessageListener('network-cleanup', function(message) {
  connection.addEventListener('datachange', function onDataChange() {
    if (!connection.data.connected) {
      connection.removeEventListener('datachange', onDataChange);
      Services.prefs.setIntPref('network.proxy.type', 2);
      sendAsyncMessage('network-disabled', true);
    }
  });
  disableRadio();
});
