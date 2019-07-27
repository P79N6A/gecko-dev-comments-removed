





"use strict";
























function RegisteredActorFactory(options, prefix) {
  
  this._prefix = prefix;
  if (typeof(options) != "function") {
    
    if (options.constructorFun) {
      this._getConstructor = () => options.constructorFun;
    } else {
      
      
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
    }
    
    
    this.name = options.constructorName;
  } else {
    
    this._getConstructor = () => options;
    
    
    this.name = options.name;

    
    
    
    if (options.prototype && options.prototype.actorPrefix) {
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

    
    
    if (!aPool.has(actor.actorID)) {
      aPool.addActor(actor);
    }
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
  },

  forEach: function(callback) {
    for (let name in this._actors) {
      callback(this._actors[name]);
    }
  },
}

exports.ActorPool = ActorPool;













function OriginalLocation(actor, line, column, name) {
  this._connection = actor ? actor.conn : null;
  this._actorID = actor ? actor.actorID : undefined;
  this._line = line;
  this._column = column;
  this._name = name;
}

OriginalLocation.fromGeneratedLocation = function (generatedLocation) {
  return new OriginalLocation(
    generatedLocation.generatedSourceActor,
    generatedLocation.generatedLine,
    generatedLocation.generatedColumn
  );
};

OriginalLocation.prototype = {
  get originalSourceActor() {
    return this._connection ? this._connection.getActor(this._actorID) : null;
  },

  get originalUrl() {
    let actor = this.originalSourceActor;
    let source = actor.source;
    return source ? source.url : actor._originalUrl;
  },

  get originalLine() {
    return this._line;
  },

  get originalColumn() {
    return this._column;
  },

  get originalName() {
    return this._name;
  },

  get generatedSourceActor() {
    throw new Error("Shouldn't  access generatedSourceActor from an OriginalLocation");
  },

  get generatedLine() {
    throw new Error("Shouldn't access generatedLine from an OriginalLocation");
  },

  get generatedColumn() {
    throw new Error("Shouldn't access generatedColumn from an Originallocation");
  },

  equals: function (other) {
    return this.originalSourceActor.url == other.originalSourceActor.url &&
           this.originalLine === other.originalLine &&
           (this.originalColumn === undefined ||
            other.originalColumn === undefined ||
            this.originalColumn === other.originalColumn);
  },

  toJSON: function () {
    return {
      source: this.originalSourceActor.form(),
      line: this.originalLine,
      column: this.originalColumn
    };
  }
};

exports.OriginalLocation = OriginalLocation;











function GeneratedLocation(actor, line, column, lastColumn) {
  this._connection = actor ? actor.conn : null;
  this._actorID = actor ? actor.actorID : undefined;
  this._line = line;
  this._column = column;
  this._lastColumn = (lastColumn !== undefined) ? lastColumn : column + 1;
}

GeneratedLocation.fromOriginalLocation = function (originalLocation) {
  return new GeneratedLocation(
    originalLocation.originalSourceActor,
    originalLocation.originalLine,
    originalLocation.originalColumn
  );
};

GeneratedLocation.prototype = {
  get originalSourceActor() {
    throw new Error();
  },

  get originalUrl() {
    throw new Error("Shouldn't access originalUrl from a GeneratedLocation");
  },

  get originalLine() {
    throw new Error("Shouldn't access originalLine from a GeneratedLocation");
  },

  get originalColumn() {
    throw new Error("Shouldn't access originalColumn from a GeneratedLocation");
  },

  get originalName() {
    throw new Error("Shouldn't access originalName from a GeneratedLocation");
  },

  get generatedSourceActor() {
    return this._connection ? this._connection.getActor(this._actorID) : null;
  },

  get generatedLine() {
    return this._line;
  },

  get generatedColumn() {
    return this._column;
  },

  get generatedLastColumn() {
    return this._lastColumn;
  },

  equals: function (other) {
    return this.generatedSourceActor.url == other.generatedSourceActor.url &&
           this.generatedLine === other.generatedLine &&
           (this.generatedColumn === undefined ||
            other.generatedColumn === undefined ||
            this.generatedColumn === other.generatedColumn);
  },

  toJSON: function () {
    return {
      source: this.generatedSourceActor.form(),
      line: this.generatedLine,
      column: this.generatedColumn,
      lastColumn: this.generatedLastColumn
    };
  }
};

exports.GeneratedLocation = GeneratedLocation;



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
