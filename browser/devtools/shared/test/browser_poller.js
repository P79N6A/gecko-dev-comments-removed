




const { Poller } = devtools.require("devtools/shared/poller");

add_task(function* () {
  let count1 = 0, count2 = 0, count3 = 0;

  let poller1 = new Poller(function () {
    count1++;
  }, 1000000000, true);
  let poller2 = new Poller(function () {
    count2++;
  }, 10);
  let poller3 = new Poller(function () {
    count3++;
  }, 1000000000);

  poller2.on();

  ok(!poller1.isPolling(), "isPolling() returns false for an off poller");
  ok(poller2.isPolling(), "isPolling() returns true for an on poller");

  yield waitUntil(() => count2 > 10);

  ok(count2 > 10, "poller that was turned on polled several times");
  ok(count1 === 0, "poller that was never turned on never polled");

  yield poller2.off();
  let currentCount2 = count2;

  poller1.on(); 
  poller3.on(); 

  yield waitUntil(() => count1 === 1);
  ok(true, "Poller calls fn immediately when `immediate` is true");
  ok(count3 === 0, "Poller does not call fn immediately when `immediate` is not set");

  ok(count2 === currentCount2, "a turned off poller does not continue to poll");
  yield poller2.off();
  yield poller2.off();
  yield poller2.off();
  ok(true, "Poller.prototype.off() is idempotent");

  
  is(count1, 1, "wait time works");

  ok(poller1.isPolling(), "isPolling() returns true for an on poller");
  ok(!poller2.isPolling(), "isPolling() returns false for an off poller");
});

add_task(function *() {
  let count = -1;
  
  
  
  let asyncPoller = new Poller(function () {
    count++;
    ok(!(count%10), `Async poller called with a multiple of 10: ${count}`);
    return new Promise(function (resolve, reject) {
      let add9 = 9;
      let interval = setInterval(() => {
        if (add9--) {
          count++;
        } else {
          clearInterval(interval);
          resolve();
        }
      }, 10);
    });
  });

  asyncPoller.on(1);
  yield waitUntil(() => count > 50);
  yield asyncPoller.off();
});

add_task(function *() {
  
  
  
  
  let inflightFinished = null;
  let pollCalls = 0;
  let asyncPoller = new Poller(function () {
    pollCalls++;
    return new Promise(function (resolve, reject) {
      setTimeout(() => {
        inflightFinished = true;
        resolve();
      }, 1000);
    });
  }, 1, true);
  asyncPoller.on();

  yield asyncPoller.off();
  ok(inflightFinished, "off() method does not resolve until remaining inflight poll calls finish");
  is(pollCalls, 1, "should only be one poll call to occur before turning off polling");
});

add_task(function *() {
  
  
  
  
  let inflightFinished = null;
  let pollCalls = 0;
  let asyncPoller = new Poller(function () {
    pollCalls++;
    return new Promise(function (resolve, reject) {
      setTimeout(() => {
        inflightFinished = true;
        resolve();
      }, 1000);
    });
  }, 1, true);
  asyncPoller.on();

  yield asyncPoller.destroy();
  ok(inflightFinished, "destroy() method does not resolve until remaining inflight poll calls finish");
  is(pollCalls, 1, "should only be one poll call to occur before destroying polling");
  
  try {
    asyncPoller.on();
    ok(false, "Calling on() after destruction should throw");
  } catch (e) {
    ok(true, "Calling on() after destruction should throw");
  }
});
