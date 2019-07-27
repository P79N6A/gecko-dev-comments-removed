



"use strict";

const protocol = require("devtools/server/protocol");
const { method, RetVal, Arg, types } = protocol;





let PromisesActor = protocol.ActorClass({
  typeName: "promises",

  



  initialize: function(conn, parent) {
    protocol.Actor.prototype.initialize.call(this, conn);
  },

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this, conn);
  },
});

exports.PromisesActor = PromisesActor;

exports.PromisesFront = protocol.FrontClass(PromisesActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.promisesActor;
    this.manage(this);
  }
});
