



"use strict";

this.EXPORTED_SYMBOLS = [
  "SQLiteStore",
];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ReadingList",
  "resource:///modules/readinglist/ReadingList.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
  "resource://gre/modules/Sqlite.jsm");








this.SQLiteStore = function SQLiteStore(pathRelativeToProfileDir) {
  this.pathRelativeToProfileDir = pathRelativeToProfileDir;
};

this.SQLiteStore.prototype = {

  










  count: Task.async(function* (userOptsList=[], controlOpts={}) {
    let [sql, args] = sqlWhereFromOptions(userOptsList, controlOpts);
    let count = 0;
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      SELECT COUNT(*) AS count FROM items ${sql};
    `, args, row => count = row.getResultByName("count"));
    return count;
  }),

  












  forEachItem: Task.async(function* (callback, userOptsList=[], controlOpts={}) {
    let [sql, args] = sqlWhereFromOptions(userOptsList, controlOpts);
    let colNames = ReadingList.ItemRecordProperties;
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      SELECT ${colNames} FROM items ${sql};
    `, args, row => callback(itemFromRow(row)));
  }),

  







  addItem: Task.async(function* (item) {
    let colNames = [];
    let paramNames = [];
    for (let propName in item) {
      colNames.push(propName);
      paramNames.push(`:${propName}`);
    }
    let conn = yield this._connectionPromise;
    try {
      yield conn.executeCached(`
        INSERT INTO items (${colNames}) VALUES (${paramNames});
      `, item);
    }
    catch (err) {
      throwExistsError(err);
    }
  }),

  







  updateItem: Task.async(function* (item) {
    yield this._updateItem(item, "url");
  }),

  







  updateItemByGUID: Task.async(function* (item) {
    yield this._updateItem(item, "guid");
  }),

  






  deleteItemByURL: Task.async(function* (url) {
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      DELETE FROM items WHERE url = :url;
    `, { url: url });
  }),

  






  deleteItemByGUID: Task.async(function* (guid) {
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      DELETE FROM items WHERE guid = :guid;
    `, { guid: guid });
  }),

  


  destroy() {
    if (!this._destroyPromise) {
      this._destroyPromise = Task.spawn(function* () {
        let conn = yield this._connectionPromise;
        yield conn.close();
        this.__connectionPromise = Promise.reject("Store destroyed");
      }.bind(this));
    }
    return this._destroyPromise;
  },

  


  get _connectionPromise() {
    if (!this.__connectionPromise) {
      this.__connectionPromise = this._createConnection();
    }
    return this.__connectionPromise;
  },

  




  _createConnection: Task.async(function* () {
    let conn = yield Sqlite.openConnection({
      path: this.pathRelativeToProfileDir,
      sharedMemoryCache: false,
    });
    Sqlite.shutdown.addBlocker("readinglist/SQLiteStore: Destroy",
                               this.destroy.bind(this));
    yield conn.execute(`
      PRAGMA locking_mode = EXCLUSIVE;
    `);
    yield this._checkSchema(conn);
    return conn;
  }),

  









  _updateItem: Task.async(function* (item, keyProp) {
    let assignments = [];
    for (let propName in item) {
      assignments.push(`${propName} = :${propName}`);
    }
    let conn = yield this._connectionPromise;
    if (!item[keyProp]) {
      throw new ReadingList.Error.Error("Item must have " + keyProp);
    }
    try {
      yield conn.executeCached(`
        UPDATE items SET ${assignments} WHERE ${keyProp} = :${keyProp};
      `, item);
    }
    catch (err) {
      throwExistsError(err);
    }
  }),

  
  _schemaVersion: 1,

  _checkSchema: Task.async(function* (conn) {
    let version = parseInt(yield conn.getSchemaVersion());
    for (; version < this._schemaVersion; version++) {
      let meth = `_migrateSchema${version}To${version + 1}`;
      yield this[meth](conn);
    }
    yield conn.setSchemaVersion(this._schemaVersion);
  }),

  _migrateSchema0To1: Task.async(function* (conn) {
    yield conn.execute(`
      PRAGMA journal_mode = wal;
    `);
    
    yield conn.execute(`
      PRAGMA journal_size_limit = 524288;
    `);
    
    
    
    
    yield conn.execute(`
      CREATE TABLE items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        guid TEXT UNIQUE,
        serverLastModified INTEGER,
        url TEXT UNIQUE,
        preview TEXT,
        title TEXT,
        resolvedURL TEXT UNIQUE,
        resolvedTitle TEXT,
        excerpt TEXT,
        archived BOOLEAN,
        deleted BOOLEAN,
        favorite BOOLEAN,
        isArticle BOOLEAN,
        wordCount INTEGER,
        unread BOOLEAN,
        addedBy TEXT,
        addedOn INTEGER,
        storedOn INTEGER,
        markedReadBy TEXT,
        markedReadOn INTEGER,
        readPosition INTEGER,
        syncStatus INTEGER
      );
    `);
    yield conn.execute(`
      CREATE INDEX items_addedOn ON items (addedOn);
    `);
    yield conn.execute(`
      CREATE INDEX items_unread ON items (unread);
    `);
  }),
};








function itemFromRow(row) {
  let item = {};
  for (let name of ReadingList.ItemRecordProperties) {
    item[name] = row.getResultByName(name);
  }
  return item;
}








function throwExistsError(err) {
  let match =
    /UNIQUE constraint failed: items\.([a-zA-Z0-9_]+)/.exec(err.message);
  if (match) {
    let newErr = new ReadingList.Error.Exists(
      "An item with the following property already exists: " + match[1]
    );
    newErr.originalError = err;
    err = newErr;
  }
  throw err;
}













function sqlWhereFromOptions(userOptsList, controlOpts) {
  
  
  userOptsList = Cu.cloneInto(userOptsList, {}, { cloneFunctions: false });

  let sort;
  let sortDir;
  let limit;
  let offset;
  for (let opts of userOptsList) {
    if ("sort" in opts) {
      sort = opts.sort;
      delete opts.sort;
    }
    if ("descending" in opts) {
      if (opts.descending) {
        sortDir = "DESC";
      }
      delete opts.descending;
    }
    if ("limit" in opts) {
      limit = opts.limit;
      delete opts.limit;
    }
    if ("offset" in opts) {
      offset = opts.offset;
      delete opts.offset;
    }
  }

  let fragments = [];

  if (sort) {
    sortDir = sortDir || "ASC";
    fragments.push(`ORDER BY ${sort} ${sortDir}`);
  }
  if (limit) {
    fragments.push(`LIMIT ${limit}`);
    if (offset) {
      fragments.push(`OFFSET ${offset}`);
    }
  }

  let args = {};
  let mainExprs = [];

  let controlSQLExpr = sqlExpressionFromOptions([controlOpts], args);
  if (controlSQLExpr) {
    mainExprs.push(`(${controlSQLExpr})`);
  }

  let userSQLExpr = sqlExpressionFromOptions(userOptsList, args);
  if (userSQLExpr) {
    mainExprs.push(`(${userSQLExpr})`);
  }

  if (mainExprs.length) {
    let conjunction = mainExprs.join(" AND ");
    fragments.unshift(`WHERE ${conjunction}`);
  }

  let sql = fragments.join(" ");
  return [sql, args];
}

















function sqlExpressionFromOptions(optsList, args) {
  let disjunctions = [];
  for (let opts of optsList) {
    let conjunctions = [];
    for (let key in opts) {
      if (Array.isArray(opts[key])) {
        
        
        
        let array = opts[key];
        let params = [];
        for (let i = 0; i < array.length; i++) {
          let paramName = uniqueParamName(args, key);
          params.push(`:${paramName}`);
          args[paramName] = array[i];
        }
        conjunctions.push(`${key} IN (${params})`);
      }
      else {
        let paramName = uniqueParamName(args, key);
        conjunctions.push(`${key} = :${paramName}`);
        args[paramName] = opts[key];
      }
    }
    let conjunction = conjunctions.join(" AND ");
    if (conjunction) {
      disjunctions.push(`(${conjunction})`);
    }
  }
  let disjunction = disjunctions.join(" OR ");
  return disjunction;
}










function uniqueParamName(args, name) {
  if (name in args) {
    for (let i = 1; ; i++) {
      let newName = `${name}_${i}`;
      if (!(newName in args)) {
        return newName;
      }
    }
  }
  return name;
}
