














"use strict";


















































XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

let gTerminationTasks = [];









function* terminationTaskFn() {
  for (let taskFn of gTerminationTasks) {
    try {
      yield Task.spawn(taskFn);
    } catch (ex) {
      Output.print(ex);
      Assert.ok(false);
    }
  }
};

function add_termination_task(taskFn) {
  gTerminationTasks.push(taskFn);
}





function getTaskId(stackFrame) {
  return stackFrame.filename + ":" + stackFrame.lineNumber;
}


let _mochitestAssert = {
  ok: function (actual) {
    let stack = Components.stack.caller;
    ok(actual, "[" + stack.name + " : " + stack.lineNumber + "] " + actual +
               " == true");
  },
  equal: function (actual, expected) {
    let stack = Components.stack.caller;
    is(actual, expected, "[" + stack.name + " : " + stack.lineNumber + "] " +
               actual + " == " + expected);
  },
};



