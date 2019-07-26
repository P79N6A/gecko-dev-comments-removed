



"use strict";

this.EXPORTED_SYMBOLS = [
  "Sqlite",
];

const {interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/log4moz.js");

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");




let connectionCounters = {};





























function openConnection(options) {
  let log = Log4Moz.repository.getLogger("Sqlite.ConnectionOpener");

  if (!options.path) {
    throw new Error("path not specified in connection options.");
  }

  
  let path = OS.Path.join(OS.Constants.Path.profileDir, options.path);

  let sharedMemoryCache = "sharedMemoryCache" in options ?
                            options.sharedMemoryCache : true;

  let file = FileUtils.File(path);
  let openDatabaseFn = sharedMemoryCache ?
                         Services.storage.openDatabase :
                         Services.storage.openUnsharedDatabase;

  let basename = OS.Path.basename(path);

  if (!connectionCounters[basename]) {
    connectionCounters[basename] = 1;
  }

  let number = connectionCounters[basename]++;
  let identifier = basename + "#" + number;

  log.info("Opening database: " + path + " (" + identifier + ")");
  try {
    let connection = openDatabaseFn(file);

    if (!connection.connectionReady) {
      log.warn("Connection is not ready.");
      return Promise.reject(new Error("Connection is not ready."));
    }

    return Promise.resolve(new OpenedConnection(connection, basename, number));
  } catch (ex) {
    log.warn("Could not open database: " + CommonUtils.exceptionStr(ex));
    return Promise.reject(ex);
  }
}














































function OpenedConnection(connection, basename, number) {
  let log = Log4Moz.repository.getLogger("Sqlite.Connection." + basename);

  
  for (let level in Log4Moz.Level) {
    if (level == "Desc") {
      continue;
    }

    let lc = level.toLowerCase();
    log[lc] = function (msg) {
      return Log4Moz.Logger.prototype[lc].call(log, "Conn #" + number + ": " + msg);
    }
  }

  this._log = log;

  this._log.info("Opened");

  this._connection = connection;
  this._open = true;

  this._cachedStatements = new Map();
  this._anonymousStatements = new Map();
  this._anonymousCounter = 0;
  this._inProgressStatements = new Map();
  this._inProgressCounter = 0;

  this._inProgressTransaction = null;
}

OpenedConnection.prototype = Object.freeze({
  TRANSACTION_DEFERRED: Ci.mozIStorageConnection.TRANSACTION_DEFERRED,
  TRANSACTION_IMMEDIATE: Ci.mozIStorageConnection.TRANSACTION_IMMEDIATE,
  TRANSACTION_EXCLUSIVE: Ci.mozIStorageConnection.TRANSACTION_EXCLUSIVE,

  get connectionReady() {
    return this._open && this._connection.connectionReady;
  },

  










  get lastInsertRowID() {
    this._ensureOpen();
    return this._connection.lastInsertRowID;
  },

  






  get affectedRows() {
    this._ensureOpen();
    return this._connection.affectedRows;
  },

  




  get schemaVersion() {
    this._ensureOpen();
    return this._connection.schemaVersion;
  },

  set schemaVersion(value) {
    this._ensureOpen();
    this._connection.schemaVersion = value;
  },

  
















  close: function () {
    if (!this._connection) {
      return Promise.resolve();
    }

    this._log.debug("Closing.");

    
    if (this._inProgressTransaction) {
      this._log.warn("Transaction in progress at time of close.");
      try {
        this._connection.rollbackTransaction();
      } catch (ex) {
        this._log.warn("Error rolling back transaction: " +
                       CommonUtils.exceptionStr(ex));
      }
      this._inProgressTransaction.reject(new Error("Connection being closed."));
      this._inProgressTransaction = null;
    }

    
    for (let [k, statement] of this._inProgressStatements) {
      statement.cancel();
    }
    this._inProgressStatements.clear();

    
    for (let [k, statement] of this._anonymousStatements) {
      statement.finalize();
    }
    this._anonymousStatements.clear();

    for (let [k, statement] of this._cachedStatements) {
      statement.finalize();
    }
    this._cachedStatements.clear();

    
    
    this._open = false;

    let deferred = Promise.defer();

    this._log.debug("Calling asyncClose().");
    this._connection.asyncClose({
      complete: function () {
        this._log.info("Closed");
        this._connection = null;
        deferred.resolve();
      }.bind(this),
    });

    return deferred.promise;
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

    return this._executeStatement(sql, statement, params, onRow);
  },

  














  execute: function (sql, params=null, onRow=null) {
    if (typeof(sql) != "string") {
      throw new Error("Must define SQL to execute as a string: " + sql);
    }

    this._ensureOpen();

    let statement = this._connection.createAsyncStatement(sql);
    let index = this._anonymousCounter++;

    this._anonymousStatements.set(index, statement);

    let deferred = Promise.defer();

    this._executeStatement(sql, statement, params, onRow).then(
      function onResult(rows) {
        this._anonymousStatements.delete(index);
        statement.finalize();
        deferred.resolve(rows);
      }.bind(this),

      function onError(error) {
        this._anonymousStatements.delete(index);
        statement.finalize();
        deferred.reject(error);
      }.bind(this)
    );

    return deferred.promise;
  },

  


  get transactionInProgress() {
    return this._open && this._connection.transactionInProgress;
  },

  






















  executeTransaction: function (func, type=this.TRANSACTION_DEFERRED) {
    this._ensureOpen();

    if (this.transactionInProgress) {
      throw new Error("A transaction is already active. Only one transaction " +
                      "can be active at a time.");
    }

    this._log.debug("Beginning transaction");
    this._connection.beginTransactionAs(type);

    let deferred = Promise.defer();
    this._inProgressTransaction = deferred;

    Task.spawn(func(this)).then(
      function onSuccess (result) {
        this._connection.commitTransaction();
        this._inProgressTransaction = null;
        this._log.debug("Transaction committed.");

        deferred.resolve(result);
      }.bind(this),

      function onError (error) {
        this._log.warn("Error during transaction. Rolling back: " +
                       CommonUtils.exceptionStr(error));
        this._connection.rollbackTransaction();
        this._inProgressTransaction = null;

        deferred.reject(error);
      }.bind(this)
    );

    return deferred.promise;
  },

  









  tableExists: function (name) {
    return this.execute(
      "SELECT name FROM sqlite_master WHERE type='table' AND name=?",
      [name])
      .then(function onResult(rows) {
        return Promise.resolve(rows.length > 0);
      }
    );
  },

  









  indexExists: function (name) {
    return this.execute(
      "SELECT name FROM sqlite_master WHERE type='index' AND name=?",
      [name])
      .then(function onResult(rows) {
        return Promise.resolve(rows.length > 0);
      }
    );
  },

  _executeStatement: function (sql, statement, params, onRow) {
    if (statement.state != statement.MOZ_STORAGE_STATEMENT_READY) {
      throw new Error("Statement is not ready for execution.");
    }

    if (onRow && typeof(onRow) != "function") {
      throw new Error("onRow must be a function. Got: " + onRow);
    }

    if (Array.isArray(params)) {
      for (let i = 0; i < params.length; i++) {
        statement.bindByIndex(i, params[i]);
      }
    } else if (params && typeof(params) == "object") {
      for (let k in params) {
        statement.bindByName(k, params[k]);
      }
    } else if (params) {
      throw new Error("Invalid type for bound parameters. Expected Array or " +
                      "object. Got: " + params);
    }

    let index = this._inProgressCounter++;

    let deferred = Promise.defer();
    let userCancelled = false;
    let errors = [];
    let rows = [];

    
    
    if (this._log.level <= Log4Moz.Level.Trace) {
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
        self._log.debug("Stmt #" + index + " finished");
        self._inProgressStatements.delete(index);

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

    this._inProgressStatements.set(index, pending);

    return deferred.promise;
  },

  _ensureOpen: function () {
    if (!this._open) {
      throw new Error("Connection is not open.");
    }
  },
});

this.Sqlite = {
  openConnection: openConnection,
};
