


"use strict";





let passedTests = 0;

function rejectWithTimeout(error = undefined) {
  let deferred = Promise.defer();
  executeSoon(function() {
    ok(true, "we get here after a timeout");
    deferred.reject(error);
  });
  return deferred.promise;
}

add_task(function failWithoutError() {
  try {
    yield rejectWithTimeout();
  } finally {
    ++passedTests;
  }
});

add_task(function failWithString() {
  try {
    yield rejectWithTimeout("Meaningless error");
  } finally {
    ++passedTests;
  }
});

add_task(function failWithoutInt() {
  try {
    yield rejectWithTimeout(42);
  } finally {
    ++passedTests;
  }
});



add_task(function failWithError() {
  try {
    yield rejectWithTimeout(new Error("This is an error"));
  } finally {
    ++passedTests;
  }
});

add_task(function done() {
  is(passedTests, 4, "Passed all tests");
});
