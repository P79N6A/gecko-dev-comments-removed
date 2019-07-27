





"use strict";

const { Ci, Cu } = require("chrome");
const Services = require("Services");
const { ActorPool, appendExtraActors, createExtraActors } = require("devtools/server/actors/common");
const { DebuggerServer } = require("devtools/server/main");
const { dumpProtocolSpec } = require("devtools/server/protocol");
const makeDebugger = require("./utils/make-debugger");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "StyleSheetActor", () => {
  return require("devtools/server/actors/stylesheets").StyleSheetActor;
});











































































function RootActor(aConnection, aParameters) {
  this.conn = aConnection;
  this._parameters = aParameters;
  this._onTabListChanged = this.onTabListChanged.bind(this);
  this._onAddonListChanged = this.onAddonListChanged.bind(this);
  this._extraActors = {};

  
  this._styleSheetActors = new Map();

  
  this.makeDebugger = makeDebugger.bind(null, {
    findDebuggees: dbg => dbg.findAllGlobals(),
    shouldAddNewGlobalAsDebuggee: () => true
  });
}

RootActor.prototype = {
  constructor: RootActor,
  applicationType: "browser",

  traits: {
    sources: true,
    editOuterHTML: true,
    
    
    highlightable: true,
    
    
    customHighlighters: [
      "BoxModelHighlighter",
      "CssTransformHighlighter",
      "SelectorHighlighter"
    ],
    
    
    
    urlToImageDataResolver: true,
    networkMonitor: true,
    
    storageInspector: true,
    
    storageInspectorReadOnly: true,
    
    conditionalBreakpoints: true,
    bulk: true,
    
    
    selectorEditable: true,
    
    
    addNewRule: true
  },

  


  sayHello: function() {
    return {
      from: this.actorID,
      applicationType: this.applicationType,
      
      testConnectionPrefix: this.conn.prefix,
      traits: this.traits
    };
  },

  


  get isRootActor() true,

  


  get window() isWorker ? null : Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType),

  


  get windows() {
    return this.docShells.map(docShell => {
      return docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindow);
    });
  },

  


  get url() { return this.window ? this.window.document.location.href : null; },

  


  get docShell() {
    return this.window
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDocShell);
  },

  


  get docShells() {
    let docShellsEnum = this.docShell.getDocShellEnumerator(
      Ci.nsIDocShellTreeItem.typeAll,
      Ci.nsIDocShell.ENUMERATE_FORWARDS
    );

    let docShells = [];
    while (docShellsEnum.hasMoreElements()) {
      docShells.push(docShellsEnum.getNext());
    }

    return docShells;
  },

  


  get webProgress() {
    return this.docShell
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebProgress);
  },

  


  disconnect: function() {
    
    if (this._parameters.tabList) {
      this._parameters.tabList.onListChanged = null;
    }
    if (this._parameters.addonList) {
      this._parameters.addonList.onListChanged = null;
    }
    if (typeof this._parameters.onShutdown === 'function') {
      this._parameters.onShutdown();
    }
    this._extraActors = null;
    this._styleSheetActors.clear();
    this._styleSheetActors = null;
  },

  

  



  onListTabs: function() {
    let tabList = this._parameters.tabList;
    if (!tabList) {
      return { from: this.actorID, error: "noTabs",
               message: "This root actor has no browser tabs." };
    }

    






    let newActorPool = new ActorPool(this.conn);
    let tabActorList = [];
    let selected;
    return tabList.getList().then((tabActors) => {
      for (let tabActor of tabActors) {
        if (tabActor.selected) {
          selected = tabActorList.length;
        }
        tabActor.parentID = this.actorID;
        newActorPool.addActor(tabActor);
        tabActorList.push(tabActor);
      }

      
      if (!this._globalActorPool) {
        this._globalActorPool = new ActorPool(this.conn);
        this._createExtraActors(this._parameters.globalActorFactories, this._globalActorPool);
        this.conn.addActorPool(this._globalActorPool);
      }

      



      if (this._tabActorPool) {
        this.conn.removeActorPool(this._tabActorPool);
      }
      this._tabActorPool = newActorPool;
      this.conn.addActorPool(this._tabActorPool);

      let reply = {
        "from": this.actorID,
        "selected": selected || 0,
        "tabs": [actor.form() for (actor of tabActorList)],
      };

      
      if (this.url) {
        reply.url = this.url;
      }

      
      this._appendExtraActors(reply);

      




      tabList.onListChanged = this._onTabListChanged;

      return reply;
    });
  },

  onTabListChanged: function () {
    this.conn.send({ from: this.actorID, type:"tabListChanged" });
    
    this._parameters.tabList.onListChanged = null;
  },

  onListAddons: function () {
    let addonList = this._parameters.addonList;
    if (!addonList) {
      return { from: this.actorID, error: "noAddons",
               message: "This root actor has no browser addons." };
    }

    return addonList.getList().then((addonActors) => {
      let addonActorPool = new ActorPool(this.conn);
      for (let addonActor of addonActors) {
          addonActorPool.addActor(addonActor);
      }

      if (this._addonActorPool) {
        this.conn.removeActorPool(this._addonActorPool);
      }
      this._addonActorPool = addonActorPool;
      this.conn.addActorPool(this._addonActorPool);

      addonList.onListChanged = this._onAddonListChanged;

      return {
        "from": this.actorID,
        "addons": [addonActor.form() for (addonActor of addonActors)]
      };
    });
  },

  onAddonListChanged: function () {
    this.conn.send({ from: this.actorID, type: "addonListChanged" });
    this._parameters.addonList.onListChanged = null;
  },

  
  onEcho: function (aRequest) {
    



    return Cu.cloneInto(aRequest, {});
  },

  onProtocolDescription: dumpProtocolSpec,

  
  _createExtraActors: createExtraActors,
  _appendExtraActors: appendExtraActors,

  

  


  preNest: function() {
    
    let e = Services.wm.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.suppressEventHandling(true);
      windowUtils.suspendTimeouts();
    }
  },

  


  postNest: function(aNestData) {
    
    let e = Services.wm.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.resumeTimeouts();
      windowUtils.suppressEventHandling(false);
    }
  },

  









  createStyleSheetActor: function(styleSheet) {
    if (this._styleSheetActors.has(styleSheet)) {
      return this._styleSheetActors.get(styleSheet);
    }
    let actor = new StyleSheetActor(styleSheet, this);
    this._styleSheetActors.set(styleSheet, actor);

    this._globalActorPool.addActor(actor);

    return actor;
  }
};

RootActor.prototype.requestTypes = {
  "listTabs": RootActor.prototype.onListTabs,
  "listAddons": RootActor.prototype.onListAddons,
  "echo": RootActor.prototype.onEcho,
  "protocolDescription": RootActor.prototype.onProtocolDescription
};

exports.RootActor = RootActor;
