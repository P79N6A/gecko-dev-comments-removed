




this.EXPORTED_SYMBOLS = [
  "GlobalObjectsModule"
];

this.GlobalObjectsModule = function GlobalObjectsModule() {
}

GlobalObjectsModule.prototype = {
  runTest: function() {
    const name = "Splendid Test";

    let ok = this.ok;
    let finishTest = this.finishTest;

    let keyRange = IDBKeyRange.only(42);
    ok(keyRange, "Got keyRange");

    let request = indexedDB.open(name, 1);
    request.onerror = function(event) {
      ok(false, "indexedDB error, '" + event.target.error.name + "'");
      finishTest();
    }
    request.onsuccess = function(event) {
      let db = event.target.result;
      ok(db, "Got database");
      finishTest();
    }
  }
}
