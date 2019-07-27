





"use strict";
























function RegisteredActorFactory(options, prefix) {
  
  this._prefix = prefix;
  if (typeof(options) != "function") {
    
    
    this._getConstructor = function () {
      
      let mod;
      try {
        mod = require(options.id);
      } catch(e) {
        throw new Error("Unable to load actor module '" + options.id + "'.\n" +
                        e.message + "\n" + e.stack + "\n");
      }
      
      let c = mod[options.constructorName];
      if (!c) {
        throw new Error("Unable to find actor constructor named '" +
                        options.constructorName + "'. (Is it exported?)");
      }
      return c;
    };
  } else {
    
    this._getConstructor = () => options;
    
    
    this.name = options.name;
    
    
    
    if (options.prototype.actorPrefix) {
      this._prefix = options.prototype.actorPrefix;
    }
  }
}
RegisteredActorFactory.prototype.createObservedActorFactory = function (conn, parentActor) {
  return new ObservedActorFactory(this._getConstructor, this._prefix, conn, parentActor);
}
exports.RegisteredActorFactory = RegisteredActorFactory;
















function ObservedActorFactory(getConstructor, prefix, conn, parentActor) {
  this._getConstructor = getConstructor;
  this._conn = conn;
  this._parentActor = parentActor;

  this.actorPrefix = prefix;

  this.actorID = null;
  this.registeredPool = null;
}
ObservedActorFactory.prototype.createActor = function () {
  
  let c = this._getConstructor();
  
  let instance = new c(this._conn, this._parentActor);
  instance.conn = this._conn;
  instance.parentID = this._parentActor.actorID;
  
  
  
  instance.actorID = this.actorID;
  this.registeredPool.addActor(instance);
  return instance;
}
exports.ObservedActorFactory = ObservedActorFactory;







































exports.createExtraActors = function createExtraActors(aFactories, aPool) {
  
  for (let name in aFactories) {
    let actor = this._extraActors[name];
    if (!actor) {
      
      
      
      
      actor = aFactories[name].createObservedActorFactory(this.conn, this);
      this._extraActors[name] = actor;
    }
    aPool.addActor(actor);
  }
}














exports.appendExtraActors = function appendExtraActors(aObject) {
  for (let name in this._extraActors) {
    let actor = this._extraActors[name];
    aObject[name] = actor.actorID;
  }
}








function ActorPool(aConnection)
{
  this.conn = aConnection;
  this._cleanups = {};
  this._actors = {};
}

ActorPool.prototype = {
  








  addActor: function AP_addActor(aActor) {
    aActor.conn = this.conn;
    if (!aActor.actorID) {
      let prefix = aActor.actorPrefix;
      if (!prefix && typeof aActor == "function") {
        
        prefix = aActor.prototype.actorPrefix || aActor.prototype.typeName;
      }
      aActor.actorID = this.conn.allocID(prefix || undefined);
    }

    if (aActor.registeredPool) {
      aActor.registeredPool.removeActor(aActor);
    }
    aActor.registeredPool = this;

    this._actors[aActor.actorID] = aActor;
    if (aActor.disconnect) {
      this._cleanups[aActor.actorID] = aActor;
    }
  },

  get: function AP_get(aActorID) {
    return this._actors[aActorID] || undefined;
  },

  has: function AP_has(aActorID) {
    return aActorID in this._actors;
  },

  


  isEmpty: function AP_isEmpty() {
    return Object.keys(this._actors).length == 0;
  },

  


  removeActor: function AP_remove(aActor) {
    delete this._actors[aActor.actorID];
    delete this._cleanups[aActor.actorID];
  },

  


  unmanage: function(aActor) {
    return this.removeActor(aActor);
  },

  


  cleanup: function AP_cleanup() {
    for each (let actor in this._cleanups) {
      actor.disconnect();
    }
    this._cleanups = {};
  }
}

exports.ActorPool = ActorPool;



exports.getOffsetColumn = function getOffsetColumn(aOffset, aScript) {
  let bestOffsetMapping = null;
  for (let offsetMapping of aScript.getAllColumnOffsets()) {
    if (!bestOffsetMapping ||
        (offsetMapping.offset <= aOffset &&
         offsetMapping.offset > bestOffsetMapping.offset)) {
      bestOffsetMapping = offsetMapping;
    }
  }

  if (!bestOffsetMapping) {
    
    
    
    
    reportError(new Error("Could not find a column for offset " + aOffset
                          + " in the script " + aScript));
    return 0;
  }

  return bestOffsetMapping.columnNumber;
}
