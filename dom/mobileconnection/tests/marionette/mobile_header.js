


SpecialPowers.addPermission("mobileconnection", true, document);



let mobileConnection = window.navigator.mozMobileConnections[0];
ok(mobileConnection instanceof MozMobileConnection,
   "mobileConnection is instanceof " + mobileConnection.constructor);

let _pendingEmulatorCmdCount = 0;


let cleanUp = function() {
  waitFor(function() {
    SpecialPowers.removePermission("mobileconnection", document);
    finish();
  }, function() {
    return _pendingEmulatorCmdCount === 0;
  });
};


let taskHelper = {
  tasks: [],

  push: function(task) {
    this.tasks.push(task);
  },

  runNext: function() {
    let task = this.tasks.shift();
    if (!task) {
      cleanUp();
      return;
    }

    if (typeof task === "function") {
      task();
    }
  },
};


let emulatorHelper = {
  sendCommand: function(cmd, callback) {
    _pendingEmulatorCmdCount++;
    runEmulatorCmd(cmd, function(results) {
      _pendingEmulatorCmdCount--;

      let result = results[results.length - 1];
      is(result, "OK", "'"+ cmd +"' returns '" + result + "'");

      if (callback && typeof callback === "function") {
        callback(results);
      }
    });
  },
};
