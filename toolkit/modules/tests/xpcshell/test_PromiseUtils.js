  


"use strict";

Components.utils.import("resource://gre/modules/PromiseUtils.jsm");
Components.utils.import("resource://gre/modules/Timer.jsm");


function run_test() {
  run_next_test();
}







add_task(function* test_resolve_string() {
  let def = PromiseUtils.defer();
  let expected = "The promise is resolved " + Math.random();
  def.resolve(expected);
  let result = yield def.promise;
  Assert.equal(result, expected, "def.resolve() resolves the promise");
});



add_task(function* test_resolve_undefined() {
  let def = PromiseUtils.defer();
  def.resolve();
  let result = yield def.promise;
  Assert.equal(result, undefined, "resolve works with undefined as well");
});



add_task(function* test_resolve_pending_promise() {
  let def = PromiseUtils.defer();
  let expected = 100 + Math.random();
  let p = new Promise((resolve, reject) => {
    setTimeout(() => resolve(expected), 100);
  });
  def.resolve(p);
  let result = yield def.promise;
  Assert.equal(result, expected, "def.promise assumed the state of the passed promise");
});



add_task(function* test_resolve_resolved_promise() {
  let def = PromiseUtils.defer();
  let expected = "Yeah resolved" + Math.random();
  let p = new Promise((resolve, reject) => resolve(expected));
  def.resolve(p);
  let result = yield def.promise;
  Assert.equal(result, expected, "Resolved promise is passed to the resolve method");
});



add_task(function* test_resolve_rejected_promise() {
  let def = PromiseUtils.defer();
  let p = new Promise((resolve, reject) => reject(new Error("There its an rejection")));
  def.resolve(p);
  yield Assert.rejects(def.promise, /There its an rejection/, "Settled rejection promise passed to the resolve method");
});



add_task(function* test_reject_Error() {
  let def = PromiseUtils.defer();
  def.reject(new Error("This one rejects"));
  yield Assert.rejects(def.promise, /This one rejects/, "reject method with Error for rejection");
});



add_task(function* test_reject_pending_promise() {
  let def = PromiseUtils.defer();
  let p = new Promise((resolve, reject) => {
    setTimeout(() => resolve(100), 100);
  });
  def.reject(p);
  yield Assert.rejects(def.promise, Promise, "Rejection with a pending promise uses the passed promise itself as the reason of rejection");
});



add_task(function* test_reject_resolved_promise() {
  let def = PromiseUtils.defer();
  let p = new Promise((resolve, reject) => resolve("This resolved"));
  def.reject(p);
  yield Assert.rejects(def.promise, Promise, "Rejection with a resolved promise uses the passed promise itself as the reason of rejection");
});



add_task(function* test_reject_resolved_promise() {
  let def = PromiseUtils.defer();
  let p = new Promise((resolve, reject) => reject(new Error("This on rejects")));
  def.reject(p);
  yield Assert.rejects(def.promise, Promise, "Rejection with a rejected promise uses the passed promise itself as the reason of rejection");
});