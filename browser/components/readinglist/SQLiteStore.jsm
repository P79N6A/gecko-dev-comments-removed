



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
  this._ensureConnection(pathRelativeToProfileDir);
};

this.SQLiteStore.prototype = {

  







  count: Task.async(function* (...optsList) {
    let [sql, args] = sqlFromOptions(optsList);
    let count = 0;
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      SELECT COUNT(*) AS count FROM items ${sql};
    `, args, row => count = row.getResultByName("count"));
    return count;
  }),

  









  forEachItem: Task.async(function* (callback, ...optsList) {
    let [sql, args] = sqlFromOptions(optsList);
    let colNames = ReadingList.ItemBasicPropertyNames;
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
    yield conn.executeCached(`
      INSERT INTO items (${colNames}) VALUES (${paramNames});
    `, item);
  }),

  







  updateItem: Task.async(function* (item) {
    let assignments = [];
    for (let propName in item) {
      assignments.push(`${propName} = :${propName}`);
    }
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      UPDATE items SET ${assignments} WHERE url = :url;
    `, item);
  }),

  






  deleteItemByURL: Task.async(function* (url) {
    let conn = yield this._connectionPromise;
    yield conn.executeCached(`
      DELETE FROM items WHERE url = :url;
    `, { url: url });
  }),

  


  destroy: Task.async(function* () {
    let conn = yield this._connectionPromise;
    yield conn.close();
    this._connectionPromise = Promise.reject("Store destroyed");
  }),

  





  _ensureConnection: Task.async(function* (pathRelativeToProfileDir) {
    if (!this._connectionPromise) {
      this._connectionPromise = Task.spawn(function* () {
        let conn = yield Sqlite.openConnection({
          path: pathRelativeToProfileDir,
          sharedMemoryCache: false,
        });
        Sqlite.shutdown.addBlocker("readinglist/SQLiteStore: Destroy",
                                   this.destroy.bind(this));
        yield conn.execute(`
          PRAGMA locking_mode = EXCLUSIVE;
        `);
        yield this._checkSchema(conn);
        return conn;
      }.bind(this));
    }
  }),

  
  _connectionPromise: null,

  
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
        url TEXT NOT NULL UNIQUE,
        resolvedURL TEXT UNIQUE,
        lastModified INTEGER,
        title TEXT,
        resolvedTitle TEXT,
        excerpt TEXT,
        status INTEGER,
        favorite BOOLEAN,
        isArticle BOOLEAN,
        wordCount INTEGER,
        unread BOOLEAN,
        addedBy TEXT,
        addedOn INTEGER,
        storedOn INTEGER,
        markedReadBy TEXT,
        markedReadOn INTEGER,
        readPosition INTEGER
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
  for (let name of ReadingList.ItemBasicPropertyNames) {
    item[name] = row.getResultByName(name);
  }
  return item;
}









function sqlFromOptions(optsList) {
  
  
  optsList = Cu.cloneInto(optsList, {}, { cloneFunctions: false });

  let sort;
  let sortDir;
  let limit;
  let offset;
  for (let opts of optsList) {
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

  function uniqueParamName(name) {
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

  
  
  let disjunctions = [];
  for (let opts of optsList) {
    let conjunctions = [];
    for (let key in opts) {
      if (Array.isArray(opts[key])) {
        
        
        
        let array = opts[key];
        let params = [];
        for (let i = 0; i < array.length; i++) {
          let paramName = uniqueParamName(key);
          params.push(`:${paramName}`);
          args[paramName] = array[i];
        }
        conjunctions.push(`${key} IN (${params})`);
      }
      else {
        let paramName = uniqueParamName(key);
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
  if (disjunction) {
    let where = `WHERE ${disjunction}`;
    fragments = [where].concat(fragments);
  }

  let sql = fragments.join(" ");
  return [sql, args];
}
