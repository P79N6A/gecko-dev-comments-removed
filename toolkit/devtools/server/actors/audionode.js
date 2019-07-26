


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const protocol = require("devtools/server/protocol");
const { method, Arg, Option, RetVal } = protocol;



protocol.types.addDictType("audio-node-param-grip", {
  type: "string",
  value: "nullable:primitive"
});





let AudioNodeActor = exports.AudioNodeActor = protocol.ActorClass({
  typeName: "audionode",

  







  initialize: function (conn, node) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.node = XPCNativeWrapper.unwrap(node);
    try {
      this.type = this.node.toString().match(/\[object (.*)\]$/)[1];
    } catch (e) {
      this.type = "";
    }
  },

  



  getType: method(function () {
    return this.type;
  }, {
    response: { type: RetVal("string") }
  }),

  



  isSource: method(function () {
    return !!~this.type.indexOf("Source") || this.type === "OscillatorNode";
  }, {
    response: { source: RetVal("boolean") }
  }),

  











  setParam: method(function (param, value, dataType) {
    
    if (dataType === "string") {
      value = value.replace(/[\'\"]*/g, "");
    }
    try {
      if (isAudioParam(this.node, param))
        this.node[param].value = cast(value, dataType);
      else
        this.node[param] = cast(value, dataType);
      return undefined;
    } catch (e) {
      return constructError(e);
    }
  }, {
    request: {
      param: Arg(0, "string"),
      value: Arg(1, "string"),
      dataType: Arg(2, "string")
    },
    response: { error: RetVal("nullable:json") }
  }),

  





  getParam: method(function (param) {
    
    if (!this.node[param])
      return undefined;
    let value = isAudioParam(this.node, param) ? this.node[param].value : this.node[param];
    let type = typeof type;
    return value;
    return { type: type, value: value };
  }, {
    request: {
      param: Arg(0, "string")
    },
    response: { text: RetVal("nullable:primitive") }
  }),
});









function cast (value, type) {
  if (!type || type === "string")
    return value;
  if (type === "number")
    return parseFloat(value);
  if (type === "boolean")
    return value === "true";
  return undefined;
}










function isAudioParam (node, prop) {
  return /AudioParam/.test(node[prop].toString());
}








function constructError (err) {
  return {
    message: err.message,
    type: err.constructor.name
  };
}




let AudioNodeFront = protocol.FrontClass(AudioNodeActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    client.addActorPool(this);
    this.manage(this);
  }
});
