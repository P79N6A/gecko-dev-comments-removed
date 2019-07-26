



"use strict";

let xulApp = require("sdk/system/xul-app");
if (xulApp.versionInRange(xulApp.platformVersion, "16.0a1", "*")) {
new function tests() {

const { indexedDB, IDBKeyRange, DOMException, IDBCursor, IDBTransaction,
        IDBOpenDBRequest, IDBVersionChangeEvent, IDBDatabase, IDBIndex,
        IDBObjectStore, IDBRequest
      } = require("sdk/indexed-db");

exports["test indexedDB is frozen"] = function(assert){
  let original = indexedDB.open;
  let f = function(){};
  assert.throws(function(){indexedDB.open = f});
  assert.equal(indexedDB.open,original);
  assert.notEqual(indexedDB.open,f);

};

exports["test db variables"] = function(assert) {
  [ indexedDB, IDBKeyRange, DOMException, IDBCursor, IDBTransaction,
    IDBOpenDBRequest, IDBOpenDBRequest, IDBVersionChangeEvent,
    IDBDatabase, IDBIndex, IDBObjectStore, IDBRequest
  ].forEach(function(value) {
    assert.notEqual(typeof(value), "undefined", "variable is defined");
  });
}

exports["test open"] = function(assert, done) {
  let request = indexedDB.open("MyTestDatabase");
  request.onerror = function(event) {
    assert.fail("Failed to open indexedDB")
    done();
  };
  request.onsuccess = function(event) {
    assert.pass("IndexedDB was open");
    done();
  };
};

exports["test dbname is unprefixed"] = function(assert, done) {
  
  let dbName = "dbname-unprefixed";
  let request = indexedDB.open(dbName);
  request.onerror = function(event) {
    assert.fail("Failed to open db");
    done();
  };
  request.onsuccess = function(event) {
    assert.equal(request.result.name, dbName);
    done();
  };
};

exports["test structuring the database"] = function(assert, done) {
  
  let customerData = [
    { ssn: "444-44-4444", name: "Bill", age: 35, email: "bill@company.com" },
    { ssn: "555-55-5555", name: "Donna", age: 32, email: "donna@home.org" }
  ];
  let dbName = "the_name";
  let request = indexedDB.open(dbName, 2);
  request.onerror = function(event) {
    assert.fail("Failed to open db");
    done();
  };
  request.onsuccess = function(event) {
    assert.pass("transaction is complete");
    testRead(assert, done);
  }
  request.onupgradeneeded = function(event) {
    assert.pass("data base upgrade")

    var db = event.target.result;

    
    
    
    var objectStore = db.createObjectStore("customers", { keyPath: "ssn" });

    
    
    objectStore.createIndex("name", "name", { unique: false });

    
    
    objectStore.createIndex("email", "email", { unique: true });

    
    customerData.forEach(function(data) {
      objectStore.add(data);
    });
    assert.pass("data added to object store");
  };
};

function testRead(assert, done) {
  let dbName = "the_name";
  let request = indexedDB.open(dbName, 2);
  request.onsuccess = function(event) {
    assert.pass("data opened")
    var db = event.target.result;
    let transaction = db.transaction(["customers"]);
    var objectStore = transaction.objectStore("customers");
    var request = objectStore.get("444-44-4444");
    request.onerror = function(event) {
      assert.fail("Failed to retrive data")
    };
    request.onsuccess = function(event) {
      
      assert.equal(request.result.name, "Bill", "Name is correct");
      done();
    };
  };
  request.onerror = function() {
    assert.fail("failed to open db");
  };
};

}
} else {
  exports.testDB = function(assert) {
    assert.pass("IndexedDB is not implemented")
  }
}

require("test").run(exports);
