





"use strict";

const { Cc, Ci, Cu } = require("chrome");
const Services = require("Services");
const { ActorPool, appendExtraActors, createExtraActors } = require("devtools/server/actors/common");
const { DebuggerServer } = require("devtools/server/main");

loader.lazyGetter(this, "ppmm", () => {
  return Cc["@mozilla.org/parentprocessmessagemanager;1"].getService(Ci.nsIMessageBroadcaster);
});











































































function RootActor(aConnection, aParameters) {
  this.conn = aConnection;
  this._parameters = aParameters;
  this._onTabListChanged = this.onTabListChanged.bind(this);
  this._onAddonListChanged = this.onAddonListChanged.bind(this);
  this._extraActors = {};

  this._globalActorPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._globalActorPool);

  this._chromeActor = null;
}

RootActor.prototype = {
  constructor: RootActor,
  applicationType: "browser",

  traits: {
    sources: true,
    
    editOuterHTML: true,
    
    
    pasteHTML: true,
    
    
    highlightable: true,
    
    
    customHighlighters: true,
    
    
    
    urlToImageDataResolver: true,
    networkMonitor: true,
    
    storageInspector: true,
    
    storageInspectorReadOnly: true,
    
    conditionalBreakpoints: true,
    
    
    debuggerSourceActors: true,
    bulk: true,
    
    
    selectorEditable: true,
    
    
    addNewRule: true,
    
    getUniqueSelector: true,
    
    directorScripts: true,
    
    
    noBlackBoxing: false,
    noPrettyPrinting: false,
    
    
    getUsedFontFaces: true,
    
    
    memoryActorAllocations: true,
    
    
    noNeedToFakeResumptionOnNavigation: true,
    
    
    
    
    
    
    get allowChromeProcess() {
      return DebuggerServer.allowChromeProcess;
    },
    
    
    profilerDataFilterable: true,
  },

  


  sayHello: function() {
    return {
      from: this.actorID,
      applicationType: this.applicationType,
      
      testConnectionPrefix: this.conn.prefix,
      traits: this.traits
    };
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
    this.conn = null;
    this._tabActorPool = null;
    this._globalActorPool = null;
    this._parameters = null;
    this._chromeActor = null;
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
        this.conn.addActorPool(this._globalActorPool);
      }
      this._createExtraActors(this._parameters.globalActorFactories, this._globalActorPool);
      



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

  onGetTab: function (options) {
    let tabList = this._parameters.tabList;
    if (!tabList) {
      return { error: "noTabs",
               message: "This root actor has no browser tabs." };
    }
    if (!this._tabActorPool) {
      this._tabActorPool = new ActorPool(this.conn);
      this.conn.addActorPool(this._tabActorPool);
    }
    return tabList.getTab(options)
                  .then(tabActor => {
      tabActor.parentID = this.actorID;
      this._tabActorPool.addActor(tabActor);

      return { tab: tabActor.form() };
    }, error => {
      if (error.error) {
        
        return error;
      } else {
        return { error: "noTab",
                 message: "Unexpected error while calling getTab(): " + error };
      }
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

  onListProcesses: function () {
    let processes = [];
    for (let i = 0; i < ppmm.childCount; i++) {
      processes.push({
        id: i, 
        parent: i == 0, 
        tabCount: undefined, 
      });
    }
    return { processes: processes };
  },

  onGetProcess: function (aRequest) {
    if (!DebuggerServer.allowChromeProcess) {
      return { error: "forbidden",
               message: "You are not allowed to debug chrome." };
    }
    if (("id" in aRequest) && typeof(aRequest.id) != "number") {
      return { error: "wrongParameter",
               message: "getProcess requires a valid `id` attribute." };
    }
    
    
    if ((!("id" in aRequest)) || aRequest.id === 0) {
      if (!this._chromeActor) {
        
        let { ChromeActor } = require("devtools/server/actors/chrome");
        this._chromeActor = new ChromeActor(this.conn);
        this._globalActorPool.addActor(this._chromeActor);
      }

      return { form: this._chromeActor.form() };
    } else {
      let mm = ppmm.getChildAt(aRequest.id);
      if (!mm) {
        return { error: "noProcess",
                 message: "There is no process with id '" + aRequest.id + "'." };
      }
      return DebuggerServer.connectToContent(this.conn, mm)
                           .then(form => ({ form }));
    }
  },

  
  onEcho: function (aRequest) {
    



    return Cu.cloneInto(aRequest, {});
  },

  onProtocolDescription: function () {
    return require("devtools/server/protocol").dumpProtocolSpec();
  },

  
  _createExtraActors: createExtraActors,
  _appendExtraActors: appendExtraActors,

  



  removeActorByName: function(aName) {
    if (aName in this._extraActors) {
      const actor = this._extraActors[aName];
      if (this._globalActorPool.has(actor)) {
        this._globalActorPool.removeActor(actor);
      }
      if (this._tabActorPool) {
        
        
        this._tabActorPool.forEach(tab => {
          tab.removeActorByName(aName);
        });
      }
      delete this._extraActors[aName];
    }
   }
};

RootActor.prototype.requestTypes = {
  "listTabs": RootActor.prototype.onListTabs,
  "getTab": RootActor.prototype.onGetTab,
  "listAddons": RootActor.prototype.onListAddons,
  "listProcesses": RootActor.prototype.onListProcesses,
  "getProcess": RootActor.prototype.onGetProcess,
  "echo": RootActor.prototype.onEcho,
  "protocolDescription": RootActor.prototype.onProtocolDescription
};

exports.RootActor = RootActor;
