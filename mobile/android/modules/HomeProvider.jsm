




"use strict";

this.EXPORTED_SYMBOLS = [ "HomeProvider" ];

const { utils: Cu } = Components;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Sqlite.jsm");
Cu.import("resource://gre/modules/Task.jsm");

const SCHEMA_VERSION = 1;

const DB_PATH = OS.Path.join(OS.Constants.Path.profileDir, "home.sqlite");




const SQL = {
  createItemsTable:
    "CREATE TABLE items (" +
      "_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
      "dataset_id TEXT NOT NULL, " +
      "url TEXT," +
      "title TEXT," +
      "description TEXT," +
      "image_url TEXT," +
      "created INTEGER" +
    ")",

  insertItem:
    "INSERT INTO items (dataset_id, url, title, description, image_url, created) " +
      "VALUES (:dataset_id, :url, :title, :description, :image_url, :created)",

  deleteFromDataset:
    "DELETE FROM items WHERE dataset_id = :dataset_id"
}

this.HomeProvider = Object.freeze({
  







  getStorage: function(datasetId) {
    return new HomeStorage(datasetId);
  }
});

this.HomeStorage = function(datasetId) {
  this.datasetId = datasetId;
};

HomeStorage.prototype = {
  








  save: function(data) {
    return Task.spawn(function save_task() {
      let db = yield Sqlite.openConnection({ path: DB_PATH });

      try {
        
        if (!(yield db.tableExists("items"))) {
          yield db.execute(SQL.createItemsTable);
          yield db.setSchemaVersion(SCHEMA_VERSION);
        }

        
        for (let item of data) {
          
          let params = {
            dataset_id: this.datasetId,
            url: item.url,
            title: item.title,
            description: item.description,
            image_url: item.image_url,
            created: Date.now()
          };
          yield db.executeCached(SQL.insertItem, params);
        }
      } finally {
        yield db.close();
      }
    }.bind(this));
  },

  





  deleteAll: function() {
    return Task.spawn(function delete_all_task() {
      let db = yield Sqlite.openConnection({ path: DB_PATH });

      try {
        
        let params = { dataset_id: this.datasetId };
        yield db.executeCached(SQL.deleteFromDataset, params);
      } finally {
        yield db.close();
      }
    }.bind(this));
  }
};
