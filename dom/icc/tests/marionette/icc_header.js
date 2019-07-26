


SpecialPowers.addPermission("mobileconnection", true, document);

let icc = navigator.mozIccManager;
ok(icc instanceof MozIccManager,
   "icc is instanceof " + icc.constructor);


let cleanUp = function () {
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
    runEmulatorCmd(cmd, function(result) {
      this.pendingCommandCount--;
      is(result[result.length - 1], "OK");

      if (callback && typeof callback === "function") {
        callback(result);
      }
    });
  },
};
