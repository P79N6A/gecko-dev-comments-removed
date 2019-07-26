




var testGenerator = testSteps();

function testSteps()
{
  const name = this.window ? window.location.pathname : "Splendid Test";

  const objectStoreData = [
    { key: "1", value: "foo" },
    { key: "2", value: "bar" },
    { key: "3", value: "baz" }
  ];

  let request = mozIndexedDB.open(name, 1);
  request.onerror = errorHandler;
  request.onupgradeneeded = grabEventAndContinueHandler;
  request.onsuccess = grabEventAndContinueHandler;
  let event = yield; 

  let db = event.target.result;

  let objectStore = db.createObjectStore("data", { keyPath: null });

  
  let addedData = 0;
  for (let i in objectStoreData) {
    request = objectStore.add(objectStoreData[i].value,
                              objectStoreData[i].key);
    request.onerror = errorHandler;
    request.onsuccess = function(event) {
      if (++addedData == objectStoreData.length) {
        testGenerator.send(event);
      }
    }
  }
  event = yield; 

  
  objectStore.createIndex("set", "", { unique: true });
  yield; 

  let trans = db.transaction("data", "readwrite");
  objectStore = trans.objectStore("data");
  index = objectStore.index("set");

  let request = index.get("bar");
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;
  
  let event = yield;

  is(event.target.result, "bar", "Got correct result");

  let request = objectStore.add("foopy", 4);
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;

  yield;

  let request = index.get("foopy");
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;
  
  let event = yield;

  is(event.target.result, "foopy", "Got correct result");

  let request = objectStore.add("foopy", 5);
  request.onerror = new ExpectError("ConstraintError", true);
  request.onsuccess = unexpectedSuccessHandler;

  trans.oncomplete = grabEventAndContinueHandler;

  yield;
  yield;

  finishTest();
  yield;
}
