  


"use strict";

Components.utils.import("resource://gre/modules/PromiseUtils.jsm");
Components.utils.import("resource://gre/modules/Timer.jsm");


function run_test() {
  run_next_test();
}



add_task(function* test_wrong_arguments() {
  let p = new Promise((resolve, reject) => {});
  
  Assert.throws(() => PromiseUtils.resolveOrTimeout("string", 200), /first argument <promise> must be a Promise object/,
                "TypeError thrown because first argument is not a Promsie object");
  
  Assert.throws(() => PromiseUtils.resolveOrTimeout(p, "string"), /second argument <delay> must be a positive number/,
                "TypeError thrown because second argument is not a positive number");
  
  Assert.throws(() => PromiseUtils.resolveOrTimeout(p, 200, "string"), /third optional argument <rejection> must be a function/,
                "TypeError thrown because thrird argument is not a function");
});



add_task(function* test_optional_third_argument() {
  let p = new Promise((resolve, reject) => {});
  yield Assert.rejects(PromiseUtils.resolveOrTimeout(p, 200), /Promise Timeout/, "Promise rejects with a default Error");
});



add_task(function* test_resolve_quickly() {
  let p = new Promise((resolve, reject) => setTimeout(() => resolve("Promise is resolved"), 20));
  let result = yield PromiseUtils.resolveOrTimeout(p, 200);
  Assert.equal(result, "Promise is resolved", "Promise resolves quickly");
});



add_task(function* test_reject_quickly() {
  let p = new Promise((resolve, reject) => setTimeout(() => reject("Promise is rejected"), 20));
  yield Assert.rejects(PromiseUtils.resolveOrTimeout(p, 200), /Promise is rejected/, "Promise rejects quickly");
});



add_task(function* test_rejection_function() {
  let p = new Promise((resolve, reject) => {});
  
  yield Assert.rejects(PromiseUtils.resolveOrTimeout(p, 200, () => {
    return "Rejection returned a string";
  }), /Rejection returned a string/, "Rejection returned a string");

  
  yield Assert.rejects(PromiseUtils.resolveOrTimeout(p, 200, () => {
    return {Name:"Promise"};
  }), Object, "Rejection returned an object");

  
  yield Assert.rejects(PromiseUtils.resolveOrTimeout(p, 200, () => {
    return;
  }), undefined, "Rejection returned undefined");
});



add_task(function* test_rejection_throw_error() {
  let p = new Promise((resolve, reject) => {});
  yield Assert.rejects(PromiseUtils.resolveOrTimeout(p, 200, () => {
    throw new Error("Rejection threw an Error");
  }), /Rejection threw an Error/, "Rejection threw an error");
});