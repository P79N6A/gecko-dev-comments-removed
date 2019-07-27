




var testGenerator = testSteps();

function testSteps()
{
  const name = this.window ? window.location.pathname : "Splendid Test";
  const START_DATA = "hi";
  const END_DATA = "bye";
  const objectStoreInfo = [
    { name: "1", options: { keyPath: null }, key: 1,
      entry: { data: START_DATA } },
    { name: "2", options: { keyPath: "foo" },
      entry: { foo: 1, data: START_DATA } },
    { name: "3", options: { keyPath: null, autoIncrement: true },
      entry: { data: START_DATA } },
    { name: "4", options: { keyPath: "foo", autoIncrement: true },
      entry: { data: START_DATA } },
  ];

  for (let i = 0; i < objectStoreInfo.length; i++) {
    
    let info = objectStoreInfo[i];

    ok(true, "1");
    request = indexedDB.open(name, i + 1);
    request.onerror = errorHandler;
    request.onupgradeneeded = grabEventAndContinueHandler;
    request.onsuccess = grabEventAndContinueHandler;
    event = yield undefined;

    let db = event.target.result;

    ok(true, "2");
    let objectStore = info.hasOwnProperty("options") ?
                      db.createObjectStore(info.name, info.options) :
                      db.createObjectStore(info.name);

    
    let index = objectStore.createIndex("data_index", "data",
                                        { unique: false });
    let uniqueIndex = objectStore.createIndex("unique_data_index", "data",
                                              { unique: true });
    
    request = info.hasOwnProperty("key") ?
              objectStore.add(info.entry, info.key) :
              objectStore.add(info.entry);
    request.onerror = errorHandler;
    request.onsuccess = grabEventAndContinueHandler;
    event = yield undefined;
    ok(true, "3");

    
    request = objectStore.openCursor();
    request.onerror = errorHandler;
    request.onsuccess = grabEventAndContinueHandler;
    event = yield undefined;
    ok(true, "4");

    let cursor = request.result;
    let obj = cursor.value;
    obj.data = END_DATA;
    request = cursor.update(obj);
    request.onerror = errorHandler;
    request.onsuccess = grabEventAndContinueHandler;
    event = yield undefined;
    ok(true, "5");

    
    request = index.get(END_DATA);
    request.onerror = errorHandler;
    request.onsuccess = grabEventAndContinueHandler;
    event = yield undefined;
    ok(true, "6");
    ok(obj.data, event.target.result.data,
                  "Non-unique index was properly updated.");

    request = uniqueIndex.get(END_DATA);
    request.onerror = errorHandler;
    request.onsuccess = grabEventAndContinueHandler;
    event = yield undefined;

    ok(true, "7");
    ok(obj.data, event.target.result.data,
                  "Unique index was properly updated.");

    
    yield undefined;

    db.close();
  }

  finishTest();
  yield undefined;
}

