


SpecialPowers.addPermission("mobileconnection", true, document);

let iccManager = navigator.mozIccManager;
ok(iccManager instanceof MozIccManager,
   "iccManager is instanceof " + iccManager.constructor);






let iccIds = iccManager.iccIds;
ok(Array.isArray(iccIds), "iccIds is an array");
is(iccIds.length, 1, "iccIds.length is " + iccIds.length);

let iccId = iccIds[0];
is(iccId, "89014103211118510720", "iccId is " + iccId);

let icc = iccManager.getIccById(iccId);
ok(icc instanceof MozIcc, "icc is instanceof " + icc.constructor);


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
