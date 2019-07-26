




var testGenerator = testSteps();

function testSteps()
{
  const name = this.window ? window.location.pathname : "Splendid Test";

  let request = indexedDB.open(name, 1);
  request.onerror = errorHandler;
  request.onsuccess = grabEventAndContinueHandler;
  let event = yield undefined;

  let db = event.target.result;

  
  is(db.version, 1, "Correct default version for a new database.");

  const versions = [
    7,
    42,
  ];

  db.close();

  for (let i = 0; i < versions.length; i++) {
    let version = versions[i];

    let request = indexedDB.open(name, version);
    request.onerror = errorHandler;
    request.onupgradeneeded = grabEventAndContinueHandler;
    let event = yield undefined;

    let db = event.target.result;

    is(db.version, version, "Database version number updated correctly");
    is(event.target.transaction.mode, "versionchange", "Correct mode");

    executeSoon(function() { testGenerator.next(); });
    yield undefined;
    db.close();
  }

  finishTest();
  yield undefined;
}

