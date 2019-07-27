





"use strict";

const protocol = require("devtools/server/protocol");
const { method, Arg, Option, RetVal } = protocol;

const {DebuggerServer} = require("devtools/server/main");





const ERR_DIRECTOR_INSTALL_TWICE = "Trying to install a director-script twice";
const ERR_DIRECTOR_INSTALL_EMPTY = "Trying to install an empty director-script";
const ERR_DIRECTOR_UNINSTALL_UNKNOWN = "Trying to uninstall an unkown director-script";

const ERR_DIRECTOR_PARENT_UNKNOWN_METHOD = "Unknown parent process method";
const ERR_DIRECTOR_CHILD_NOTIMPLEMENTED_METHOD = "Unexpected call to notImplemented method";
const ERR_DIRECTOR_CHILD_MULTIPLE_REPLIES = "Unexpected multiple replies to called parent method";
const ERR_DIRECTOR_CHILD_NO_REPLY = "Unexpected no reply to called parent method";






var gDirectorScripts = Object.create(null);

const DirectorRegistry = exports.DirectorRegistry = {
  






  install: function (id, scriptDef) {
    if (id in gDirectorScripts) {
      console.error(ERR_DIRECTOR_INSTALL_TWICE,id);
      return false;
    }

    if (!scriptDef) {
      console.error(ERR_DIRECTOR_INSTALL_EMPTY, id);
      return false;
    }

    gDirectorScripts[id] = scriptDef;

    return true;
  },

  




  uninstall: function(id) {
    if (id in gDirectorScripts) {
      delete gDirectorScripts[id];

      return true;
    }

    console.error(ERR_DIRECTOR_UNINSTALL_UNKNOWN, id);

    return false;
  },

  




  checkInstalled: function (id) {
    return (this.list().indexOf(id) >= 0);
  },

  




  get: function(id) {
    return gDirectorScripts[id];
  },

  


  list: function() {
    return Object.keys(gDirectorScripts);
  },

  


  clear: function() {
   gDirectorScripts = Object.create(null);
  }
};





let gTrackedMessageManager = new Set();

exports.setupParentProcess = function setupParentProcess({mm, prefix}) {
  
  if (gTrackedMessageManager.has(mm)) {
    return;
  }
  gTrackedMessageManager.add(mm);

  
  mm.addMessageListener("debug:director-registry-request", handleChildRequest);

  DebuggerServer.once("disconnected-from-child:" + prefix, handleMessageManagerDisconnected);

  

  function handleMessageManagerDisconnected(evt, { mm: disconnected_mm }) {
    
    if (disconnected_mm !== mm || !gTrackedMessageManager.has(mm)) {
      return;
    }

    gTrackedMessageManager.delete(mm);

    
    mm.removeMessageListener("debug:director-registry-request", handleChildRequest);
  }

  function handleChildRequest(msg) {
    switch (msg.json.method) {
    case "get":
      return DirectorRegistry.get(msg.json.args[0]);
    case "list":
      return DirectorRegistry.list();
    default:
      console.error(ERR_DIRECTOR_PARENT_UNKNOWN_METHOD, msg.json.method);
      throw new Error(ERR_DIRECTOR_PARENT_UNKNOWN_METHOD);
    }
  }
};


if (DebuggerServer.isInChildProcess) {
  setupChildProcess();
}

function setupChildProcess() {
  const { sendSyncMessage } = DebuggerServer.parentMessageManager;

  DebuggerServer.setupInParent({
    module: "devtools/server/actors/director-registry",
    setupParent: "setupParentProcess"
  });

  DirectorRegistry.install = notImplemented.bind(null, "install");
  DirectorRegistry.uninstall = notImplemented.bind(null, "uninstall");
  DirectorRegistry.clear = notImplemented.bind(null, "clear");

  DirectorRegistry.get = callParentProcess.bind(null, "get");
  DirectorRegistry.list = callParentProcess.bind(null, "list");

  

  function notImplemented(method) {
    console.error(ERR_DIRECTOR_CHILD_NOTIMPLEMENTED_METHOD, method);
    throw Error(ERR_DIRECTOR_CHILD_NOTIMPLEMENTED_METHOD);
  }

  function callParentProcess(method, ...args) {
    var reply = sendSyncMessage("debug:director-registry-request", {
      method: method,
      args: args
    });

    if (reply.length === 0) {
      console.error(ERR_DIRECTOR_CHILD_NO_REPLY);
      throw Error(ERR_DIRECTOR_CHILD_NO_REPLY);
    } else if (reply.length > 1) {
      console.error(ERR_DIRECTOR_CHILD_MULTIPLE_REPLIES);
      throw Error(ERR_DIRECTOR_CHILD_MULTIPLE_REPLIES);
    }

    return reply[0];
  };
};





const DirectorRegistryActor = exports.DirectorRegistryActor = protocol.ActorClass({
  typeName: "director-registry",

  
  initialize: function(conn, parentActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  finalize: method(function() {
    
  }, {
    oneway: true
  }),

  









  install: method(function(id, { scriptCode, scriptOptions }) {
    
    if (!id || id.length === 0) {
      throw Error("director-script id is mandatory");
    }

    if (!scriptCode) {
      throw Error("director-script scriptCode is mandatory");
    }

    return DirectorRegistry.install(id, {
      scriptId: id,
      scriptCode: scriptCode,
      scriptOptions: scriptOptions
    });
  }, {
    request: {
      scriptId: Arg(0, "string"),
      scriptCode: Option(1, "string"),
      scriptOptions: Option(1, "nullable:json")
    },
    response: {
      success: RetVal("boolean")
    }
  }),

  





  uninstall: method(function (id) {
    return DirectorRegistry.uninstall(id);
  }, {
    request: {
      scritpId: Arg(0, "string")
    },
    response: {
      success: RetVal("boolean")
    }
  }),

  


  list: method(function () {
    return DirectorRegistry.list();
  }, {
    response: {
      directorScripts: RetVal("array:string")
    }
  })
});




exports.DirectorRegistryFront = protocol.FrontClass(DirectorRegistryActor, {
  initialize: function(client, { directorRegistryActor }) {
    protocol.Front.prototype.initialize.call(this, client, {
      actor: directorRegistryActor
    });
    this.manage(this);
  }
});
