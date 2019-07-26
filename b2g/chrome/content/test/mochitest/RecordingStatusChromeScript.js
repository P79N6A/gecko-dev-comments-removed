


const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
const { Services } = Cu.import('resource://gre/modules/Services.jsm');

var processId;

function peekChildId(aSubject, aTopic, aData) {
  Services.obs.removeObserver(peekChildId, 'recording-device-events');
  Services.obs.removeObserver(peekChildId, 'recording-device-ipc-events');
  let props = aSubject.QueryInterface(Ci.nsIPropertyBag2);
  if (props.hasKey('childID')) {
    processId = props.get('childID');
  }
}

addMessageListener('init-chrome-event', function(message) {
  
  let browser = Services.wm.getMostRecentWindow('navigator:browser');
  let type = message.type;
  browser.addEventListener('mozChromeEvent', function(event) {
    let details = event.detail;
    if (details.type === type) {
      sendAsyncMessage('chrome-event', details);
    }
  }, true);

  Services.obs.addObserver(peekChildId, 'recording-device-events', false);
  Services.obs.addObserver(peekChildId, 'recording-device-ipc-events', false);
});

addMessageListener('fake-content-shutdown', function(message) {
    let props = Cc["@mozilla.org/hash-property-bag;1"]
                  .createInstance(Ci.nsIWritablePropertyBag2);
    if (processId) {
      props.setPropertyAsUint64('childID', processId);
    }
    Services.obs.notifyObservers(props, 'recording-device-ipc-events', 'content-shutdown');
});
