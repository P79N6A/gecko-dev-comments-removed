



"use strict";

Cu.import("resource://gre/modules/Timer.jsm", this);

add_task(function* test_globals() {
  Assert.equal(Promise.defer || undefined, undefined, "We are testing DOM Promise.");
  Assert.notEqual(PromiseDebugging, undefined, "PromiseDebugging is available.");
});

add_task(function* test_promiseID() {
  let p1 = new Promise(resolve => {});
  let p2 = new Promise(resolve => {});
  let p3 = p2.then(null, null);
  let promise = [p1, p2, p3];

  let identifiers = promise.map(PromiseDebugging.getPromiseID);
  info("Identifiers: " + JSON.stringify(identifiers));
  let idSet = new Set(identifiers);
  Assert.equal(idSet.size, identifiers.length,
    "PromiseDebugging.getPromiseID returns a distinct id per promise");

  let identifiers2 = promise.map(PromiseDebugging.getPromiseID);
  Assert.equal(JSON.stringify(identifiers),
               JSON.stringify(identifiers2),
               "Successive calls to PromiseDebugging.getPromiseID return the same id for the same promise");
});

add_task(function* test_observe_uncaught() {
  
  let names = new Map();

  
  let CallbackResults = function(name) {
    this.name = name;
    this.expected = new Set();
    this.observed = new Set();
    this.blocker = new Promise(resolve => this.resolve = resolve);
  };
  CallbackResults.prototype = {
    observe: function(promise) {
      info(this.name + " observing Promise " + names.get(promise));
      Assert.equal(PromiseDebugging.getState(promise).state, "rejected",
                   this.name + " observed a rejected Promise");
      if (!this.expected.has(promise)) {
        Assert.ok(false,
            this.name + " observed a Promise that it expected to observe, " +
            names.get(promise) +
            " (" + PromiseDebugging.getPromiseID(promise) +
            ", " + PromiseDebugging.getAllocationStack(promise) + ")");

      }
      Assert.ok(this.expected.delete(promise),
                this.name + " observed a Promise that it expected to observe, " +
                names.get(promise)  + " (" + PromiseDebugging.getPromiseID(promise) + ")");
      Assert.ok(!this.observed.has(promise),
                this.name + " observed a Promise that it has not observed yet");
      this.observed.add(promise);
      if (this.expected.size == 0) {
        this.resolve();
      } else {
        info(this.name + " is still waiting for " + this.expected.size + " observations:");
        info(JSON.stringify([names.get(x) for (x of this.expected.values())]));
      }
    },
  };

  let onLeftUncaught = new CallbackResults("onLeftUncaught");
  let onConsumed = new CallbackResults("onConsumed");

  let observer = {
    onLeftUncaught: function(promise, data) {
      onLeftUncaught.observe(promise);
    },
    onConsumed: function(promise) {
      onConsumed.observe(promise);
    },
  };

  let resolveLater = function(delay = 20) {
    return new Promise((resolve, reject) => setTimeout(resolve, delay));
  };
  let rejectLater = function(delay = 20) {
    return new Promise((resolve, reject) => setTimeout(reject, delay));
  };
  let makeSamples = function*() {
    yield {
      promise: Promise.resolve(0),
      name: "Promise.resolve",
    };
    yield {
      promise: Promise.resolve(resolve => resolve(0)),
      name: "Resolution callback",
    };
    yield {
      promise: Promise.resolve(0).then(null, null),
      name: "`then(null, null)`"
    };
    yield {
      promise: Promise.reject(0).then(null, () => {}),
      name: "Reject and catch immediately",
    };
    yield {
      promise: resolveLater(),
      name: "Resolve later",
    };
    yield {
      promise: Promise.reject("Simple rejection"),
      leftUncaught: true,
      consumed: false,
      name: "Promise.reject",
    };

    
    let p = Promise.reject("Reject now, consume later");
    setTimeout(() => p.then(null, () => {
      info("Consumed promise");
    }), 200);
    yield {
      promise: p,
      leftUncaught: true,
      consumed: true,
      name: "Reject now, consume later",
    };

    yield {
      promise: Promise.all([
        Promise.resolve("Promise.all"),
        rejectLater()
      ]),
      leftUncaught: true,
      name: "Rejecting through Promise.all"
    };
    yield {
      promise: Promise.race([
        resolveLater(500),
        Promise.reject(),
      ]),
      leftUncaught: true, 
      name: "Rejecting through Promise.race",
    };
    yield {
      promise: Promise.race([
        Promise.resolve(),
        rejectLater(500)
      ]),
      leftUncaught: false, 
      name: "Resolving through Promise.race",
    };

    let boom = new Error("`throw` in the constructor");
    yield {
      promise: new Promise(() => { throw boom; }),
      leftUncaught: true,
      name: "Throwing in the constructor",
    };

    let rejection = Promise.reject("`reject` during resolution");
    yield {
      promise: rejection,
      leftUncaught: false,
      consumed: false, 
      name: "Promise.reject, again",
    };

    yield {
      promise: new Promise(resolve => resolve(rejection)),
      leftUncaught: true,
      consumed: false,
      name: "Resolving with a rejected promise",
    };

    yield {
      promise: Promise.resolve(0).then(() => rejection),
      leftUncaught: true,
      consumed: false,
      name: "Returning a rejected promise from success handler",
    };

    yield {
      promise: Promise.resolve(0).then(() => { throw new Error(); }),
      leftUncaught: true,
      consumed: false,
      name: "Throwing during the call to the success callback",
    };
  };
  let samples = [];
  for (let s of makeSamples()) {
    samples.push(s);
    info("Promise '" + s.name + "' has id " + PromiseDebugging.getPromiseID(s.promise));
  }

  PromiseDebugging.addUncaughtRejectionObserver(observer);

  for (let s of samples) {
    names.set(s.promise, s.name);
    if (s.leftUncaught || false) {
      onLeftUncaught.expected.add(s.promise);
    }
    if (s.consumed || false) {
      onConsumed.expected.add(s.promise);
    }
  }

  info("Test setup, waiting for callbacks.");
  yield onLeftUncaught.blocker;

  info("All calls to onLeftUncaught are complete.");
  if (onConsumed.expected.size != 0) {
    info("onConsumed is still waiting for the following Promise:");
    info(JSON.stringify([names.get(x) for (x of onConsumed.expected.values())]));
    yield onConsumed.blocker;
  }

  info("All calls to onConsumed are complete.");
  PromiseDebugging.removeUncaughtRejectionObserver(observer);
});


add_task(function* test_uninstall_observer() {
  let Observer = function() {
    this.blocker = new Promise(resolve => this.resolve = resolve);
    this.active = true;
  };
  Observer.prototype = {
    set active(x) {
      this._active = x;
      if (x) {
        PromiseDebugging.addUncaughtRejectionObserver(this);
      } else {
        PromiseDebugging.removeUncaughtRejectionObserver(this);
      }
    },
    onLeftUncaught: function() {
      Assert.ok(this._active, "This observer is active.");
      this.resolve();
    },
    onConsumed: function() {
      Assert.ok(false, "We should not consume any Promise.");
    },
  };

  info("Adding an observer.");
  let deactivate = new Observer();
  Promise.reject("I am an uncaught rejection.");
  yield deactivate.blocker;
  Assert.ok(true, "The observer has observed an uncaught Promise.");
  deactivate.active = false;
  info("Removing the observer, it should not observe any further uncaught Promise.");

  info("Rejecting a Promise and waiting a little to give a chance to observers.");
  let wait = new Observer();
  Promise.reject("I am another uncaught rejection.");
  yield wait.blocker;
  yield new Promise(resolve => setTimeout(resolve, 100));
  
  wait.active = false;

});
