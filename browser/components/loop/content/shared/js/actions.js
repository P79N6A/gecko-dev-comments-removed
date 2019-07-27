





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.actions = (function() {
  "use strict";

  






  function Action(name, schema, values) {
    var validatedData = new loop.validate.Validator(schema || {})
                                         .validate(values || {});
    for (var prop in validatedData)
      this[prop] = validatedData[prop];

    this.name = name;
  }

  Action.define = function(name, schema) {
    return Action.bind(null, name, schema);
  };

  return {
    


    GatherCallData: Action.define("gatherCallData", {
      
      
      calleeId: [String, null],
      
      callId: [String, null]
    }),

    


    CancelCall: Action.define("cancelCall", {
    }),

    



    ConnectCall: Action.define("connectCall", {
      
      
      sessionData: Object
    }),

    




    ConnectionProgress: Action.define("connectionProgress", {
      
      state: String
    }),

    


    ConnectionFailure: Action.define("connectionFailure", {
      
      reason: String
    })
  };
})();
