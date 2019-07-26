


'use strict';

module.metadata = {
  'stability': 'experimental',
  'engines': {
    'Firefox': '*'
  }
};

const { Class } = require('../core/heritage');
const { merge } = require('../util/object');
const { Disposable } = require('../core/disposable');
const { off, emit, setListeners } = require('../event/core');
const { EventTarget } = require('../event/target');
const { URL } = require('../url');
const { add, remove, has, clear, iterator } = require('../lang/weak-set');
const { WindowTracker } = require('../deprecated/window-utils');
const { isShowing } = require('./sidebar/utils');
const { isBrowser, getMostRecentBrowserWindow, windows, isWindowPrivate } = require('../window/utils');
const { ns } = require('../core/namespace');
const { remove: removeFromArray } = require('../util/array');
const { show, hide, toggle } = require('./sidebar/actions');
const { Worker: WorkerTrait } = require('../content/worker');
const { contract: sidebarContract } = require('./sidebar/contract');
const { create, dispose, updateTitle, updateURL, isSidebarShowing, showSidebar, hideSidebar } = require('./sidebar/view');
const { defer } = require('../core/promise');
const { models, views, viewsFor, modelFor } = require('./sidebar/namespace');
const { isLocalURL } = require('../url');
const { ensure } = require('../system/unload');

const Worker = WorkerTrait.resolve({
  _injectInDocument: '__injectInDocument'
}).compose({
  get _injectInDocument() true
});

const sidebarNS = ns();

const WEB_PANEL_BROWSER_ID = 'web-panels-browser';

let sidebars = {};

const Sidebar = Class({
  implements: [ Disposable ],
  extends: EventTarget,
  setup: function(options) {
    let model = sidebarContract(options);
    models.set(this, model);

    validateTitleAndURLCombo({}, this.title, this.url);

    const self = this;
    const internals = sidebarNS(self);
    const windowNS = internals.windowNS = ns();

    
    ensure(this, 'destroy');

    setListeners(this, options);

    let bars = [];
    internals.tracker = WindowTracker({
      onTrack: function(window) {
        if (!isBrowser(window))
          return;

        let sidebar = window.document.getElementById('sidebar');
        let sidebarBox = window.document.getElementById('sidebar-box');

        let bar = create(window, {
          id: self.id,
          title: self.title,
          sidebarurl: self.url
        });
        bars.push(bar);
        windowNS(window).bar = bar;

        bar.addEventListener('command', function() {
          if (isSidebarShowing(window, self)) {
            hideSidebar(window, self);
            return;
          }

          showSidebar(window, self);
        }, false);

        function onSidebarLoad() {
          
          let isReady = sidebar.docShell && sidebar.contentDocument;
          if (!isReady)
            return;

          
          let panelBrowser = sidebar.contentDocument.getElementById(WEB_PANEL_BROWSER_ID);
          if (!panelBrowser) {
            bar.removeAttribute('checked');
            return;
          }

          let sbTitle = window.document.getElementById('sidebar-title');
          function onWebPanelSidebarCreated() {
            if (panelBrowser.contentWindow.location != model.url ||
                sbTitle.value != model.title) {
              return;
            }

            let worker = windowNS(window).worker = Worker({
              window: panelBrowser.contentWindow
            });

            function onWebPanelSidebarUnload() {
              windowNS(window).onWebPanelSidebarUnload = null;

              
              bar.setAttribute('checked', 'false');

              emit(self, 'hide', {});
              emit(self, 'detach', worker);
            }
            windowNS(window).onWebPanelSidebarUnload = onWebPanelSidebarUnload;
            panelBrowser.contentWindow.addEventListener('unload', onWebPanelSidebarUnload, true);

            
            bar.setAttribute('checked', 'true');

            function onWebPanelSidebarLoad() {
              panelBrowser.contentWindow.removeEventListener('load', onWebPanelSidebarLoad, true);
              windowNS(window).onWebPanelSidebarLoad = null;

              
              
              emit(self, 'show', {});
            }
            windowNS(window).onWebPanelSidebarLoad = onWebPanelSidebarLoad;
            panelBrowser.contentWindow.addEventListener('load', onWebPanelSidebarLoad, true);

            emit(self, 'attach', worker);
          }
          windowNS(window).onWebPanelSidebarCreated = onWebPanelSidebarCreated;
          panelBrowser.addEventListener('DOMWindowCreated', onWebPanelSidebarCreated, true);
        }
        windowNS(window).onSidebarLoad = onSidebarLoad;
        sidebar.addEventListener('load', onSidebarLoad, true); 
      },
      onUntrack: function(window) {
        if (!isBrowser(window))
          return;

        
        hideSidebar(window, self);

        
        let { bar } = windowNS(window);
        if (bar) {
          removeFromArray(viewsFor(self), bar);
          dispose(bar);
        }

        
        let sidebar = window.document.getElementById('sidebar');

        if (windowNS(window).onSidebarLoad) {
          sidebar && sidebar.removeEventListener('load', windowNS(window).onSidebarLoad, true)
          windowNS(window).onSidebarLoad = null;
        }

        let panelBrowser = sidebar && sidebar.contentDocument.getElementById(WEB_PANEL_BROWSER_ID);
        if (windowNS(window).onWebPanelSidebarCreated) {
          panelBrowser && panelBrowser.removeEventListener('DOMWindowCreated', windowNS(window).onWebPanelSidebarCreated, true);
          windowNS(window).onWebPanelSidebarCreated = null;
        }

        if (windowNS(window).onWebPanelSidebarLoad) {
          panelBrowser && panelBrowser.contentWindow.removeEventListener('load', windowNS(window).onWebPanelSidebarLoad, true);
          windowNS(window).onWebPanelSidebarLoad = null;
        }

        if (windowNS(window).onWebPanelSidebarUnload) {
          panelBrowser && panelBrowser.contentWindow.removeEventListener('unload', windowNS(window).onWebPanelSidebarUnload, true);
          windowNS(window).onWebPanelSidebarUnload = null;
        }
      }
    });

    views.set(this, bars);

    add(sidebars, this);
  },
  get id() (modelFor(this) || {}).id,
  get title() (modelFor(this) || {}).title,
  set title(v) {
    
    if (!modelFor(this))
      return;
    
    if (typeof v != 'string')
      throw Error('title must be a string');
    validateTitleAndURLCombo(this, v, this.url);
    
    updateTitle(this, v);
    return modelFor(this).title = v;
  },
  get url() (modelFor(this) || {}).url,
  set url(v) {
    
    if (!modelFor(this))
      return;
    
    if (!isLocalURL(v))
      throw Error('the url must be a valid local url');
    validateTitleAndURLCombo(this, this.title, v);
    
    updateURL(this, v);
    modelFor(this).url = v;
  },
  show: function() {
    return showSidebar(null, this);
  },
  hide: function() {
    return hideSidebar(null, this);
  },
  dispose: function() {
    const internals = sidebarNS(this);

    off(this);

    remove(sidebars, this);

    
    internals.tracker.unload();
    internals.tracker = null;

    internals.windowNS = null;

    views.delete(this);
    models.delete(this);
  }
});
exports.Sidebar = Sidebar;

function validateTitleAndURLCombo(sidebar, title, url) {
  if (sidebar.title == title && sidebar.url == url) {
    return false;
  }

  for (let window of windows(null, { includePrivate: true })) {
    let sidebar = window.document.querySelector('menuitem[sidebarurl="' + url + '"][label="' + title + '"]');
    if (sidebar) {
      throw Error('The provided title and url combination is invalid (already used).');
    }
  }

  return false;
}

isShowing.define(Sidebar, isSidebarShowing.bind(null, null));
show.define(Sidebar, showSidebar.bind(null, null));
hide.define(Sidebar, hideSidebar.bind(null, null));

function toggleSidebar(window, sidebar) {
  
  window = window || getMostRecentBrowserWindow();
  if (isSidebarShowing(window, sidebar)) {
    return hideSidebar(window, sidebar);
  }
  return showSidebar(window, sidebar);
}
toggle.define(Sidebar, toggleSidebar.bind(null, null));
