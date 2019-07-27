



"use strict";

this.EXPORTED_SYMBOLS = [
  "Sqlite",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;


const TRANSACTIONS_QUEUE_TIMEOUT_MS = 120000 

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
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
XPCOMUtils.defineLazyServiceGetter(this, "FinalizationWitnessService",
                                   "@mozilla.org/toolkit/finalizationwitness;1",
                                   "nsIFinalizationWitnessService");
XPCOMUtils.defineLazyModuleGetter(this, "PromiseUtils",
                                  "resource://gre/modules/PromiseUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");



let connectionCounters = new Map();





let wrappedConnections = new Set();




let isClosed = false;

let Debugging = {
  
  
  
  failTestsOnAutoClose: true
};


function logScriptError(message) {
  let consoleMessage = Cc["@mozilla.org/scripterror;1"].
                       createInstance(Ci.nsIScriptError);
  let stack = new Error();
  consoleMessage.init(message, stack.fileName, null, stack.lineNumber, 0,
                      Ci.nsIScriptError.errorFlag, "component javascript");
  Services.console.logMessage(consoleMessage);

  
  
  
  if (Debugging.failTestsOnAutoClose) {
    Promise.reject(new Error(message));
  }
}








function getIdentifierByPath(path) {
  let basename = OS.Path.basename(path);
  let number = connectionCounters.get(basename) || 0;
  connectionCounters.set(basename, number + 1);
  return basename + "#" + number;
}





XPCOMUtils.defineLazyGetter(this, "Barriers", () => {
  let Barriers = {
    





    shutdown: new AsyncShutdown.Barrier("Sqlite.jsm: wait until all clients have completed their task"),

    




    connections: new AsyncShutdown.Barrier("Sqlite.jsm: wait until all connections are closed"),
  };

  






  let finalizationObserver = function (subject, topic, identifier) {
    let connectionData = ConnectionData.byId.get(identifier);

    if (connectionData === undefined) {
      logScriptError("Error: Attempt to finalize unknown Sqlite connection: " +
                     identifier + "\n");
      return;
    }

    ConnectionData.byId.delete(identifier);
    logScriptError("Warning: Sqlite connection '" + identifier +
                   "' was not properly closed. Auto-close triggered by garbage collection.\n");
    connectionData.close();
  };
  Services.obs.addObserver(finalizationObserver, "sqlite-finalization-witness", false);

  





  AsyncShutdown.profileBeforeChange.addBlocker("Sqlite.jsm shutdown blocker",
    Task.async(function* () {
      yield Barriers.shutdown.wait();
      
      
      

      
      isClosed = true;

      
      yield Barriers.connections.wait();

      
      Services.obs.removeObserver(finalizationObserver, "sqlite-finalization-witness");
    }),

    function status() {
      if (isClosed) {
        
        
        return { description: "Waiting for connections to close",
                 state: Barriers.connections.state };
      }

      
      
      
      return { description: "Waiting for the barrier to be lifted",
               state: Barriers.shutdown.state };
  });

  return Barriers;
});



















function ConnectionData(connection, identifier, options={}) {
  this._log = Log.repository.getLoggerWithMessagePrefix("Sqlite.Connection",
                                                        identifier + ": ");
  this._log.info("Opened");

  this._dbConn = connection;

  
  
  this._identifier = identifier;

  this._open = true;

  this._cachedStatements = new Map();
  this._anonymousStatements = new Map();
  this._anonymousCounter = 0;

  
  
  this._pendingStatements = new Map();

  
  this._statementCounter = 0;

  
  this._operationsCounter = 0;

  this._hasInProgressTransaction = false;
  
  
  this._transactionQueue = Promise.resolve();

  this._idleShrinkMS = options.shrinkMemoryOnConnectionIdleMS;
  if (this._idleShrinkMS) {
    this._idleShrinkTimer = Cc["@mozilla.org/timer;1"]
                              .createInstance(Ci.nsITimer);
    
    
  }

  
  
  this._deferredClose = PromiseUtils.defer();
  this._closeRequested = false;

  
  
  this._barrier = new AsyncShutdown.Barrier(`${this._identifier}: waiting for clients`);

  Barriers.connections.client.addBlocker(
    this._identifier + ": waiting for shutdown",
    this._deferredClose.promise,
    () =>  ({
      identifier: this._identifier,
      isCloseRequested: this._closeRequested,
      hasDbConn: !!this._dbConn,
      hasInProgressTransaction: this._hasInProgressTransaction,
      pendingStatements: this._pendingStatements.size,
      statementCounter: this._statementCounter,
    })
  );
}











ConnectionData.byId = new Map();

ConnectionData.prototype = Object.freeze({
  


























  executeBeforeShutdown: function(parent, name, task) {
    if (!name) {
      throw new TypeError("Expected a human-readable name as first argument");
    }
    if (typeof task != "function") {
      throw new TypeError("Expected a function as second argument");
    }
    if (this._closeRequested) {
      throw new Error(`${this._identifier}: cannot execute operation ${name}, the connection is already closing`);
    }

    
    let status = {
      
      
      command: "<not started>",

      
      isPending: false,
    };

    
    
    
    
    let loggedDb = Object.create(parent, {
      execute: {
        value: Task.async(function*(sql, ...rest) {
          status.isPending = true;
          status.command = sql;
          try {
            return (yield this.execute(sql, ...rest));
          } finally {
            status.isPending = false;
          }
        }.bind(this))
      },
      close: {
        value: Task.async(function*() {
          status.isPending = false;
          status.command = "<close>";
          try {
            return (yield this.close());
          } finally {
            status.isPending = false;
          }
        }.bind(this))
      },
      executeCached: {
        value: Task.async(function*(sql, ...rest) {
          status.isPending = false;
          status.command = sql;
          try {
            return (yield this.executeCached(sql, ...rest));
          } finally {
            status.isPending = false;
          }
        }.bind(this))
      },
    });

    let promiseResult = task(loggedDb);
    if (!promiseResult || typeof promiseResult != "object" || !("then" in promiseResult)) {
      throw new TypeError("Expected a Promise");
    }
    let key = `${this._identifier}: ${name} (${this._getOperationId()})`;
    let promiseComplete = promiseResult.catch(() => {});
    this._barrier.client.addBlocker(key, promiseComplete, {
      fetchState: () => status
    });

    return Task.spawn(function*() {
      try {
        return (yield promiseResult);
      } finally {
        this._barrier.client.removeBlocker(key, promiseComplete)
      }
    }.bind(this));
  },
  close: function () {
    this._closeRequested = true;

    if (!this._dbConn) {
      return this._deferredClose.promise;
    }

    this._log.debug("Request to close connection.");
    this._clearIdleShrinkTimer();

    return this._barrier.wait().then(() => {
      if (!this._dbConn) {
        return;
      }
      return this._finalize();
    });
  },

  clone: function (readOnly=false) {
    this.ensureOpen();

    this._log.debug("Request to clone connection.");

    let options = {
      connection: this._dbConn,
      readOnly: readOnly,
    };
    if (this._idleShrinkMS)
      options.shrinkMemoryOnConnectionIdleMS = this._idleShrinkMS;

    return cloneStorageConnection(options);
  },
  _getOperationId: function() {
    return this._operationsCounter++;
  },
  _finalize: function () {
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

    
    
    let markAsClosed = () => {
      this._log.info("Closed");
      this._dbConn = null;
      
      
      Barriers.connections.client.removeBlocker(this._deferredClose.promise);
      this._deferredClose.resolve();
    }
    if (wrappedConnections.has(this._identifier)) {
      wrappedConnections.delete(this._identifier);
      markAsClosed();
    } else {
      this._log.debug("Calling asyncClose().");
      this._dbConn.asyncClose(markAsClosed);
    }
    return this._deferredClose.promise;
  },

  executeCached: function (sql, params=null, onRow=null) {
    this.ensureOpen();

    if (!sql) {
      throw new Error("sql argument is empty.");
    }

    let statement = this._cachedStatements.get(sql);
    if (!statement) {
      statement = this._dbConn.createAsyncStatement(sql);
      this._cachedStatements.set(sql, statement);
    }

    this._clearIdleShrinkTimer();

    return new Promise((resolve, reject) => {
      try {
        this._executeStatement(sql, statement, params, onRow).then(
          result => {
            this._startIdleShrinkTimer();
            resolve(result);
          },
          error => {
            this._startIdleShrinkTimer();
            reject(error);
          }
        );
      } catch (ex) {
        this._startIdleShrinkTimer();
        throw ex;
      }
    });
  },

  execute: function (sql, params=null, onRow=null) {
    if (typeof(sql) != "string") {
      throw new Error("Must define SQL to execute as a string: " + sql);
    }

    this.ensureOpen();

    let statement = this._dbConn.createAsyncStatement(sql);
    let index = this._anonymousCounter++;

    this._anonymousStatements.set(index, statement);
    this._clearIdleShrinkTimer();

    let onFinished = () => {
      this._anonymousStatements.delete(index);
      statement.finalize();
      this._startIdleShrinkTimer();
    };

    return new Promise((resolve, reject) => {
      try {
        this._executeStatement(sql, statement, params, onRow).then(
          rows => {
            onFinished();
            resolve(rows);
          },
          error => {
            onFinished();
            reject(error);
          }
        );
      } catch (ex) {
        onFinished();
        throw ex;
      }
    });
  },

  get transactionInProgress() {
    return this._open && this._hasInProgressTransaction;
  },

  executeTransaction: function (func, type) {
    if (typeof type == "undefined") {
      throw new Error("Internal error: expected a type");
    }
    this.ensureOpen();

    this._log.debug("Beginning transaction");

    let promise = this._transactionQueue.then(() => {
      if (this._closeRequested) {
        throw new Error("Transaction canceled due to a closed connection.");
      }

      let transactionPromise = Task.spawn(function* () {
        
        
        if (this._hasInProgressTransaction) {
          console.error("Unexpected transaction in progress when trying to start a new one.");
        }
        this._hasInProgressTransaction = true;
        try {
          
          try {
            yield this.execute("BEGIN " + type + " TRANSACTION");
          } catch (ex) {
            
            
            
            
            
            if (wrappedConnections.has(this._identifier)) {
              this._log.warn("A new transaction could not be started cause the wrapped connection had one in progress: " +
                             CommonUtils.exceptionStr(ex));
              
              
              this._hasInProgressTransaction = false;
            } else {
              this._log.warn("A transaction was already in progress, likely a nested transaction: " +
                             CommonUtils.exceptionStr(ex));
              throw ex;
            }
          }

          let result;
          try {
            result = yield Task.spawn(func);
          } catch (ex) {
            
            
            if (this._closeRequested) {
              this._log.warn("Connection closed while performing a transaction: " +
                             CommonUtils.exceptionStr(ex));
            } else {
              this._log.warn("Error during transaction. Rolling back: " +
                             CommonUtils.exceptionStr(ex));
              
              if (this._hasInProgressTransaction) {
                try {
                  yield this.execute("ROLLBACK TRANSACTION");
                } catch (inner) {
                  this._log.warn("Could not roll back transaction: " +
                                 CommonUtils.exceptionStr(inner));
                }
              }
            }
            
            throw ex;
          }

          
          if (this._closeRequested) {
            this._log.warn("Connection closed before committing the transaction.");
            throw new Error("Connection closed before committing the transaction.");
          }

          
          if (this._hasInProgressTransaction) {
            try {
              yield this.execute("COMMIT TRANSACTION");
            } catch (ex) {
              this._log.warn("Error committing transaction: " +
                             CommonUtils.exceptionStr(ex));
              throw ex;
            }
          }

          return result;
        } finally {
          this._hasInProgressTransaction = false;
        }
      }.bind(this));

      
      
      
      
      let timeoutPromise = new Promise((resolve, reject) => {
        setTimeout(() => reject(new Error("Transaction timeout, most likely caused by unresolved pending work.")),
                   TRANSACTIONS_QUEUE_TIMEOUT_MS);
      });
      return Promise.race([transactionPromise, timeoutPromise]);
    });
    
    
    this._transactionQueue = promise.catch(ex => { console.error(ex) });

    
    this._barrier.client.addBlocker(`Transaction (${this._getOperationId()})`,
      this._transactionQueue);
    return promise;
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

    let deferred = PromiseUtils.defer();
    let userCancelled = false;
    let errors = [];
    let rows = [];
    let handledRow = false;

    
    
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

          handledRow = true;

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
        self._log.info("Error when executing SQL (" +
                       error.result + "): " + error.message);
        errors.push(error);
      },

      handleCompletion: function (reason) {
        self._log.debug("Stmt #" + index + " finished.");
        self._pendingStatements.delete(index);

        switch (reason) {
          case Ci.mozIStorageStatementCallback.REASON_FINISHED:
            
            
            let result = onRow ? handledRow : rows;
            deferred.resolve(result);
            break;

          case Ci.mozIStorageStatementCallback.REASON_CANCELED:
            
            
            if (userCancelled) {
              let result = onRow ? handledRow : rows;
              deferred.resolve(result);
            } else {
              deferred.reject(new Error("Statement was cancelled."));
            }

            break;

          case Ci.mozIStorageStatementCallback.REASON_ERROR:
            let error = new Error("Error(s) encountered during statement execution: " + [error.message for (error of errors)].join(", "));
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

  ensureOpen: function () {
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
  }
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
  let identifier = getIdentifierByPath(path);

  log.info("Opening database: " + path + " (" + identifier + ")");

  return new Promise((resolve, reject) => {
    let dbOptions = null;
    if (!sharedMemoryCache) {
      dbOptions = Cc["@mozilla.org/hash-property-bag;1"].
        createInstance(Ci.nsIWritablePropertyBag);
      dbOptions.setProperty("shared", false);
    }
    Services.storage.openAsyncDatabase(file, dbOptions, (status, connection) => {
      if (!connection) {
        log.warn(`Could not open connection to ${path}: ${status}`);
        reject(new Error(`Could not open connection to ${path}: ${status}`));
        return;
      }
      log.info("Connection opened");
      try {
        resolve(
          new OpenedConnection(connection.QueryInterface(Ci.mozIStorageAsyncConnection),
                              identifier, openedOptions));
      } catch (ex) {
        log.warn("Could not open database: " + CommonUtils.exceptionStr(ex));
        reject(ex);
      }
    });
  });
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
    throw new Error("Sqlite.jsm has been shutdown. Cannot clone connection to: " + source.database.path);
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
  let identifier = getIdentifierByPath(path);

  log.info("Cloning database: " + path + " (" + identifier + ")");

  return new Promise((resolve, reject) => {
    source.asyncClone(!!options.readOnly, (status, connection) => {
      if (!connection) {
        log.warn("Could not clone connection: " + status);
        reject(new Error("Could not clone connection: " + status));
      }
      log.info("Connection cloned");
      try {
        let conn = connection.QueryInterface(Ci.mozIStorageAsyncConnection);
        resolve(new OpenedConnection(conn, identifier, openedOptions));
      } catch (ex) {
        log.warn("Could not clone database: " + CommonUtils.exceptionStr(ex));
        reject(ex);
      }
    });
  });
}



















function wrapStorageConnection(options) {
  let log = Log.repository.getLogger("Sqlite.ConnectionWrapper");

  let connection = options && options.connection;
  if (!connection || !(connection instanceof Ci.mozIStorageAsyncConnection)) {
    throw new TypeError("connection not specified or invalid.");
  }

  if (isClosed) {
    throw new Error("Sqlite.jsm has been shutdown. Cannot wrap connection to: " + connection.database.path);
  }

  let path = connection.databaseFile.path;
  let identifier = getIdentifierByPath(path);

  log.info("Wrapping database: " + path + " (" + identifier + ")");
  return new Promise(resolve => {
    try {
      let conn = connection.QueryInterface(Ci.mozIStorageAsyncConnection);
      let wrapper = new OpenedConnection(conn, identifier);
      
      
      wrappedConnections.add(identifier);
      resolve(wrapper);
    } catch (ex) {
      log.warn("Could not wrap database: " + CommonUtils.exceptionStr(ex));
      throw ex;
    }
  });
}















































function OpenedConnection(connection, identifier, options={}) {
  
  
  
  
  
  this._connectionData = new ConnectionData(connection, identifier, options);

  
  
  ConnectionData.byId.set(this._connectionData._identifier,
                          this._connectionData);

  
  
  
  
  this._witness = FinalizationWitnessService.make(
    "sqlite-finalization-witness",
    this._connectionData._identifier);
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
    this._connectionData.ensureOpen();
    return this.execute("PRAGMA user_version = " + value);
  },

  

















  close: function () {
    
    
    
    if (ConnectionData.byId.has(this._connectionData._identifier)) {
      ConnectionData.byId.delete(this._connectionData._identifier);
      this._witness.forget();
    }
    return this._connectionData.close();
  },

  













  clone: function (readOnly=false) {
    return this._connectionData.clone(readOnly);
  },

  executeBeforeShutdown: function(name, task) {
    return this._connectionData.executeBeforeShutdown(this, name, task);
  },

  

























































  executeCached: function (sql, params=null, onRow=null) {
    return this._connectionData.executeCached(sql, params, onRow);
  },

  














  execute: function (sql, params=null, onRow=null) {
    return this._connectionData.execute(sql, params, onRow);
  },

  


  get transactionInProgress() {
    return this._connectionData.transactionInProgress;
  },

  





































  executeTransaction: function (func, type=this.TRANSACTION_DEFERRED) {
    if (this.TRANSACTION_TYPES.indexOf(type) == -1) {
      throw new Error("Unknown transaction type: " + type);
    }

    return this._connectionData.executeTransaction(() => func(this), type);
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
    return this._connectionData.shrinkMemory();
  },

  









  discardCachedStatements: function () {
    return this._connectionData.discardCachedStatements();
  },
});

this.Sqlite = {
  openConnection: openConnection,
  cloneStorageConnection: cloneStorageConnection,
  wrapStorageConnection: wrapStorageConnection,
  





  get shutdown() {
    return Barriers.shutdown.client;
  }
};
