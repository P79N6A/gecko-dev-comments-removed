





"use strict";



Cu.import("resource://gre/modules/Services.jsm");






































function CommonCreateExtraActors(aFactories, aPool) {
  
  for (let name in aFactories) {
    let actor = this._extraActors[name];
    if (!actor) {
      actor = aFactories[name].bind(null, this.conn, this);
      actor.prototype = aFactories[name].prototype;
      actor.parentID = this.actorID;
      this._extraActors[name] = actor;
    }
    aPool.addActor(actor);
  }
}














function CommonAppendExtraActors(aObject) {
  for (let name in this._extraActors) {
    let actor = this._extraActors[name];
    aObject[name] = actor.actorID;
  }
}









































































function RootActor(aConnection, aParameters) {
  this.conn = aConnection;
  this._parameters = aParameters;
  this._onTabListChanged = this.onTabListChanged.bind(this);
  this._onAddonListChanged = this.onAddonListChanged.bind(this);
  this._extraActors = {};
}

RootActor.prototype = {
  constructor: RootActor,
  applicationType: "browser",

  


  sayHello: function() {
    return {
      from: this.actorID,
      applicationType: this.applicationType,
      
      testConnectionPrefix: this.conn.prefix,
      traits: {
        sources: true
      }
    };
  },

  


  get isRootActor() true,

  


  get window() Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType),

  


  get webProgress() {
    return this.window
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDocShell)
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

      
      this._createExtraActors(this._parameters.globalActorFactories, newActorPool);

      



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
    



    return JSON.parse(JSON.stringify(aRequest));
  },

  
  _createExtraActors: CommonCreateExtraActors,
  _appendExtraActors: CommonAppendExtraActors,

  

  


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

  

  







  addToParentPool: function(aActor) {
    this.conn.addActor(aActor);
  },

  





  removeFromParentPool: function(aActor) {
    this.conn.removeActor(aActor);
  }
}

RootActor.prototype.requestTypes = {
  "listTabs": RootActor.prototype.onListTabs,
  "listAddons": RootActor.prototype.onListAddons,
  "echo": RootActor.prototype.onEcho
};
