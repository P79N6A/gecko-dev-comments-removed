




var testGenerator = testSteps();

function testSteps()
{
  const name = this.window ? window.location.pathname : "Splendid Test";

  const objectStoreData = [
    { key: "1", value: "foo" },
    { key: "2", value: "bar" },
    { key: "3", value: "baz" }
  ];

  let request = indexedDB.open(name, 1);
  request.onerror = errorHandler;
  request.onupgradeneeded = grabEventAndContinueHandler;
  request.onsuccess = grabEventAndContinueHandler;
  let event = yield undefined; 

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
  event = yield undefined; 

  
  objectStore.createIndex("set", "", { unique: true });
  yield undefined; 

  let trans = db.transaction("data", "readwrite");
  objectStore = trans.objectStore("data");
  index = objectStore.index("set");

  request = index.get("bar");
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;
  
  event = yield undefined;

  is(event.target.result, "bar", "Got correct result");

  request = objectStore.add("foopy", 4);
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;

  yield undefined;

  request = index.get("foopy");
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;
  
  event = yield undefined;

  is(event.target.result, "foopy", "Got correct result");

  request = objectStore.add("foopy", 5);
  request.addEventListener("error", new ExpectError("ConstraintError", true));
  request.onsuccess = unexpectedSuccessHandler;

  trans.oncomplete = grabEventAndContinueHandler;

  yield undefined;
  yield undefined;

  finishTest();
  yield undefined;
}
