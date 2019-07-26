






"use strict";

const testFile = gProfD.clone();
testFile.append("DeferredSaveTest");

Components.utils.import("resource://gre/modules/Promise.jsm");

let context = Components.utils.import("resource://gre/modules/DeferredSave.jsm", {});
let DeferredSave = context.DeferredSave;


function DeferredSaveTester(aDelay, aDataProvider) {
  let tester = {
    
    waDeferred: null,

    
    writtenData: undefined,

    dataToSave: "Data to save",

    save: (aData, aWriteHandler) => {
      tester.writeHandler = aWriteHandler || writer;
      tester.dataToSave = aData;
      return tester.saver.saveChanges();
    },

    flush: (aWriteHandler) => {
      tester.writeHandler = aWriteHandler || writer;
      return tester.saver.flush();
    },

    get error() {
      return tester.saver.error;
    }
  };

  
  
  
  function writer(aTester) {
    do_print("default write callback");
    let length = aTester.writtenData.length;
    do_execute_soon(() => aTester.waDeferred.resolve(length));
  }

  if (!aDataProvider)
    aDataProvider = () => tester.dataToSave;

  tester.saver = new DeferredSave(testFile.path, aDataProvider, aDelay);

  
  
  context.OS.File.writeAtomic = function mock_writeAtomic(aFile, aData, aOptions) {
      do_print("writeAtomic: " + aFile + " data: '" + aData + "', " + aOptions.toSource());
      tester.writtenData = aData;
      tester.waDeferred = Promise.defer();
      tester.writeHandler(tester);
      return tester.waDeferred.promise;
    };

  return tester;
};




function delay(aDelayMS) {
  let deferred = Promise.defer();
  do_timeout(aDelayMS, () => deferred.resolve(null));
  return deferred.promise;
}

function run_test() {
  run_next_test();
}


add_task(function test_basic_save_succeeds() {
  let tester = DeferredSaveTester(1);
  let data = "Test 1 Data";

  yield tester.save(data);
  do_check_eq(tester.writtenData, data);
  do_check_eq(1, tester.saver.totalSaves);
});



add_task(function test_two_saves() {
  let tester = DeferredSaveTester(1);
  let firstCallback_happened = false;
  let firstData = "Test first save";
  let secondData = "Test second save";

  
  
  tester.save(firstData).then(count => {
    do_check_eq(secondData, tester.writtenData);
    do_check_false(firstCallback_happened);
    firstCallback_happened = true;
  }, do_report_unexpected_exception);

  yield tester.save(secondData);
  do_check_true(firstCallback_happened);
  do_check_eq(secondData, tester.writtenData);
  do_check_eq(1, tester.saver.totalSaves);
});



add_task(function test_two_saves_delay() {
  let tester = DeferredSaveTester(50);
  let firstCallback_happened = false;
  let delayDone = false;

  let firstData = "First data to save with delay";
  let secondData = "Modified data to save with delay";

  tester.save(firstData).then(count => {
    do_check_false(firstCallback_happened);
    do_check_true(delayDone);
    do_check_eq(secondData, tester.writtenData);
    firstCallback_happened = true;
  }, do_report_unexpected_exception);

  yield delay(5);
  delayDone = true;
  yield tester.save(secondData);
  do_check_true(firstCallback_happened);
  do_check_eq(secondData, tester.writtenData);
  do_check_eq(1, tester.saver.totalSaves);
  do_check_eq(0, tester.saver.overlappedSaves);
});




add_task(function test_error_immediate() {
  let tester = DeferredSaveTester(1);
  let testError = new Error("Forced failure");
  function writeFail(aTester) {
    aTester.waDeferred.reject(testError);
  }

  yield tester.save("test_error_immediate", writeFail).then(
    count => do_throw("Did not get expected error"),
    error => do_check_eq(testError.message, error.message)
    );
  do_check_eq(testError, tester.error);

  
  yield tester.save("test_error_immediate succeeds");
  do_check_eq(null, tester.error);
  
  do_check_eq(2, tester.saver.totalSaves);
});




add_task(function dirty_while_writing() {
  let tester = DeferredSaveTester(1);
  let firstData = "First data";
  let secondData = "Second data";
  let thirdData = "Third data";
  let firstCallback_happened = false;
  let secondCallback_happened = false;
  let writeStarted = Promise.defer();

  function writeCallback(aTester) {
    writeStarted.resolve(aTester.waDeferred);
  }

  do_print("First save");
  tester.save(firstData, writeCallback).then(
    count => {
      do_check_false(firstCallback_happened);
      do_check_false(secondCallback_happened);
      do_check_eq(tester.writtenData, firstData);
      firstCallback_happened = true;
    }, do_report_unexpected_exception);

  do_print("waiting for writer");
  let writer = yield writeStarted.promise;
  do_print("Write started");

  
  
  
  yield delay(1);

  tester.save(secondData).then(
    count => {
      do_check_true(firstCallback_happened);
      do_check_false(secondCallback_happened);
      do_check_eq(tester.writtenData, thirdData);
      secondCallback_happened = true;
    }, do_report_unexpected_exception);

  
  yield delay(1);
  let thirdWrite = tester.save(thirdData);

  
  yield delay(1);
  writer.resolve(firstData.length);

  
  yield thirdWrite;
  do_check_true(firstCallback_happened);
  do_check_true(secondCallback_happened);
  do_check_eq(tester.writtenData, thirdData);
  do_check_eq(2, tester.saver.totalSaves);
  do_check_eq(1, tester.saver.overlappedSaves);
});


function disabled_write_callback(aTester) {
  do_throw("Should not have written during clean flush");
  deferred.reject(new Error("Write during supposedly clean flush"));
}



function write_then_disable(aTester) {
  do_print("write_then_disable");
  let length = aTester.writtenData.length;
  aTester.writeHandler = disabled_write_callback;
  do_execute_soon(() => aTester.waDeferred.resolve(length));
}



add_task(function flush_after_save() {
  let tester = DeferredSaveTester(1);
  let dataToSave = "Flush after save";

  yield tester.save(dataToSave);
  yield tester.flush(disabled_write_callback);
  do_check_eq(1, tester.saver.totalSaves);
});


add_task(function flush_during_write() {
  let tester = DeferredSaveTester(1);
  let dataToSave = "Flush during write";
  let firstCallback_happened = false;
  let writeStarted = Promise.defer();

  function writeCallback(aTester) {
    writeStarted.resolve(aTester.waDeferred);
  }

  tester.save(dataToSave, writeCallback).then(
    count => {
      do_check_false(firstCallback_happened);
      firstCallback_happened = true;
    }, do_report_unexpected_exception);

  let writer = yield writeStarted.promise;

  
  let flushing = tester.flush(disabled_write_callback);
  yield delay(2);
  writer.resolve(dataToSave.length);

  
  yield flushing;
  do_check_true(firstCallback_happened);
  do_check_eq(1, tester.saver.totalSaves);
});









add_task(function flush_while_dirty() {
  let tester = DeferredSaveTester(20);
  let firstData = "Flush while dirty, valid data";
  let firstCallback_happened = false;

  tester.save(firstData, write_then_disable).then(
    count => {
      do_check_false(firstCallback_happened);
      firstCallback_happened = true;
      do_check_eq(tester.writtenData, firstData);
    }, do_report_unexpected_exception);

  
  
  let flushing = tester.flush();
  tester.dataToSave = "Flush while dirty, invalid data";
  yield flushing;
  do_check_true(firstCallback_happened);
  do_check_eq(tester.writtenData, firstData);
  do_check_eq(1, tester.saver.totalSaves);
});






add_task(function flush_writing_dirty() {
  let tester = DeferredSaveTester(5);
  let firstData = "Flush first pass data";
  let secondData = "Flush second pass data";
  let firstCallback_happened = false;
  let secondCallback_happened = false;
  let writeStarted = Promise.defer();

  function writeCallback(aTester) {
    writeStarted.resolve(aTester.waDeferred);
  }

  tester.save(firstData, writeCallback).then(
    count => {
      do_check_false(firstCallback_happened);
      do_check_eq(tester.writtenData, firstData);
      firstCallback_happened = true;
    }, do_report_unexpected_exception);

  let writer = yield writeStarted.promise;
  

  
  
  tester.save(secondData, write_then_disable).then(
    count => {
      do_check_true(firstCallback_happened);
      do_check_false(secondCallback_happened);
      do_check_eq(tester.writtenData, secondData);
      secondCallback_happened = true;
    }, do_report_unexpected_exception);

  let flushing = tester.flush(write_then_disable);
  tester.dataToSave = "Flush, invalid data: changed late";
  yield delay(1);
  
  writer.resolve(firstData.length);
  
  yield flushing;
  do_check_true(firstCallback_happened);
  do_check_true(secondCallback_happened);
  do_check_eq(tester.writtenData, secondData);
  do_check_eq(2, tester.saver.totalSaves);
  do_check_eq(1, tester.saver.overlappedSaves);
});






const expectedDataError = "Failed to serialize data";
let badDataError = null;
function badDataProvider() {
  let err = new Error(badDataError);
  badDataError = "badDataProvider called twice";
  throw err;
}



add_task(function data_throw() {
  badDataError = expectedDataError;
  let tester = DeferredSaveTester(1, badDataProvider);
  yield tester.save("data_throw").then(
    count => do_throw("Expected serialization failure"),
    error => do_check_eq(error.message, expectedDataError));
});


add_task(function data_throw_during_flush() {
  badDataError = expectedDataError;
  let tester = DeferredSaveTester(1, badDataProvider);
  let firstCallback_happened = false;

  
  tester.save("data_throw_during_flush", disabled_write_callback).then(
    count => do_throw("Expected serialization failure"),
    error => {
      do_check_false(firstCallback_happened);
      do_check_eq(error.message, expectedDataError);
      firstCallback_happened = true;
    });

  yield tester.flush(disabled_write_callback).then(
    count => do_throw("Expected serialization failure"),
    error => do_check_eq(error.message, expectedDataError)
    );

  do_check_true(firstCallback_happened);
});












add_task(function delay_flush_race() {
  let tester = DeferredSaveTester(5);
  let firstData = "First save";
  let secondData = "Second save";
  let thirdData = "Third save";
  let writeStarted = Promise.defer();

  function writeCallback(aTester) {
    writeStarted.resolve(aTester.waDeferred);
  }

  
  let firstSave = tester.save(firstData, writeCallback);

  let writer = yield writeStarted.promise;
  

  
  let secondSave = tester.save(secondData);

  
  writer.resolve(firstData.length);
  yield firstSave;
  do_check_eq(tester.writtenData, firstData);

  tester.save(thirdData);
  let flushing = tester.flush();

  yield secondSave;
  do_check_eq(tester.writtenData, thirdData);

  yield flushing;
  do_check_eq(tester.writtenData, thirdData);

  
  
  do_check_eq(null, tester.saver._timer);
});
