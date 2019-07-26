



var testnum = 0;
let dbConnection; 

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");

function countDeletedEntries(expected)
{
  let deferred = Promise.defer();
  let stmt = dbConnection.createAsyncStatement("SELECT COUNT(*) AS numEntries FROM moz_deleted_formhistory");
  stmt.executeAsync({
    handleResult: function(resultSet) {
      do_check_eq(expected, resultSet.getNextRow().getResultByName("numEntries"));
      deferred.resolve();
    },
    handleError : function () {
      do_throw("Error occurred counting deleted entries: " + error);
      deferred.reject();
    },
    handleCompletion : function () {
      stmt.finalize();
    }
  });
  return deferred.promise;
}

function checkTimeDeleted(guid, checkFunction)
{
  let deferred = Promise.defer();
  let stmt = dbConnection.createAsyncStatement("SELECT timeDeleted FROM moz_deleted_formhistory WHERE guid = :guid");
  stmt.params.guid = guid;
  stmt.executeAsync({
    handleResult: function(resultSet) {
      checkFunction(resultSet.getNextRow().getResultByName("timeDeleted"));
      deferred.resolve();
    },
    handleError : function () {
      do_throw("Error occurred getting deleted entries: " + error);
      deferred.reject();
    },
    handleCompletion : function () {
      stmt.finalize();
    }
  });
  return deferred.promise;
}

function promiseUpdateEntry(op, name, value)
{
  var change = { op: op };
  if (name !== null)
    change.fieldname = name;
  if (value !== null)
    change.value = value;
  return promiseUpdate(change);
}

function promiseUpdate(change)
{
  let deferred = Promise.defer();
  FormHistory.update(change,
                     { handleError: function (error) {
                         do_throw("Error occurred updating form history: " + error);
                         deferred.reject(error);
                       },
                       handleCompletion: function (reason) { if (!reason) deferred.resolve(); }
                     });
  return deferred.promise;
}

function promiseSearchEntries(terms, params)
{
  let deferred = Promise.defer();
  let results = [];
  FormHistory.search(terms, params,
                     { handleResult: function(result) results.push(result),
                       handleError: function (error) {
                         do_throw("Error occurred searching form history: " + error);
                         deferred.reject(error);
                       },
                       handleCompletion: function (reason) { if (!reason) deferred.resolve(results); }
                     });
  return deferred.promise;
}

function promiseCountEntries(name, value, checkFn)
{
  let deferred = Promise.defer();
  countEntries(name, value, function (result) { checkFn(result); deferred.resolve(); } );
  return deferred.promise;
}

add_task(function ()
{
  let oldSupportsDeletedTable = FormHistory._supportsDeletedTable;
  FormHistory._supportsDeletedTable = true;

  try {

  
  var testfile = do_get_file("formhistory_apitest.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");

  function checkExists(num) { do_check_true(num > 0); }
  function checkNotExists(num) { do_check_true(num == 0); }

  
  
  testnum++;
  yield promiseCountEntries("name-A", null, checkExists);
  yield promiseCountEntries("name-B", null, checkExists);
  yield promiseCountEntries("name-C", null, checkExists);
  yield promiseCountEntries("name-D", null, checkExists);
  yield promiseCountEntries("name-A", "value-A", checkExists);
  yield promiseCountEntries("name-B", "value-B1", checkExists);
  yield promiseCountEntries("name-B", "value-B2", checkExists);
  yield promiseCountEntries("name-C", "value-C", checkExists);
  yield promiseCountEntries("name-D", "value-D", checkExists);
  

  
  let dbFile = Services.dirsvc.get("ProfD", Ci.nsIFile).clone();
  dbFile.append("formhistory.sqlite");
  dbConnection = Services.storage.openUnsharedDatabase(dbFile);

  let deferred = Promise.defer();

  let stmt = dbConnection.createAsyncStatement("DELETE FROM moz_deleted_formhistory");
  stmt.executeAsync({
    handleResult: function(resultSet) { },
    handleError : function () {
      do_throw("Error occurred counting deleted all entries: " + error);
    },
    handleCompletion : function () {
      stmt.finalize();
      deferred.resolve();
    }
  });
  yield deferred.promise;

  
  
  testnum++;
  yield promiseCountEntries("blah", null, checkNotExists);
  yield promiseCountEntries("", null, checkNotExists);
  yield promiseCountEntries("name-A", "blah", checkNotExists);
  yield promiseCountEntries("name-A", "", checkNotExists);
  yield promiseCountEntries("name-A", null, checkExists);
  yield promiseCountEntries("blah", "value-A", checkNotExists);
  yield promiseCountEntries("", "value-A", checkNotExists);
  yield promiseCountEntries(null, "value-A", checkExists);

  
  
  deferred = Promise.defer();
  yield FormHistory.count({ fieldname: null, value: null },
                          { handleResult: function(result) checkNotExists(result),
                            handleError: function (error) {
                              do_throw("Error occurred searching form history: " + error);
                            },
                            handleCompletion: function(reason) { if (!reason) deferred.resolve() }
                          });
  yield deferred.promise;

  
  
  testnum++;
  yield promiseUpdateEntry("remove", "name-A", null);

  yield promiseCountEntries("name-A", "value-A", checkNotExists);
  yield promiseCountEntries("name-B", "value-B1", checkExists);
  yield promiseCountEntries("name-B", "value-B2", checkExists);
  yield promiseCountEntries("name-C", "value-C", checkExists);
  yield promiseCountEntries("name-D", "value-D", checkExists);
  yield countDeletedEntries(1);

  
  
  testnum++;
  yield promiseUpdateEntry("remove", "name-B", null);

  yield promiseCountEntries("name-A", "value-A", checkNotExists);
  yield promiseCountEntries("name-B", "value-B1", checkNotExists);
  yield promiseCountEntries("name-B", "value-B2", checkNotExists);
  yield promiseCountEntries("name-C", "value-C", checkExists);
  yield promiseCountEntries("name-D", "value-D", checkExists);
  yield countDeletedEntries(3);

  
  
  testnum++;
  yield promiseCountEntries("time-A", null, checkExists); 
  yield promiseCountEntries("time-B", null, checkExists); 
  yield promiseCountEntries("time-C", null, checkExists); 
  yield promiseCountEntries("time-D", null, checkExists); 
  yield promiseUpdate({ op : "remove", firstUsedStart: 1050, firstUsedEnd: 2000 });

  yield promiseCountEntries("time-A", null, checkExists);
  yield promiseCountEntries("time-B", null, checkExists);
  yield promiseCountEntries("time-C", null, checkNotExists);
  yield promiseCountEntries("time-D", null, checkExists);
  yield countDeletedEntries(4);

  
  
  testnum++;
  yield promiseUpdate({ op : "remove", firstUsedStart: 1000, firstUsedEnd: 2000 });

  yield promiseCountEntries("time-A", null, checkNotExists);
  yield promiseCountEntries("time-B", null, checkNotExists);
  yield promiseCountEntries("time-C", null, checkNotExists);
  yield promiseCountEntries("time-D", null, checkExists);
  yield countDeletedEntries(6);

  
  
  testnum++;
  yield promiseUpdateEntry("remove", null, null);

  yield promiseCountEntries("name-C", null, checkNotExists);
  yield promiseCountEntries("name-D", null, checkNotExists);
  yield promiseCountEntries("name-C", "value-C", checkNotExists);
  yield promiseCountEntries("name-D", "value-D", checkNotExists);

  yield promiseCountEntries(null, null, checkNotExists);
  yield countDeletedEntries(6);

  
  
  testnum++;
  yield promiseUpdateEntry("add", "newname-A", "newvalue-A");
  yield promiseCountEntries("newname-A", "newvalue-A", checkExists);

  
  
  testnum++;
  yield promiseUpdateEntry("remove", "newname-A", "newvalue-A");
  yield promiseCountEntries("newname-A", "newvalue-A", checkNotExists);

  
  
  testnum++;
  yield promiseUpdateEntry("add", "field1", "value1");
  yield promiseCountEntries("field1", "value1", checkExists);

  let processFirstResult = function processResults(results)
  {
    
    if (results.length > 0) {
      let result = results[0];
      return [result.timesUsed, result.firstUsed, result.lastUsed, result.guid];
    }
  }

  results = yield promiseSearchEntries(["timesUsed", "firstUsed", "lastUsed"],
                                       { fieldname: "field1", value: "value1" });
  let [timesUsed, firstUsed, lastUsed] = processFirstResult(results);
  do_check_eq(1, timesUsed);
  do_check_true(firstUsed > 0);
  do_check_true(lastUsed > 0);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 1));

  
  
  testnum++;
  yield promiseUpdateEntry("add", "field1", "value1b");
  yield promiseCountEntries("field1", "value1", checkExists);
  yield promiseCountEntries("field1", "value1b", checkExists);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 2));

  
  
  testnum++;

  results = yield promiseSearchEntries(["guid"], { fieldname: "field1", value: "value1" });
  let guid = processFirstResult(results)[3];

  yield promiseUpdate({ op : "update", guid: guid, value: "modifiedValue" });
  yield promiseCountEntries("field1", "modifiedValue", checkExists);
  yield promiseCountEntries("field1", "value1", checkNotExists);
  yield promiseCountEntries("field1", "value1b", checkExists);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 2));

  
  
  testnum++;
  yield promiseUpdate({ op : "add", fieldname: "field2", value: "value2",
                        timesUsed: 20, firstUsed: 100, lastUsed: 500 });

  results = yield promiseSearchEntries(["timesUsed", "firstUsed", "lastUsed"],
                                       { fieldname: "field2", value: "value2" });
  [timesUsed, firstUsed, lastUsed] = processFirstResult(results);

  do_check_eq(20, timesUsed);
  do_check_eq(100, firstUsed);
  do_check_eq(500, lastUsed);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 3));

  
  
  testnum++;
  yield promiseUpdate({ op : "bump", fieldname: "field2", value: "value2",
                        timesUsed: 20, firstUsed: 100, lastUsed: 500 });
  results = yield promiseSearchEntries(["timesUsed", "firstUsed", "lastUsed"],
                                       { fieldname: "field2", value: "value2" });
  [timesUsed, firstUsed, lastUsed] = processFirstResult(results);
  do_check_eq(21, timesUsed);
  do_check_eq(100, firstUsed);
  do_check_true(lastUsed > 500);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 3));

  
  
  testnum++;
  yield promiseUpdate({ op : "bump", fieldname: "field3", value: "value3",
                        timesUsed: 10, firstUsed: 50, lastUsed: 400 });
  results = yield promiseSearchEntries(["timesUsed", "firstUsed", "lastUsed"],
                                       { fieldname: "field3", value: "value3" });
  [timesUsed, firstUsed, lastUsed] = processFirstResult(results);
  do_check_eq(10, timesUsed);
  do_check_eq(50, firstUsed);
  do_check_eq(400, lastUsed);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 4));

  
  
  testnum++;
  results = yield promiseSearchEntries(["guid"], { fieldname: "field3", value: "value3" });
  guid = processFirstResult(results)[3];
  yield promiseUpdate({ op : "bump", guid: guid, timesUsed: 20, firstUsed: 55, lastUsed: 400 });
  results = yield promiseSearchEntries(["timesUsed", "firstUsed", "lastUsed"],
                                       { fieldname: "field3", value: "value3" });
  [timesUsed, firstUsed, lastUsed] = processFirstResult(results);
  do_check_eq(11, timesUsed);
  do_check_eq(50, firstUsed);
  do_check_true(lastUsed > 400);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 4));

  
  
  testnum++;
  yield countDeletedEntries(7);

  results = yield promiseSearchEntries(["guid"], { fieldname: "field1", value: "value1b" });
  guid = processFirstResult(results)[3];

  yield promiseUpdate({ op : "remove", guid: guid});
  yield promiseCountEntries("field1", "modifiedValue", checkExists);
  yield promiseCountEntries("field1", "value1b", checkNotExists);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 3));

  yield countDeletedEntries(8);
  yield checkTimeDeleted(guid, function (timeDeleted) do_check_true(timeDeleted > 10000));

  
  
  testnum++;
  yield promiseUpdate({ op : "add", fieldname: "field4", value: "value4",
                        timesUsed: 5, firstUsed: 230, lastUsed: 600 });
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 4));

  
  
  testnum++;
  results = yield promiseSearchEntries(["timesUsed", "firstUsed", "lastUsed"],
                                       { fieldname: "field1", value: "modifiedValue" });
  [timesUsed, firstUsed, lastUsed] = processFirstResult(results);

  yield promiseUpdate({ op : "remove", firstUsedStart: 60, firstUsedEnd: 250 });
  yield promiseCountEntries("field1", "modifiedValue", checkExists);
  yield promiseCountEntries("field2", "value2", checkNotExists);
  yield promiseCountEntries("field3", "value3", checkExists);
  yield promiseCountEntries("field4", "value4", checkNotExists);
  yield promiseCountEntries(null, null, function(num) do_check_eq(num, 2));
  yield countDeletedEntries(10);

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
  finally {
    FormHistory._supportsDeletedTable = oldSupportsDeletedTable;
    dbConnection.asyncClose(do_test_finished);
  }
});

function run_test() run_next_test();
