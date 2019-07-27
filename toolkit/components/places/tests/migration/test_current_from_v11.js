


add_task(function* setup() {
  yield setupPlacesDatabase("places_v11.sqlite");
});

add_task(function* database_is_valid() {
  Assert.equal(PlacesUtils.history.databaseStatus,
               PlacesUtils.history.DATABASE_STATUS_UPGRADED);

  let db = yield PlacesUtils.promiseDBConnection();
  Assert.equal((yield db.getSchemaVersion()), CURRENT_SCHEMA_VERSION);
});

add_task(function* test_moz_hosts() {
  let db = yield PlacesUtils.promiseDBConnection();

  
  yield db.execute("SELECT host, frecency, typed, prefix FROM moz_hosts");

  
  yield PlacesTestUtils.promiseAsyncUpdates();

  
  
  let rows = yield db.execute(
    `SELECT (SELECT COUNT(host) FROM moz_hosts),
            (SELECT COUNT(DISTINCT rev_host)
             FROM moz_places
             WHERE LENGTH(rev_host) > 1)
    `);

  Assert.equal(rows.length, 1);
  let mozHostsCount = rows[0].getResultByIndex(0);
  let mozPlacesCount = rows[0].getResultByIndex(1);

  Assert.ok(mozPlacesCount > 0, "There is some url in the database");
  Assert.equal(mozPlacesCount, mozHostsCount, "moz_hosts has the expected number of entries");
});

add_task(function* test_journal() {
  let db = yield PlacesUtils.promiseDBConnection();
  let rows = yield db.execute("PRAGMA journal_mode");
  Assert.equal(rows.length, 1);
  
  Assert.equal(rows[0].getResultByIndex(0), "wal");
});
