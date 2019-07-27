



"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");
const {
  method, Arg, Option, RetVal, Front, FrontClass, Actor, ActorClass
} = require("devtools/server/protocol");
const events = require("sdk/event/core");
const { createSystem } = require("gcli/system");




const GcliActor = ActorClass({
  typeName: "gcli",

  events: {
    "commands-changed" : {
      type: "commandsChanged"
    }
  },

  initialize: function(conn, tabActor) {
    Actor.prototype.initialize.call(this, conn);

    this._commandsChanged = this._commandsChanged.bind(this);

    this._tabActor = tabActor;
    this._requisitionPromise = undefined; 
  },

  disconnect: function() {
    return this.destroy();
  },

  destroy: function() {
    Actor.prototype.destroy.call(this);

    
    if (this._requisitionPromise == null) {
      this._commandsChanged = undefined;
      this._tabActor = undefined;
      return Promise.resolve();
    }

    return this._getRequisition().then(requisition => {
      requisition.destroy();

      this._system.commands.onCommandsChange.remove(this._commandsChanged);
      this._system.destroy();
      this._system = undefined;

      this._requisitionPromise = undefined;
      this._tabActor = undefined;

      this._commandsChanged = undefined;
    });
  },

  


  _testOnly_addItemsByModule: method(function(names) {
    return this._getRequisition().then(requisition => {
      return requisition.system.addItemsByModule(names);
    });
  }, {
    request: {
      customProps: Arg(0, "array:string")
    }
  }),

  


  _testOnly_removeItemsByModule: method(function(names) {
    return this._getRequisition().then(requisition => {
      return requisition.system.removeItemsByModule(names);
    });
  }, {
    request: {
      customProps: Arg(0, "array:string")
    }
  }),

  





  specs: method(function(customProps) {
    return this._getRequisition().then(requisition => {
      return requisition.system.commands.getCommandSpecs(customProps);
    });
  }, {
    request: {
      customProps: Arg(0, "nullable:array:string")
    },
    response: {
      value: RetVal("array:json")
    }
  }),

  






  execute: method(function(typed) {
    return this._getRequisition().then(requisition => {
      return requisition.updateExec(typed).then(output => output.toJson());
    });
  }, {
    request: {
      typed: Arg(0, "string") 
    },
    response: RetVal("json")
  }),

  


  state: method(function(typed, start, rank) {
    return this._getRequisition().then(requisition => {
      return requisition.update(typed).then(() => {
        return requisition.getStateData(start, rank);
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      start: Arg(1, "number"), 
      rank: Arg(2, "number") 
    },
    response: RetVal("json")
  }),

  






  parseType: method(function(typed, paramName) {
    return this._getRequisition().then(requisition => {
      return requisition.update(typed).then(() => {
        let assignment = requisition.getAssignment(paramName);
        return Promise.resolve(assignment.predictions).then(predictions => {
          return {
            status: assignment.getStatus().toString(),
            message: assignment.message,
            predictions: predictions
          };
        });
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      paramName: Arg(1, "string") 
    },
    response: RetVal("json")
  }),

  



  nudgeType: method(function(typed, by, paramName) {
    return this.requisition.update(typed).then(() => {
      const assignment = this.requisition.getAssignment(paramName);
      return this.requisition.nudge(assignment, by).then(() => {
        return assignment.arg == null ? undefined : assignment.arg.text;
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"),    
      by: Arg(1, "number"),       
      paramName: Arg(2, "string") 
    },
    response: RetVal("string")
  }),

  


  getSelectionLookup: method(function(commandName, paramName) {
    return this._getRequisition().then(requisition => {
      const command = requisition.system.commands.get(commandName);
      if (command == null) {
        throw new Error("No command called '" + commandName + "'");
      }

      let type;
      command.params.forEach(param => {
        if (param.name === paramName) {
          type = param.type;
        }
      });

      if (type == null) {
        throw new Error("No parameter called '" + paramName + "' in '" +
                        commandName + "'");
      }

      const reply = type.getLookup(requisition.executionContext);
      return Promise.resolve(reply).then(lookup => {
        
        
        return lookup.map(info => ({ name: info.name }));
      });
    });
  }, {
    request: {
      commandName: Arg(0, "string"), 
      paramName: Arg(1, "string"),   
    },
    response: RetVal("json")
  }),

  


  _getRequisition: function() {
    if (this._tabActor == null) {
      throw new Error('GcliActor used post-destroy');
    }

    if (this._requisitionPromise != null) {
      return this._requisitionPromise;
    }

    const Requisition = require("gcli/cli").Requisition;
    const tabActor = this._tabActor;

    this._system = createSystem({ location: "server" });
    this._system.commands.onCommandsChange.add(this._commandsChanged);

    const gcliInit = require("devtools/commandline/commands-index");
    gcliInit.addAllItemsByModule(this._system);

    
    
    
    this._requisitionPromise = this._system.load().then(() => {
      const environment = {
        get chromeWindow() {
          throw new Error("environment.chromeWindow is not available in runAt:server commands");
        },

        get chromeDocument() {
          throw new Error("environment.chromeDocument is not available in runAt:server commands");
        },

        get window() tabActor.window,
        get document() tabActor.window.document,
        get __deprecatedTabActor() tabActor,
      };

      return new Requisition(this._system, { environment: environment });
    });

    return this._requisitionPromise;
  },

  


  _commandsChanged: function() {
    events.emit(this, "commands-changed");
  },
});

exports.GcliActor = GcliActor;




const GcliFront = exports.GcliFront = FrontClass(GcliActor, {
  initialize: function(client, tabForm) {
    Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.gcliActor;

    
    
    this.manage(this);
  },
});


const knownFronts = new WeakMap();






exports.GcliFront.create = function(target) {
  return target.makeRemote().then(() => {
    let front = knownFronts.get(target.client);
    if (front == null && target.form.gcliActor != null) {
      front = new GcliFront(target.client, target.form);
      knownFronts.set(target.client, front);
    }
    return front;
  });
};
