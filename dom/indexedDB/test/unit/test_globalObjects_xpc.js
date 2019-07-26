




var testGenerator = testSteps();

function testSteps()
{
  const name = "Splendid Test";

  
  let keyRange = IDBKeyRange.only(42);
  ok(keyRange, "Got keyRange");

  let request = indexedDB.open(name, 1);
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;
  let event = yield undefined;

  let db = event.target.result;
  ok(db, "Got database");

  finishTest();
  yield undefined;
}
