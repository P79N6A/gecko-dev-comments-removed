












"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

let sharedUrl = "chrome://mochitests/content/chrome/" +
                "toolkit/components/formautofill/test/chrome/loader_common.js";
Services.scriptloader.loadSubScript(sharedUrl, this);



let Output = {
  print: message => assert.ok(true, message),
};


let Assert = {
  ok: assert.ok,
  equal: assert.equal,
};


function add_task_in_parent_process(taskFn, taskIdOverride) {
  let taskId = taskIdOverride || getTaskId(Components.stack.caller);
  Output.print("Registering in the parent process: " + taskId);
  addMessageListener("start_task_" + taskId, function () {
    Task.spawn(function* () {
      try {
        Output.print("Running in the parent process " + taskId);
        yield Task.spawn(taskFn);
      } catch (ex) {
        assert.ok(false, ex);
      }

      sendAsyncMessage("finish_task_" + taskId, {});
    });
  });
}
let add_task = function () {};
let add_task_in_child_process = function () {};
let add_task_in_both_processes = add_task_in_parent_process;



let context = this;
addMessageListener("start_load_in_parent", function (message) {
  Output.print("Starting loading infrastructure in parent process.");
  let headUrl = "chrome://mochitests/content/chrome/" +
                "toolkit/components/formautofill/test/chrome/head_common.js";
  Services.scriptloader.loadSubScript(headUrl, context);

  Services.scriptloader.loadSubScript(message.testUrl, context);

  
  add_task_in_parent_process(terminationTaskFn, terminationTaskFn.name);

  Output.print("Finished loading infrastructure in parent process.");
  sendAsyncMessage("finish_load_in_parent", {});
});



