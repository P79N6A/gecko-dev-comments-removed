


"use strict";

let xulApp = require("sdk/system/xul-app");
if (xulApp.versionInRange(xulApp.platformVersion, "16.0a1", "*")) {
new function tests() {

const { indexedDB, IDBKeyRange, DOMException
      } = require("sdk/indexed-db");

exports["test indexedDB is frozen"] = function(assert){
  let original = indexedDB.open;
  let f = function(){};
  assert.throws(function(){indexedDB.open = f});
  assert.equal(indexedDB.open,original);
  assert.notEqual(indexedDB.open,f);

};

exports["test db variables"] = function(assert) {
  [ indexedDB, IDBKeyRange, DOMException
  ].forEach(function(value) {
    assert.notEqual(typeof(value), "undefined", "variable is defined");
  });
}

exports["test open"] = function(assert, done) {
  testOpen(0, assert, done);
}

function testOpen(step, assert, done) {
  const dbName = "MyTestDatabase";
  const openParams = [
    { dbName: "MyTestDatabase", dbVersion: 10 },
    { dbName: "MyTestDatabase" },
    { dbName: "MyTestDatabase", dbOptions: { storage: "temporary" } },
    { dbName: "MyTestDatabase", dbOptions: { version: 20, storage: "default" } }
  ];

  let params = openParams[step];

  let request;
  let expectedStorage;
  let expectedVersion;
  let upgradeNeededCalled = false;
  if ("dbVersion" in params) {
    request = indexedDB.open(params.dbName, params.dbVersion);
    expectedVersion = params.dbVersion;
    expectedStorage = "persistent";
  } else if ("dbOptions" in params) {
    request = indexedDB.open(params.dbName, params.dbOptions);
    if ("version" in params.dbOptions) {
      expectedVersion = params.dbOptions.version;
    } else {
      expectedVersion = 1;
    }
    if ("storage" in params.dbOptions) {
      expectedStorage = params.dbOptions.storage;
    } else {
      expectedStorage = "persistent";
    }
  } else {
    request = indexedDB.open(params.dbName);
    expectedVersion = 1;
    expectedStorage = "persistent";
  }
  request.onerror = function(event) {
    assert.fail("Failed to open indexedDB")
    done();
  }
  request.onupgradeneeded = function(event) {
    upgradeNeededCalled = true;
    assert.equal(event.oldVersion, 0, "Correct old version");
  }
  request.onsuccess = function(event) {
    assert.pass("IndexedDB was open");
    assert.equal(upgradeNeededCalled, true, "Upgrade needed called");
    let db = request.result;
    assert.equal(db.storage, expectedStorage, "Storage is correct");
    db.onversionchange = function(event) {
      assert.equal(event.oldVersion, expectedVersion, "Old version is correct");
      db.close();
    }
    if ("dbOptions" in params) {
      request = indexedDB.deleteDatabase(params.dbName, params.dbOptions);
    } else {
      request = indexedDB.deleteDatabase(params.dbName);
    }
    request.onerror = function(event) {
      assert.fail("Failed to delete indexedDB")
      done();
    }
    request.onsuccess = function(event) {
      assert.pass("IndexedDB was deleted");

      if (++step == openParams.length) {
        done();
      } else {
        testOpen(step, assert, done);
      }
    }
  }
}

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

require("sdk/test").run(exports);
