


SpecialPowers.addPermission("mobileconnection", true, document);



let mobileConnection = window.navigator.mozMobileConnections[0];
ok(mobileConnection instanceof MozMobileConnection,
   "mobileConnection is instanceof " + mobileConnection.constructor);


let cleanUp = function() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
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
  pendingCommandCount: 0,

  sendCommand: function(cmd, callback) {
    this.pendingCommandCount++;
    runEmulatorCmd(cmd, function(results) {
      this.pendingCommandCount--;

      let result = results[results.length - 1];
      is(result, "OK", "'"+ cmd +"' returns '" + result + "'");

      if (callback && typeof callback === "function") {
        callback(results);
      }
    });
  },
};
