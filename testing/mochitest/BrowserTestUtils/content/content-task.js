



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Task.jsm", this);

addMessageListener("content-task:spawn", function (msg) {
  let id = msg.data.id;
  let source = msg.data.runnable || "()=>{}";

  try {
    let runnablestr = `
      (() => {
        return (${source});
      })();`

    let runnable = eval(runnablestr);
    let iterator = runnable(msg.data.arg);
    Task.spawn(iterator).then((val) => {
      sendAsyncMessage("content-task:complete", {
        id: id,
        result: val,
      });
    }, (e) => {
      sendAsyncMessage("content-task:complete", {
        id: id,
        error: e.toString(),
      });
    });
  } catch (e) {
    sendAsyncMessage("content-task:complete", {
      id: id,
      error: e.toString(),
    });
  }
});
