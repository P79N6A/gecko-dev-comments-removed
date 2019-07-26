


'use strict';

const { Cc, Ci } = require('chrome');
const { Class } = require('../core/heritage');
const { tabNS } = require('./namespace');
const { EventTarget } = require('../event/target');
const { activateTab, getTabTitle, setTabTitle, closeTab, getTabURL,
        setTabURL, getOwnerWindow, getTabContentType, getTabId } = require('./utils');
const { emit } = require('../event/core');
const { when: unload } = require('../system/unload');

const { EVENTS } = require('./events');
const ERR_FENNEC_MSG = 'This method is not yet supported by Fennec';

const Tab = Class({
  extends: EventTarget,
  initialize: function initialize(options) {
    options = options.tab ? options : { tab: options };

    EventTarget.prototype.initialize.call(this, options);
    let tabInternals = tabNS(this);

    tabInternals.window = options.window || getOwnerWindow(options.tab);
    tabInternals.tab = options.tab;
  },

  




  get title() getTabTitle(tabNS(this).tab),
  set title(title) setTabTitle(tabNS(this).tab, title),

  




  get url() getTabURL(tabNS(this).tab),
  set url(url) setTabURL(tabNS(this).tab, url),

  



  get favicon() {
    
    console.error(ERR_FENNEC_MSG);

    
    return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAEklEQVQ4jWNgGAWjYBSMAggAAAQQAAF/TXiOAAAAAElFTkSuQmCC';
  },

  getThumbnail: function() {
    
    console.error(ERR_FENNEC_MSG);

    
    return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAFAAAAAtCAYAAAA5reyyAAAAJElEQVRoge3BAQEAAACCIP+vbkhAAQAAAAAAAAAAAAAAAADXBjhtAAGQ0AF/AAAAAElFTkSuQmCC';
  },

  get id() {
    return getTabId(tabNS(this).tab);
  },

  




  get index() {
    let tabs = tabNS(this).window.BrowserApp.tabs;
    let tab = tabNS(this).tab;
    for (var i = tabs.length; i >= 0; i--) {
      if (tabs[i] === tab)
        return i;
    }
    return null;
  },
  set index(value) {
    console.error(ERR_FENNEC_MSG); 
  },

  



  get isPinned() {
    console.error(ERR_FENNEC_MSG); 
    return false; 
  },
  pin: function pin() {
    console.error(ERR_FENNEC_MSG); 
  },
  unpin: function unpin() {
    console.error(ERR_FENNEC_MSG); 
  },

  




  get contentType() getTabContentType(tabNS(this).tab),

  



  attach: function attach(options) {
    
    
    let { Worker } = require('./worker');
    return Worker(options, tabNS(this).tab.browser.contentWindow);
  },

  


  activate: function activate() {
    activateTab(tabNS(this).tab, tabNS(this).window);
  },

  


  close: function close(callback) {
    if (callback)
      this.once(EVENTS.close.name, callback);

    closeTab(tabNS(this).tab);
  },

  


  reload: function reload() {
    tabNS(this).tab.browser.reload();
  }
});
exports.Tab = Tab;
