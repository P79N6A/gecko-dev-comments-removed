


'use strict';


const { browserWindows } = require('../windows');
const { tabs } = require('../windows/tabs-firefox');

Object.defineProperties(tabs, {
  open: { value: function open(options) {
    if (options.inNewWindow)
        
        return browserWindows.open({ tabs: [ options ] });
    
    return browserWindows.activeWindow.tabs.open(options);
  }}
});





module.exports = Object.create(tabs, {
  isPrototypeOf: { value: Object.prototype.isPrototypeOf }
});
