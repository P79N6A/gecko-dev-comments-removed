



"use strict";

var Cu = require('chrome').Cu;
var XPCOMUtils = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {}).XPCOMUtils;

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommandUtils",
                                  "resource:///modules/devtools/DeveloperToolbar.jsm");

XPCOMUtils.defineLazyGetter(this, "Requisition", function() {
  return require("gcli/cli").Requisition;
});

XPCOMUtils.defineLazyGetter(this, "centralCanon", function() {
  return require("gcli/commands/commands").centralCanon;
});

var util = require('gcli/util/util');

var protocol = require("devtools/server/protocol");
var method = protocol.method;
var Arg = protocol.Arg;
var Option = protocol.Option;
var RetVal = protocol.RetVal;




var GcliActor = exports.GcliActor = protocol.ActorClass({
  typeName: "gcli",

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    let browser = tabActor.browser;

    let environment = {
      chromeWindow: browser.ownerGlobal,
      chromeDocument: browser.ownerDocument,
      window: browser.contentWindow,
      document: browser.contentDocument
    };

    this.requisition = new Requisition({ environment: env });
  },

  


  specs: method(function() {
    return this.requisition.canon.getCommandSpecs();
  }, {
    request: {},
    response: RetVal("json")
  }),

  






  execute: method(function(typed) {
    return this.requisition.updateExec(typed).then(function(output) {
      return output.toJson();
    });
  }, {
    request: {
      typed: Arg(0, "string") 
    },
    response: RetVal("json")
  }),

  


  state: method(function(typed, start, rank) {
    return this.requisition.update(typed).then(() => {
      return this.requisition.getStateData(start, rank);
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      start: Arg(1, "number"), 
      rank: Arg(2, "number") 
    },
    response: RetVal("json")
  }),

  






  typeparse: method(function(typed, param) {
    return this.requisition.update(typed).then(function() {
      var assignment = this.requisition.getAssignment(param);

      return promise.resolve(assignment.predictions).then(function(predictions) {
        return {
          status: assignment.getStatus().toString(),
          message: assignment.message,
          predictions: predictions
        };
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      param: Arg(1, "string") 
    },
    response: RetVal("json")
  }),

  



  typeincrement: method(function(typed, param) {
    return this.requisition.update(typed).then(function() {
      var assignment = this.requisition.getAssignment(param);
      return this.requisition.increment(assignment).then(function() {
        return assignment.arg == null ? undefined : assignment.arg.text;
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      param: Arg(1, "string") 
    },
    response: RetVal("string")
  }),

  


  typedecrement: method(function(typed, param) {
    return this.requisition.update(typed).then(function() {
      var assignment = this.requisition.getAssignment(param);
      return this.requisition.decrement(assignment).then(function() {
        return assignment.arg == null ? undefined : assignment.arg.text;
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      param: Arg(1, "string") 
    },
    response: RetVal("string")
  }),

  


  selectioninfo: method(function(commandName, paramName, action) {
    var command = this.requisition.canon.getCommand(commandName);
    if (command == null) {
      throw new Error('No command called \'' + commandName + '\'');
    }

    var type;
    command.params.forEach(function(param) {
      if (param.name === paramName) {
        type = param.type;
      }
    });
    if (type == null) {
      throw new Error('No parameter called \'' + paramName + '\' in \'' +
                      commandName + '\'');
    }

    switch (action) {
      case 'lookup':
        return type.lookup(context);
      case 'data':
        return type.data(context);
      default:
        throw new Error('Action must be either \'lookup\' or \'data\'');
    }
  }, {
    request: {
      typed: Arg(0, "string"), 
      param: Arg(1, "string"), 
      action: Arg(1, "string") 
    },
    response: RetVal("json")
  })
});

exports.GcliFront = protocol.FrontClass(GcliActor, {
  initialize: function(client, tabForm) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.gcliActor;

    
    
    this.manage(this);
  },
});
