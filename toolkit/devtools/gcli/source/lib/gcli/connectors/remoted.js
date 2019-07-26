

















'use strict';

var Promise = require('../util/promise').Promise;
var host = require('../util/host');
var fileparser = require('../util/fileparser');

var protocol = require('./protocol');
var method = protocol.method;
var Arg = protocol.Arg;
var RetVal = protocol.RetVal;




var Remoter = exports.Remoter = function(requisition) {
  this.requisition = requisition;
  this._listeners = [];
};




Remoter.prototype.addListener = function(action) {
  var listener = {
    action: action,
    caller: function() {
      action('canonChanged', this.requisition.canon.getCommandSpecs());
    }.bind(this)
  };
  this._listeners.push(listener);

  this.requisition.canon.onCanonChange.add(listener.caller);
};




Remoter.prototype.removeListener = function(action) {
  var listener;

  this._listeners = this._listeners.filter(function(li) {
    if (li.action === action) {
      listener = li;
      return false;
    }
    return true;
  });

  if (listener == null) {
    throw new Error('action not a known listener');
  }

  this.requisition.canon.onCanonChange.remove(listener.caller);
};




Remoter.prototype.exposed = {
  


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
    return this.requisition.update(typed).then(function() {
      return this.requisition.getStateData(start, rank);
    }.bind(this));
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

      return Promise.resolve(assignment.predictions).then(function(predictions) {
        return {
          status: assignment.getStatus().toString(),
          message: assignment.message,
          predictions: predictions
        };
      });
    }.bind(this));
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
        var arg = assignment.arg;
        return arg == null ? undefined : arg.text;
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
        var arg = assignment.arg;
        return arg == null ? undefined : arg.text;
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
        return type.lookup(this.requisition.executionContext);
      case 'data':
        return type.data(this.requisition.executionContext);
      default:
        throw new Error('Action must be either \'lookup\' or \'data\'');
    }
  }, {
    request: {
      commandName: Arg(0, "string"), 
      paramName: Arg(1, "string"), 
      action: Arg(2, "string") 
    },
    response: RetVal("json")
  }),

  



  system: method(function(cmd, args, cwd, env) {
    return host.spawn({ cmd: cmd, args: args, cwd: cwd, env: env });
  }, {
    request: {
      cmd: Arg(0, "string"), 
      args: Arg(1, "array:string"), 
      cwd: Arg(2, "string"), 
      env: Arg(3, "json") 
    },
    response: RetVal("json")
  }),

  


  parsefile: method(function(typed, filetype, existing, matches) {
    var options = {
      filetype: filetype,
      existing: existing,
      matches: new RegExp(matches)
    };

    return fileparser.parse(typed, options).then(function(reply) {
      reply.status = reply.status.toString();
      if (reply.predictor == null) {
        return reply;
      }

      return reply.predictor().then(function(predictions) {
        delete reply.predictor;
        reply.predictions = predictions;
        return reply;
      });
    });
  }, {
    request: {
      typed: Arg(0, "string"), 
      filetype: Arg(1, "array:string"), 
      existing: Arg(2, "string"), 
      matches: Arg(3, "json") 
    },
    response: RetVal("json")
  })
};
