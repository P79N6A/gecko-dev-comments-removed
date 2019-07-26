




var testGenerator = testSteps();

function testSteps()
{
  const data = { id: new Date().getTime(),
                 num: parseInt(Math.random() * 1000) };

  let request = indexedDB.open(this.window ? window.location.pathname : "Splendid Test", 1);
  request.onerror = errorHandler;
  request.onupgradeneeded = grabEventAndContinueHandler;
  let event = yield undefined;

  let db = event.target.result;
  db.onerror = errorHandler;

  event.target.onsuccess = continueToNextStep;

  
  let objectStore = db.createObjectStore("foo", { keyPath: "id" });
  objectStore.add(data);
  yield undefined;
  db.close();

  let request = indexedDB.open(this.window ? window.location.pathname : "Splendid Test", 2);
  request.onerror = errorHandler;
  request.onupgradeneeded = grabEventAndContinueHandler;
  let event = yield undefined;

  let db2 = event.target.result;
  db2.onerror = errorHandler;

  event.target.onsuccess = continueToNextStep;

  
  event.target.transaction.objectStore("foo").createIndex("foo", "num");
  yield undefined;

  
  let seenCount = 0;


  db2.transaction("foo").objectStore("foo").index("foo")
     .openKeyCursor().onsuccess = function(event) {
    let cursor = event.target.result;
    if (cursor) {
      is(cursor.key, data.num, "Good key");
      is(cursor.primaryKey, data.id, "Good value");
      seenCount++;
      cursor.continue();
    }
    else {
      continueToNextStep();
    }
  };
  yield undefined;

  is(seenCount, 1, "Saw our entry");

  finishTest();
  yield undefined;
}
