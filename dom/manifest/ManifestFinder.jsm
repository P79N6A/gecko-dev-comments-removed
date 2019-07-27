



'use strict';
const {
  utils: Cu
} = Components;
Cu.import('resource://gre/modules/PromiseMessage.jsm');
Cu.import('resource://gre/modules/Task.jsm');




function ManifestFinder() {}







ManifestFinder.prototype.hasManifestLink = Task.async(
  function* (aWindowOrBrowser) {
    const msgKey = 'DOM:WebManifest:hasManifestLink';
    if (!(aWindowOrBrowser && (aWindowOrBrowser.namespaceURI || aWindowOrBrowser.location))) {
      throw new TypeError('Invalid input.');
    }
    if (isXULBrowser(aWindowOrBrowser)) {
      const mm = aWindowOrBrowser.messageManager;
      const reply = yield PromiseMessage.send(mm, msgKey);
      return reply.data.result;
    }
    return checkForManifest(aWindowOrBrowser);
  }
);

function isXULBrowser(aBrowser) {
  const XUL = 'http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul';
  return (aBrowser.namespaceURI && aBrowser.namespaceURI === XUL);
}

function checkForManifest(aWindow) {
  
  if (!aWindow || aWindow.top !== aWindow) {
    return false;
  }
  const elem = aWindow.document.querySelector('link[rel~="manifest"]');
  
  if (!elem || !elem.getAttribute('href')) {
    return false;
  }
  return true;
}

this.EXPORTED_SYMBOLS = [ 
  'ManifestFinder'
];
