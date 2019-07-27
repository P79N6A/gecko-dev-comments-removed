





"use strict";

const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");

const { Cu, Ci } = require("chrome");

const { on, once, off, emit } = events;
const { method, Arg, Option, RetVal, types } = protocol;

const { sandbox, evaluate } = require('sdk/loader/sandbox');
const { Class } = require("sdk/core/heritage");

const { PlainTextConsole } = require('sdk/console/plain-text');

const { DirectorRegistry } = require("./director-registry");





const ERR_MESSAGEPORT_FINALIZED = "message port finalized";

const ERR_DIRECTOR_UNKNOWN_SCRIPTID = "unkown director-script id";
const ERR_DIRECTOR_UNINSTALLED_SCRIPTID = "uninstalled director-script id";




types.addDictType("messageportevent", {
  isTrusted: "boolean",
  data: "nullable:primitive",
  origin: "nullable:string",
  lastEventId: "nullable:string",
  source: "messageport",
  ports: "nullable:array:messageport"
});





let MessagePortActor = exports.MessagePortActor = protocol.ActorClass({
  typeName: "messageport",

  







  initialize: function(conn, port) {
    protocol.Actor.prototype.initialize.call(this, conn);

    
    
    this.port = port;
  },

  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  





  postMessage: method(function (msg) {
    if (!this.port) {
      console.error(ERR_MESSAGEPORT_FINALIZED);
      return;
    }

    this.port.postMessage(msg);
  }, {
    oneway: true,
    request: {
      msg: Arg(0, "nullable:json")
    }
  }),

  


  start: method(function () {
    if (!this.port) {
      console.error(ERR_MESSAGEPORT_FINALIZED);
      return;
    }

    
    
    
    
    
    
    this.port.onmessage = (evt) => {
      var ports;

      
      if (Array.isArray(evt.ports)) {
        ports = evt.ports.map((port) => {
          let actor = new MessagePortActor(this.conn, port);
          this.manage(actor);
          return actor;
        });
      }

      emit(this, "message", {
        isTrusted: evt.isTrusted,
        data: evt.data,
        origin: evt.origin,
        lastEventId: evt.lastEventId,
        source: this,
        ports: ports
      });
    };
  }, {
    oneway: true,
    request: {}
  }),

  



  close: method(function () {
    if (!this.port) {
      console.error(ERR_MESSAGEPORT_FINALIZED);
      return;
    }

    this.port.onmessage = null;
    this.port.close();
  }, {
    oneway: true,
    request: {}
  }),

  finalize: method(function () {
    this.close();
    this.port = null;
  }, {
    oneway: true
  }),

  


  events: {
    "message": {
      type: "message",
      msg: Arg(0, "nullable:messageportevent")
    }
  }
});




let MessagePortFront = exports.MessagePortFront = protocol.FrontClass(MessagePortActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  }
});





types.addDictType("director-script-error", {
  directorScriptId: "string",
  message: "nullable:string",
  stack: "nullable:string",
  fileName: "nullable:string",
  lineNumber: "nullable:number",
  columnNumber: "nullable:number"
});




types.addDictType("director-script-attach", {
  directorScriptId: "string",
  url: "string",
  innerId: "number",
  port: "nullable:messageport"
});




types.addDictType("director-script-detach", {
  directorScriptId: "string",
  innerId: "number"
});












let DirectorScriptActor = exports.DirectorScriptActor = protocol.ActorClass({
  typeName: "director-script",

  


  events: {
    "error": {
      type: "error",
      data: Arg(0, "director-script-error")
    },
    "attach": {
      type: "attach",
      data: Arg(0, "director-script-attach")
    },
    "detach": {
      type: "detach",
      data: Arg(0, "director-script-detach")
    }
  },

  













  initialize: function(conn, tabActor, { scriptId, scriptCode, scriptOptions }) {
    protocol.Actor.prototype.initialize.call(this, conn, tabActor);

    this.tabActor = tabActor;

    this._scriptId = scriptId;
    this._scriptCode = scriptCode;
    this._scriptOptions = scriptOptions;
    this._setupCalled = false;

    this._onGlobalCreated   = this._onGlobalCreated.bind(this);
    this._onGlobalDestroyed = this._onGlobalDestroyed.bind(this);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);

    this.finalize();
  },

  









  setup: method(function ({ reload, skipAttach }) {
    if (this._setupCalled) {
      
      return;
    }

    this._setupCalled = true;

    on(this.tabActor, "window-ready", this._onGlobalCreated);
    on(this.tabActor, "window-destroyed", this._onGlobalDestroyed);

    
    if (skipAttach) {
      return;
    }

    if (reload) {
      this.window.location.reload();
    } else {
      
      this._onGlobalCreated({ id: getWindowID(this.window), window: this.window, isTopLevel: true });
    }
  }, {
    request: {
      reload: Option(0, "boolean"),
      skipAttach: Option(0, "boolean")
    },
    oneway: true
  }),

  


  getMessagePort: method(function () {
    return this._messagePortActor;
  }, {
    request: { },
    response: {
      port: RetVal("nullable:messageport")
    }
  }),

  



  finalize: method(function () {
    if (!this._setupCalled) {
      return;
    }

    off(this.tabActor, "window-ready", this._onGlobalCreated);
    off(this.tabActor, "window-destroyed", this._onGlobalDestroyed);

    this._onGlobalDestroyed({ id: this._lastAttachedWinId });

    this._setupCalled = false;
  }, {
    oneway: true
  }),

  
  get window() {
    return this.tabActor.window;
  },

  
  _onGlobalCreated: function({ id, window, isTopLevel }) {
    if (!isTopLevel) {
      
      return;
    }

    try {
      if (this._lastAttachedWinId) {
        
        
        this._onGlobalDestroyed(this._lastAttachedWinId);
      }

      
      

      
      this._scriptSandbox = new DirectorScriptSandbox({
        scriptId: this._scriptId,
        scriptCode: this._scriptCode,
        scriptOptions: this._scriptOptions
      });

      
      this._lastAttachedWinId = id;
      var port = this._scriptSandbox.attach(window, id);
      this._onDirectorScriptAttach(window, port);
    } catch(e) {
      this._onDirectorScriptError(e);
    }
  },
  _onGlobalDestroyed: function({ id }) {
     if (id !== this._lastAttachedWinId) {
       
       return;
     }

     
     if (this._messagePortActor) {
       this.unmanage(this._messagePortActor);
       this._messagePortActor = null;
     }

     
     if (this._scriptSandbox) {
       this._scriptSandbox.destroy(this._onDirectorScriptError.bind(this));

       
       emit(this, "detach", {
         directorScriptId: this._scriptId,
         innerId: this._lastAttachedWinId
       });

       this._lastAttachedWinId = null;
       this._scriptSandbox = null;
     }
  },
  _onDirectorScriptError: function(error) {
    
    if (error) {
      
      console.error("director-script-error", error);
      
      emit(this, "error", {
        directorScriptId: this._scriptId,
        message: error.toString(),
        stack: error.stack,
        fileName: error.fileName,
        lineNumber: error.lineNumber,
        columnNumber: error.columnNumber
      });
    }
  },
  _onDirectorScriptAttach: function(window, port) {
    let portActor = new MessagePortActor(this.conn, port);
    this.manage(portActor);
    this._messagePortActor = portActor;

    emit(this, "attach", {
      directorScriptId: this._scriptId,
      url: (window && window.location) ? window.location.toString() : "",
      innerId: this._lastAttachedWinId,
      port: this._messagePortActor
    });
  }
});




let DirectorScriptFront = exports.DirectorScriptFront = protocol.FrontClass(DirectorScriptActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  }
});




const DirectorManagerActor = exports.DirectorManagerActor = protocol.ActorClass({
  typeName: "director-manager",

  


  events: {
    "director-script-error": {
      type: "error",
      data: Arg(0, "director-script-error")
    },
    "director-script-attach": {
      type: "attach",
      data: Arg(0, "director-script-attach")
    },
    "director-script-detach": {
      type: "detach",
      data: Arg(0, "director-script-detach")
    }
  },

  
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._directorScriptActorsMap = new Map();
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  


  list: method(function () {
    var enabled_script_ids = [for (id of this._directorScriptActorsMap.keys()) id];

    return {
      installed: DirectorRegistry.list(),
      enabled: enabled_script_ids
    };
  }, {
    response: {
      directorScripts: RetVal("json")
    }
  }),

  








  enableByScriptIds: method(function(selectedIds, { reload }) {
    if (selectedIds && selectedIds.length === 0) {
      
      return;
    }

    for (let scriptId of DirectorRegistry.list()) {
      
      if (selectedIds.indexOf("*") < 0 &&
          selectedIds.indexOf(scriptId) < 0) {
        continue;
      }

      let actor = this.getByScriptId(scriptId);

      
      
      actor.setup({ reload: false, skipAttach: reload });
    }

    if (reload) {
      this.tabActor.window.location.reload();
    }
  }, {
    oneway: true,
    request: {
      selectedIds: Arg(0, "array:string"),
      reload: Option(1, "boolean")
    }
  }),

  








  disableByScriptIds: method(function(selectedIds, { reload }) {
    if (selectedIds && selectedIds.length === 0) {
      
      return;
    }

    for (let scriptId of this._directorScriptActorsMap.keys()) {
      
      if (selectedIds.indexOf("*") < 0 &&
          selectedIds.indexOf(scriptId) < 0) {
        continue;
      }

      let actor = this._directorScriptActorsMap.get(scriptId);
      this._directorScriptActorsMap.delete(scriptId);

      
      actor.finalize();
      
      off(actor);

      this.unmanage(actor);
    }

    if (reload) {
      this.tabActor.window.location.reload();
    }
  }, {
    oneway: true,
    request: {
      selectedIds: Arg(0, "array:string"),
      reload: Option(1, "boolean")
    }
  }),

  



  getByScriptId: method(function(scriptId) {
    var id = scriptId;
    
    if (!DirectorRegistry.checkInstalled(id)) {
      console.error(ERR_DIRECTOR_UNKNOWN_SCRIPTID, id);
      throw Error(ERR_DIRECTOR_UNKNOWN_SCRIPTID);
    }

    
    let actor = this._directorScriptActorsMap.get(id);

    
    if (!actor) {
      let directorScriptDefinition = DirectorRegistry.get(id);

      
      if (!directorScriptDefinition) {

        console.error(ERR_DIRECTOR_UNINSTALLED_SCRIPTID, id);
        throw Error(ERR_DIRECTOR_UNINSTALLED_SCRIPTID);
      }

      actor = new DirectorScriptActor(this.conn, this.tabActor, directorScriptDefinition);
      this._directorScriptActorsMap.set(id, actor);

      on(actor, "error", emit.bind(null, this, "director-script-error"));
      on(actor, "attach", emit.bind(null, this, "director-script-attach"));
      on(actor, "detach", emit.bind(null, this, "director-script-detach"));

      this.manage(actor);
    }

    return actor;
  }, {
    request: {
      scriptId: Arg(0, "string")
    },
    response: {
      directorScript: RetVal("director-script")
    }
  }),

  finalize: method(function() {
    this.disableByScriptIds(["*"], false);
  }, {
    oneway: true
  })
});




exports.DirectorManagerFront = protocol.FrontClass(DirectorManagerActor, {
  initialize: function(client, { directorManagerActor }) {
    protocol.Front.prototype.initialize.call(this, client, {
      actor: directorManagerActor
    });
    this.manage(this);
  }
});







const DirectorScriptSandbox = Class({
  initialize: function({scriptId, scriptCode, scriptOptions}) {
    this._scriptId = scriptId;
    this._scriptCode = scriptCode;
    this._scriptOptions = scriptOptions;
  },

  attach: function(window, innerId) {
    this._innerId = innerId,
    this._window = window;
    this._proto = Cu.createObjectIn(this._window);

    var id = this._scriptId;
    var uri = this._scriptCode;

    this._sandbox = sandbox(window, {
      sandboxName: uri,
      sandboxPrototype: this._proto,
      sameZoneAs: window,
      wantXrays: true,
      wantComponents: false,
      wantExportHelpers: false,
      metadata: {
        URI: uri,
        addonID: id,
        SDKDirectorScript: true,
        "inner-window-id": innerId
      }
    });

    
    
    var module = Cu.cloneInto(Object.create(null, {
      id: { enumerable: true, value: id },
      uri: { enumerable: true, value: uri },
      exports: { enumerable: true, value: Cu.createObjectIn(this._sandbox) }
    }), this._sandbox);

    
    let directorScriptConsole = new PlainTextConsole(null, this._innerId);

    
    Object.defineProperties(this._proto, {
      module: { enumerable: true, value: module },
      exports: { enumerable: true, value: module.exports },
      console: {
        enumerable: true,
        value: Cu.cloneInto(directorScriptConsole, this._sandbox, { cloneFunctions: true })
      }
    });

    Object.defineProperties(this._sandbox, {
      require: {
        enumerable: true,
        value: Cu.cloneInto(function() {
          throw Error("NOT IMPLEMENTED");
        }, this._sandbox, { cloneFunctions: true })
      }
    });

    
    

    
    evaluate(this._sandbox, this._scriptCode, 'javascript:' + this._scriptCode);

    
    let { port1, port2 } = new this._window.MessageChannel();

    
    var sandboxOnUnloadQueue = this._sandboxOnUnloadQueue = [];

    
    var attachOptions = this._attachOptions = Cu.createObjectIn(this._sandbox);
    Object.defineProperties(attachOptions, {
      port: { enumerable: true, value: port1 },
      window: { enumerable: true, value: window },
      scriptOptions: { enumerable: true, value: Cu.cloneInto(this._scriptOptions, this._sandbox) },
      onUnload: {
        enumerable: true,
        value: Cu.cloneInto(function (cb) {
          
          if (typeof cb == "function") {
            sandboxOnUnloadQueue.push(cb);
          }
        }, this._sandbox, { cloneFunctions: true })
      }
    });

    
    var exports = this._proto.module.exports;
    if (this._scriptOptions && "attachMethod" in this._scriptOptions) {
      this._sandboxOnAttach = exports[this._scriptOptions.attachMethod];
    } else {
      this._sandboxOnAttach = exports;
    }

    if (typeof this._sandboxOnAttach !== "function") {
      throw Error("the configured attachMethod '" +
                  (this._scriptOptions.attachMethod || "module.exports") +
                  "' is not exported by the directorScript");
    }

    
    this._sandboxOnAttach.call(this._sandbox, attachOptions);

    return port2;
  },
  destroy:  function(onError) {
    
    while(this._sandboxOnUnloadQueue && this._sandboxOnUnloadQueue.length > 0) {
      let cb = this._sandboxOnUnloadQueue.pop();

      try {
        cb();
      } catch(e) {
        console.error("Exception on DirectorScript Sandbox destroy", e);
        onError(e);
      }
    }

    Cu.nukeSandbox(this._sandbox);
  }
});

function getWindowID(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindowUtils)
               .currentInnerWindowID;
}
