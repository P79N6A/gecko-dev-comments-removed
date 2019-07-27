


const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
const { Services } = Cu.import('resource://gre/modules/Services.jsm');
const { SystemAppProxy } = Cu.import('resource://gre/modules/SystemAppProxy.jsm');

addMessageListener('init-chrome-event', function(message) {
  
  let type = message.type;

  SystemAppProxy.addEventListener('mozChromeEvent', function(event) {
    let details = event.detail;
    if (details.type === type) {
      sendAsyncMessage('chrome-event', details);
    }
  }, true);
});
