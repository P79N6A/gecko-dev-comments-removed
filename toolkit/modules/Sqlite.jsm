



"use strict";

this.EXPORTED_SYMBOLS = [
  "Sqlite",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Log",
                                  "resource://gre/modules/Log.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");




let connectionCounters = new Map();




let isClosed = false;





XPCOMUtils.defineLazyGetter(this, "Barriers", () => {
  let Barriers = {
    





    shutdown: new AsyncShutdown.Barrier("Sqlite.jsm: wait until all clients have completed their task"),

    




    connections: new AsyncShutdown.Barrier("Sqlite.jsm: wait until all connections are closed"),
  };

  





  AsyncShutdown.profileBeforeChange.addBlocker("Sqlite.jsm shutdown blocker",
    Task.async(function* () {
      yield Barriers.shutdown.wait();
      
      
      

      
      isClosed = true;

      
      yield Barriers.connections.wait();
    }),

    function status() {
      if (isClosed) {
        
        
        return { description: "Waiting for connections to close",
                 status: Barriers.connections.status };
      }

      
      
      
      return { description: "Waiting for the barrier to be lifted",
               status: Barriers.shutdown.status };
  });

  return Barriers;
});



































function openConnection(options) {
  let log = Log.repository.getLogger("Sqlite.ConnectionOpener");

  if (!options.path) {
    throw new Error("path not specified in connection options.");
  }

  if (isClosed) {
    throw new Error("Sqlite.jsm has been shutdown. Cannot open connection to: " + options.path);
  }


  
  let path = OS.Path.join(OS.Constants.Path.profileDir, options.path);

  let sharedMemoryCache = "sharedMemoryCache" in options ?
                            options.sharedMemoryCache : true;

  let openedOptions = {};

  if ("shrinkMemoryOnConnectionIdleMS" in options) {
    if (!Number.isInteger(options.shrinkMemoryOnConnectionIdleMS)) {
      throw new Error("shrinkMemoryOnConnectionIdleMS must be an integer. " +
                      "Got: " + options.shrinkMemoryOnConnectionIdleMS);
    }

    openedOptions.shrinkMemoryOnConnectionIdleMS =
      options.shrinkMemoryOnConnectionIdleMS;
  }

  let file = FileUtils.File(path);

  let basename = OS.Path.basename(path);
  let number = connectionCounters.get(basename) || 0;
  connectionCounters.set(basename, number + 1);

  let identifier = basename + "#" + number;

  log.info("Opening database: " + path + " (" + identifier + ")");
  let deferred = Promise.defer();
  let options = null;
  if (!sharedMemoryCache) {
    options = Cc["@mozilla.org/hash-property-bag;1"].
      createInstance(Ci.nsIWritablePropertyBag);
    options.setProperty("shared", false);
  }
  Services.storage.openAsyncDatabase(file, options, function(status, connection) {
    if (!connection) {
      log.warn("Could not open connection: " + status);
      deferred.reject(new Error("Could not open connection: " + status));
    }
    log.info("Connection opened");
    try {
      deferred.resolve(
        new OpenedConnection(connection.QueryInterface(Ci.mozIStorageAsyncConnection), basename, number,
        openedOptions));
    } catch (ex) {
      log.warn("Could not open database: " + CommonUtils.exceptionStr(ex));
      deferred.reject(ex);
    }
  });
  return deferred.promise;
}






























function cloneStorageConnection(options) {
  let log = Log.repository.getLogger("Sqlite.ConnectionCloner");

  let source = options && options.connection;
  if (!source) {
    throw new TypeError("connection not specified in clone options.");
  }
  if (!source instanceof Ci.mozIStorageAsyncConnection) {
    throw new TypeError("Connection must be a valid Storage connection.");
  }

  if (isClosed) {
    throw new Error("Sqlite.jsm has been shutdown. Cannot close connection to: " + source.database.path);
  }

  let openedOptions = {};

  if ("shrinkMemoryOnConnectionIdleMS" in options) {
    if (!Number.isInteger(options.shrinkMemoryOnConnectionIdleMS)) {
      throw new TypeError("shrinkMemoryOnConnectionIdleMS must be an integer. " +
                          "Got: " + options.shrinkMemoryOnConnectionIdleMS);
    }
    openedOptions.shrinkMemoryOnConnectionIdleMS =
      options.shrinkMemoryOnConnectionIdleMS;
  }

  let path = source.databaseFile.path;
  let basename = OS.Path.basename(path);
  let number = connectionCounters.get(basename) || 0;
  connectionCounters.set(basename, number + 1);
  let identifier = basename + "#" + number;

  log.info("Cloning database: " + path + " (" + identifier + ")");
  let deferred = Promise.defer();

  source.asyncClone(!!options.readOnly, (status, connection) => {
    if (!connection) {
      log.warn("Could not clone connection: " + status);
      deferred.reject(new Error("Could not clone connection: " + status));
    }
    log.info("Connection cloned");
    try {
      let conn = connection.QueryInterface(Ci.mozIStorageAsyncConnection);
      deferred.resolve(new OpenedConnection(conn, basename, number,
                                            openedOptions));
    } catch (ex) {
      log.warn("Could not clone database: " + CommonUtils.exceptionStr(ex));
      deferred.reject(ex);
    }
  });
  return deferred.promise;
}
















































function OpenedConnection(connection, basename, number, options) {
  this._log = Log.repository.getLoggerWithMessagePrefix("Sqlite.Connection." + basename,
                                                        "Conn #" + number + ": ");

  this._log.info("Opened");

  this._connection = connection;
  this._connectionIdentifier = basename + " Conn #" + number;
  this._open = true;

  this._cachedStatements = new Map();
  this._anonymousStatements = new Map();
  this._anonymousCounter = 0;

  
  
  this._pendingStatements = new Map();

  
  this._statementCounter = 0;

  this._inProgressTransaction = null;

  this._idleShrinkMS = options.shrinkMemoryOnConnectionIdleMS;
  if (this._idleShrinkMS) {
    this._idleShrinkTimer = Cc["@mozilla.org/timer;1"]
                              .createInstance(Ci.nsITimer);
    
    
  }

  this._deferredClose = Promise.defer();

  Barriers.connections.client.addBlocker(
    this._connectionIdentifier + ": waiting for shutdown",
    this._deferredClose.promise
  );
}

OpenedConnection.prototype = Object.freeze({
  TRANSACTION_DEFERRED: "DEFERRED",
  TRANSACTION_IMMEDIATE: "IMMEDIATE",
  TRANSACTION_EXCLUSIVE: "EXCLUSIVE",

  TRANSACTION_TYPES: ["DEFERRED", "IMMEDIATE", "EXCLUSIVE"],

  






  getSchemaVersion: function() {
    let self = this;
    return this.execute("PRAGMA user_version").then(
      function onSuccess(result) {
        if (result == null) {
          return 0;
        }
        return JSON.stringify(result[0].getInt32(0));
      }
    );
  },

  setSchemaVersion: function(value) {
    if (!Number.isInteger(value)) {
      
      throw new TypeError("Schema version must be an integer. Got " + value);
    }
    this._ensureOpen();
    return this.execute("PRAGMA user_version = " + value);
  },

  

















  close: function () {
    if (!this._connection) {
      return this._deferredClose.promise;
    }

    this._log.debug("Request to close connection.");
    this._clearIdleShrinkTimer();

    
    
    
    
    if (!this._inProgressTransaction) {
      this._finalize(this._deferredClose);
      return this._deferredClose.promise;
    }

    
    
    
    this._log.warn("Transaction in progress at time of close. Rolling back.");

    let onRollback = this._finalize.bind(this, this._deferredClose);

    this.execute("ROLLBACK TRANSACTION").then(onRollback, onRollback);
    this._inProgressTransaction.reject(new Error("Connection being closed."));
    this._inProgressTransaction = null;

    return this._deferredClose.promise;
  },

  













  clone: function (readOnly=false) {
    this._ensureOpen();

    this._log.debug("Request to clone connection.");

    let options = {
      connection: this._connection,
      readOnly: readOnly,
    };
    if (this._idleShrinkMS)
      options.shrinkMemoryOnConnectionIdleMS = this._idleShrinkMS;

    return cloneStorageConnection(options);
  },

  _finalize: function (deferred) {
    this._log.debug("Finalizing connection.");
    
    for (let [k, statement] of this._pendingStatements) {
      statement.cancel();
    }
    this._pendingStatements.clear();

    
    this._statementCounter = 0;

    
    for (let [k, statement] of this._anonymousStatements) {
      statement.finalize();
    }
    this._anonymousStatements.clear();

    for (let [k, statement] of this._cachedStatements) {
      statement.finalize();
    }
    this._cachedStatements.clear();

    
    
    this._open = false;

    this._log.debug("Calling asyncClose().");
    this._connection.asyncClose({
      complete: function () {
        this._log.info("Closed");
        this._connection = null;
        
        
        Barriers.connections.client.removeBlocker(deferred.promise);
        deferred.resolve();
      }.bind(this),
    });
  },

  

























































  executeCached: function (sql, params=null, onRow=null) {
    this._ensureOpen();

    if (!sql) {
      throw new Error("sql argument is empty.");
    }

    let statement = this._cachedStatements.get(sql);
    if (!statement) {
      statement = this._connection.createAsyncStatement(sql);
      this._cachedStatements.set(sql, statement);
    }

    this._clearIdleShrinkTimer();

    let deferred = Promise.defer();

    try {
      this._executeStatement(sql, statement, params, onRow).then(
        function onResult(result) {
          this._startIdleShrinkTimer();
          deferred.resolve(result);
        }.bind(this),
        function onError(error) {
          this._startIdleShrinkTimer();
          deferred.reject(error);
        }.bind(this)
      );
    } catch (ex) {
      this._startIdleShrinkTimer();
      throw ex;
    }

    return deferred.promise;
  },

  














  execute: function (sql, params=null, onRow=null) {
    if (typeof(sql) != "string") {
      throw new Error("Must define SQL to execute as a string: " + sql);
    }

    this._ensureOpen();

    let statement = this._connection.createAsyncStatement(sql);
    let index = this._anonymousCounter++;

    this._anonymousStatements.set(index, statement);
    this._clearIdleShrinkTimer();

    let onFinished = function () {
      this._anonymousStatements.delete(index);
      statement.finalize();
      this._startIdleShrinkTimer();
    }.bind(this);

    let deferred = Promise.defer();

    try {
      this._executeStatement(sql, statement, params, onRow).then(
        function onResult(rows) {
          onFinished();
          deferred.resolve(rows);
        }.bind(this),

        function onError(error) {
          onFinished();
          deferred.reject(error);
        }.bind(this)
      );
    } catch (ex) {
      onFinished();
      throw ex;
    }

    return deferred.promise;
  },

  


  get transactionInProgress() {
    return this._open && !!this._inProgressTransaction;
  },

  






















  executeTransaction: function (func, type=this.TRANSACTION_DEFERRED) {
    if (this.TRANSACTION_TYPES.indexOf(type) == -1) {
      throw new Error("Unknown transaction type: " + type);
    }

    this._ensureOpen();

    if (this._inProgressTransaction) {
      throw new Error("A transaction is already active. Only one transaction " +
                      "can be active at a time.");
    }

    this._log.debug("Beginning transaction");
    let deferred = Promise.defer();
    this._inProgressTransaction = deferred;
    Task.spawn(function doTransaction() {
      
      
      
      yield this.execute("BEGIN " + type + " TRANSACTION");

      let result;
      try {
        result = yield Task.spawn(func(this));
      } catch (ex) {
        
        
        
        
        if (!this._inProgressTransaction) {
          this._log.warn("Connection was closed while performing transaction. " +
                         "Received error should be due to closed connection: " +
                         CommonUtils.exceptionStr(ex));
          throw ex;
        }

        this._log.warn("Error during transaction. Rolling back: " +
                       CommonUtils.exceptionStr(ex));
        try {
          yield this.execute("ROLLBACK TRANSACTION");
        } catch (inner) {
          this._log.warn("Could not roll back transaction. This is weird: " +
                         CommonUtils.exceptionStr(inner));
        }

        throw ex;
      }

      
      if (!this._inProgressTransaction) {
        this._log.warn("Connection was closed while performing transaction. " +
                       "Unable to commit.");
        throw new Error("Connection closed before transaction committed.");
      }

      try {
        yield this.execute("COMMIT TRANSACTION");
      } catch (ex) {
        this._log.warn("Error committing transaction: " +
                       CommonUtils.exceptionStr(ex));
        throw ex;
      }

      throw new Task.Result(result);
    }.bind(this)).then(
      function onSuccess(result) {
        this._inProgressTransaction = null;
        deferred.resolve(result);
      }.bind(this),
      function onError(error) {
        this._inProgressTransaction = null;
        deferred.reject(error);
      }.bind(this)
    );

    return deferred.promise;
  },

  







  tableExists: function (name) {
    return this.execute(
      "SELECT name FROM (SELECT * FROM sqlite_master UNION ALL " +
                        "SELECT * FROM sqlite_temp_master) " +
      "WHERE type = 'table' AND name=?",
      [name])
      .then(function onResult(rows) {
        return Promise.resolve(rows.length > 0);
      }
    );
  },

  







  indexExists: function (name) {
    return this.execute(
      "SELECT name FROM (SELECT * FROM sqlite_master UNION ALL " +
                        "SELECT * FROM sqlite_temp_master) " +
      "WHERE type = 'index' AND name=?",
      [name])
      .then(function onResult(rows) {
        return Promise.resolve(rows.length > 0);
      }
    );
  },

  




  shrinkMemory: function () {
    this._log.info("Shrinking memory usage.");

    let onShrunk = this._clearIdleShrinkTimer.bind(this);

    return this.execute("PRAGMA shrink_memory").then(onShrunk, onShrunk);
  },

  









  discardCachedStatements: function () {
    let count = 0;
    for (let [k, statement] of this._cachedStatements) {
      ++count;
      statement.finalize();
    }
    this._cachedStatements.clear();
    this._log.debug("Discarded " + count + " cached statements.");
    return count;
  },

  



  _bindParameters: function (statement, params) {
    if (!params) {
      return;
    }

    if (Array.isArray(params)) {
      
      if (params.length && (typeof(params[0]) == "object")) {
        let paramsArray = statement.newBindingParamsArray();
        for (let p of params) {
          let bindings = paramsArray.newBindingParams();
          for (let [key, value] of Iterator(p)) {
            bindings.bindByName(key, value);
          }
          paramsArray.addParams(bindings);
        }

        statement.bindParameters(paramsArray);
        return;
      }

      
      for (let i = 0; i < params.length; i++) {
        statement.bindByIndex(i, params[i]);
      }
      return;
    }

    
    if (params && typeof(params) == "object") {
      for (let k in params) {
        statement.bindByName(k, params[k]);
      }
      return;
    }

    throw new Error("Invalid type for bound parameters. Expected Array or " +
                    "object. Got: " + params);
  },

  _executeStatement: function (sql, statement, params, onRow) {
    if (statement.state != statement.MOZ_STORAGE_STATEMENT_READY) {
      throw new Error("Statement is not ready for execution.");
    }

    if (onRow && typeof(onRow) != "function") {
      throw new Error("onRow must be a function. Got: " + onRow);
    }

    this._bindParameters(statement, params);

    let index = this._statementCounter++;

    let deferred = Promise.defer();
    let userCancelled = false;
    let errors = [];
    let rows = [];

    
    
    if (this._log.level <= Log.Level.Trace) {
      let msg = "Stmt #" + index + " " + sql;

      if (params) {
        msg += " - " + JSON.stringify(params);
      }
      this._log.trace(msg);
    } else {
      this._log.debug("Stmt #" + index + " starting");
    }

    let self = this;
    let pending = statement.executeAsync({
      handleResult: function (resultSet) {
        
        
        for (let row = resultSet.getNextRow(); row && !userCancelled; row = resultSet.getNextRow()) {
          if (!onRow) {
            rows.push(row);
            continue;
          }

          try {
            onRow(row);
          } catch (e if e instanceof StopIteration) {
            userCancelled = true;
            pending.cancel();
            break;
          } catch (ex) {
            self._log.warn("Exception when calling onRow callback: " +
                           CommonUtils.exceptionStr(ex));
          }
        }
      },

      handleError: function (error) {
        self._log.info("Error when executing SQL (" + error.result + "): " +
                       error.message);
        errors.push(error);
      },

      handleCompletion: function (reason) {
        self._log.debug("Stmt #" + index + " finished.");
        self._pendingStatements.delete(index);

        switch (reason) {
          case Ci.mozIStorageStatementCallback.REASON_FINISHED:
            
            let result = onRow ? null : rows;
            deferred.resolve(result);
            break;

          case Ci.mozIStorageStatementCallback.REASON_CANCELLED:
            
            
            if (userCancelled) {
              let result = onRow ? null : rows;
              deferred.resolve(result);
            } else {
              deferred.reject(new Error("Statement was cancelled."));
            }

            break;

          case Ci.mozIStorageStatementCallback.REASON_ERROR:
            let error = new Error("Error(s) encountered during statement execution.");
            error.errors = errors;
            deferred.reject(error);
            break;

          default:
            deferred.reject(new Error("Unknown completion reason code: " +
                                      reason));
            break;
        }
      },
    });

    this._pendingStatements.set(index, pending);
    return deferred.promise;
  },

  _ensureOpen: function () {
    if (!this._open) {
      throw new Error("Connection is not open.");
    }
  },

  _clearIdleShrinkTimer: function () {
    if (!this._idleShrinkTimer) {
      return;
    }

    this._idleShrinkTimer.cancel();
  },

  _startIdleShrinkTimer: function () {
    if (!this._idleShrinkTimer) {
      return;
    }

    this._idleShrinkTimer.initWithCallback(this.shrinkMemory.bind(this),
                                           this._idleShrinkMS,
                                           this._idleShrinkTimer.TYPE_ONE_SHOT);
  },
});

this.Sqlite = {
  openConnection: openConnection,
  cloneStorageConnection: cloneStorageConnection,
  





  get shutdown() {
    return Barriers.shutdown.client;
  }
};
