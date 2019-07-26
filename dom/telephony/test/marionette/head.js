


let Promise = SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;




let emulator = (function() {
  let pendingCmdCount = 0;
  let originalRunEmulatorCmd = runEmulatorCmd;

  
  runEmulatorCmd = function() {
    throw "Use emulator.run(cmd, callback) instead of runEmulatorCmd";
  };

  function run(cmd, callback) {
    pendingCmdCount++;
    originalRunEmulatorCmd(cmd, function(result) {
      pendingCmdCount--;
      if (callback && typeof callback === "function") {
        callback(result);
      }
    });
  }

  


  function waitFinish() {
    let deferred = Promise.defer();

    waitFor(function() {
      deferred.resolve();
    }, function() {
      return pendingCmdCount === 0;
    });

    return deferred.promise;
  }

  return {
    run: run,
    waitFinish: waitFinish
  };
}());

function startTest(test) {
  
  finish = (function() {
    let originalFinish = finish;

    function tearDown() {
      log('== Test TearDown ==');
      emulator.waitFinish()
        .then(function() {
          originalFinish.apply(this, arguments);
        });
    }

    return tearDown.bind(this);
  }());

  
  test();
}
