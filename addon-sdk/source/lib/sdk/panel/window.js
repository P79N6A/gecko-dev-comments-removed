


'use strict';



module.metadata = {
  'stability': 'unstable',
  'engines': {
    'Firefox': '*'
  }
};

const { getMostRecentBrowserWindow, windows: getWindows } = require('../window/utils');
const { ignoreWindow } = require('../private-browsing/utils');
const { isPrivateBrowsingSupported } = require('../self');

function getWindow(anchor) {
  let window;
  let windows = getWindows("navigator:browser", {
    includePrivate: isPrivateBrowsingSupported
  });

  if (anchor) {
    let anchorWindow = anchor.ownerDocument.defaultView.top;
    let anchorDocument = anchorWindow.document;

    
    for (let enumWindow of windows) {
      
      if (enumWindow == anchorWindow) {
        window = anchorWindow;
        break;
      }

      
      try {
        let browser = enumWindow.gBrowser.getBrowserForDocument(anchorDocument);
        if (browser) {
          window = enumWindow;
          break;
        }
      }
      catch (e) {
      }

      
    }
  }

  
  
  if (!window)
    window = getMostRecentBrowserWindow();

  
  if (ignoreWindow(window)) {
  	return null;
  }

  return window;
}
exports.getWindow = getWindow;
