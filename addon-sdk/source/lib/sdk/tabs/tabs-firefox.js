


'use strict';


const { browserWindows: windows } = require('../windows');
const { tabs } = require('../windows/tabs-firefox');
const { isPrivate } = require('../private-browsing');
const { isWindowPBSupported } = require('../private-browsing/utils')
const { isPrivateBrowsingSupported } = require('sdk/self');

const supportPrivateTabs = isPrivateBrowsingSupported && isWindowPBSupported;

Object.defineProperties(tabs, {
  open: { value: function open(options) {
    if (options.inNewWindow) {
        
        windows.open({
          tabs: [ options ],
          isPrivate: options.isPrivate
        });
        return undefined;
    }
    


    let activeWindow = windows.activeWindow;
    let privateState = !!options.isPrivate;
    
    if (!supportPrivateTabs || privateState === isPrivate(activeWindow)) {
      activeWindow.tabs.open(options);
    }
    else {
      
      let window = getWindow(privateState);
      if (window) {
      	window.tabs.open(options);
      }
      
      else {
        windows.open({
          tabs: [ options ],
          isPrivate: options.isPrivate
        });
      }
    }

    return undefined;
  }}
});

function getWindow(privateState) {
  for each (let window in windows) {
  	if (privateState === isPrivate(window)) {
  	  return window;
  	}
  }
  return null;
}





module.exports = Object.create(tabs, {
  isPrototypeOf: { value: Object.prototype.isPrototypeOf }
});
