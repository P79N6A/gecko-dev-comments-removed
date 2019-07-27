"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

let principal = Services.scriptSecurityManager.getSystemPrincipal();
let lockID = "{435d2192-4f21-48d4-90b7-285f147a56be}";


function startSettingsRequestManager() {
  Cu.import("resource://gre/modules/SettingsRequestManager.jsm");
}


function addAndSend(msg, reply, callback, payload, runNext = true) {
  let handler = {
    receiveMessage: function(message) {
      if (message.name === reply) {
        cpmm.removeMessageListener(reply, handler);
        callback(message);
        if (runNext) {
          run_next_test();
        }
      }
    }
  };
  cpmm.addMessageListener(reply, handler);
  cpmm.sendAsyncMessage(msg, payload, undefined, principal);
}


function send_settingsRun() {
  let msg = {lockID: lockID, isServiceLock: true};
  cpmm.sendAsyncMessage("Settings:Run", msg, undefined, principal);
}

function kill_child() {
  let msg = {lockID: lockID, isServiceLock: true};
  cpmm.sendAsyncMessage("child-process-shutdown", msg, undefined, principal);
}

function run_test() {
  do_get_profile();
  startSettingsRequestManager();
  run_next_test();
}

add_test(function test_createLock() {
  let msg = {lockID: lockID, isServiceLock: true};
  cpmm.sendAsyncMessage("Settings:CreateLock", msg, undefined, principal);
  cpmm.sendAsyncMessage(
    "Settings:RegisterForMessages", undefined, undefined, principal);
  ok(true);
  run_next_test();
});

add_test(function test_get_empty() {
  let requestID = 10;
  let msgReply = "Settings:Get:OK";
  let msgHandler = function(message) {
    equal(requestID, message.data.requestID);
    equal(lockID, message.data.lockID);
    ok(Object.keys(message.data.settings).length >= 0);
  };

  addAndSend("Settings:Get", msgReply, msgHandler, {
    requestID: requestID,
    lockID: lockID,
    name: "language.current"
  });

  send_settingsRun();
});

add_test(function test_set_get_nonempty() {
  let settings = { "language.current": "fr-FR:XPC" };
  let requestIDSet = 20;
  let msgReplySet = "Settings:Set:OK";
  let msgHandlerSet = function(message) {
    equal(requestIDSet, message.data.requestID);
    equal(lockID, message.data.lockID);
  };

  addAndSend("Settings:Set", msgReplySet, msgHandlerSet, {
    requestID: requestIDSet,
    lockID: lockID,
    settings: settings
  }, false);

  let requestIDGet = 25;
  let msgReplyGet = "Settings:Get:OK";
  let msgHandlerGet = function(message) {
    equal(requestIDGet, message.data.requestID);
    equal(lockID, message.data.lockID);
    for(let p in settings) {
      equal(settings[p], message.data.settings[p]);
    }
  };

  addAndSend("Settings:Get", msgReplyGet, msgHandlerGet, {
    requestID: requestIDGet,
    lockID: lockID,
    name: Object.keys(settings)[0]
  });

  
  send_settingsRun();
});


add_test(function test_wait_for_finalize() {
  let settings = { "language.current": "en-US:XPC" };
  let requestIDSet = 30;
  let msgReplySet = "Settings:Set:OK";
  let msgHandlerSet = function(message) {
    equal(requestIDSet, message.data.requestID);
    equal(lockID, message.data.lockID);
  };

  addAndSend("Settings:Set", msgReplySet, msgHandlerSet, {
    requestID: requestIDSet,
    lockID: lockID,
    settings: settings
  }, false);

  let requestIDGet = 35;
  let msgReplyGet = "Settings:Get:OK";
  let msgHandlerGet = function(message) {
    equal(requestIDGet, message.data.requestID);
    equal(lockID, message.data.lockID);
    for(let p in settings) {
      equal(settings[p], message.data.settings[p]);
    }
  };

  addAndSend("Settings:Get", msgReplyGet, msgHandlerGet, {
    requestID: requestIDGet,
    lockID: lockID,
    name: Object.keys(settings)[0]
  });

  
  
  kill_child();

  
  send_settingsRun();
});
