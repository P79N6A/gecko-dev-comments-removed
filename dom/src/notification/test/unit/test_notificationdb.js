const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

let systemNotification = {
  origin: "app://system.gaiamobile.org/manifest.webapp",
  id: "{2bc883bf-2809-4432-b0f4-f54e10372764}",
  title: "SystemNotification:" + Date.now(),
  dir: "auto",
  lang: "",
  body: "System notification body",
  tag: "",
  icon: "icon.png"
};

let calendarNotification = {
  origin: "app://calendar.gaiamobile.org/manifest.webapp",
  id: "{d8d11299-a58e-429b-9a9a-57c562982fbf}",
  title: "CalendarNotification:" + Date.now(),
  dir: "auto",
  lang: "",
  body: "Calendar notification body",
  tag: "",
  icon: "icon.png"
};

function run_test() {
  do_get_profile();
  Cu.import("resource://gre/modules/NotificationDB.jsm");
  run_next_test();
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
  cpmm.sendAsyncMessage(msg, payload);
}


function compareNotification(notif1, notif2) {
  
  for (let prop in notif1) {
    
    do_check_eq(notif1[prop], notif2[prop]);
  }
}


add_test(function test_get_none() {
  let requestID = 0;
  let msgReply = "Notification:GetAll:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
    do_check_eq(0, message.data.notifications.length);
  };

  addAndSend("Notification:GetAll", msgReply, msgHandler, {
    origin: systemNotification.origin,
    requestID: requestID
  });
});


add_test(function test_send_one() {
  let requestID = 1;
  let msgReply = "Notification:Save:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
  };

  addAndSend("Notification:Save", msgReply, msgHandler, {
    origin: systemNotification.origin,
    notification: systemNotification,
    requestID: requestID
  });
});


add_test(function test_get_one() {
  let requestID = 2;
  let msgReply = "Notification:GetAll:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
    do_check_eq(1, message.data.notifications.length);
    
    compareNotification(systemNotification, message.data.notifications[0]);
  };

  addAndSend("Notification:GetAll", msgReply, msgHandler, {
    origin: systemNotification.origin,
    requestID: requestID
  });
});


add_test(function test_delete_one() {
  let requestID = 3;
  let msgReply = "Notification:Delete:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
  };

  addAndSend("Notification:Delete", msgReply, msgHandler, {
    origin: systemNotification.origin,
    id: systemNotification.id,
    requestID: requestID
  });
});


add_test(function test_get_none_again() {
  let requestID = 4;
  let msgReply = "Notification:GetAll:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
    do_check_eq(0, message.data.notifications.length);
  };

  addAndSend("Notification:GetAll", msgReply, msgHandler, {
    origin: systemNotification.origin,
    requestID: requestID
  });
});


add_test(function test_delete_one_nonexistent() {
  let requestID = 5;
  let msgReply = "Notification:Delete:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
  };

  addAndSend("Notification:Delete", msgReply, msgHandler, {
    origin: systemNotification.origin,
    id: systemNotification.id,
    requestID: requestID
  });
});


add_test(function test_send_two_get_one() {
  let requestID = 6;
  let calls = 0;

  let msgGetReply = "Notification:GetAll:Return:OK";
  let msgGetHandler = function(message) {
    do_check_eq(requestID + 2, message.data.requestID);
    do_check_eq(1, message.data.notifications.length);
    
    compareNotification(systemNotification, message.data.notifications[0]);
  };

  let msgSaveReply = "Notification:Save:Return:OK";
  let msgSaveHandler = function(message) {
    calls += 1;
    if (calls === 2) {
      addAndSend("Notification:GetAll", msgGetReply, msgGetHandler, {
        origin: systemNotification.origin,
        requestID: (requestID + 2)
      });
    }
  };

  addAndSend("Notification:Save", msgSaveReply, msgSaveHandler, {
    origin: systemNotification.origin,
    notification: systemNotification,
    requestID: requestID
  }, false);

  addAndSend("Notification:Save", msgSaveReply, msgSaveHandler, {
    origin: systemNotification.origin,
    notification: systemNotification,
    requestID: (requestID + 1)
  }, false);
});


add_test(function test_delete_previous() {
  let requestID = 8;
  let msgReply = "Notification:Delete:Return:OK";
  let msgHandler = function(message) {
    do_check_eq(requestID, message.data.requestID);
  };

  addAndSend("Notification:Delete", msgReply, msgHandler, {
    origin: systemNotification.origin,
    id: systemNotification.id,
    requestID: requestID
  });
});


add_test(function test_send_two_get_one() {
  let requestID = 10;
  let tag = "voicemail";

  let systemNotification1 = systemNotification;
  systemNotification1.id = "{f271f9ee-3955-4c10-b1f2-af552fb270ee}";
  systemNotification1.tag = tag;

  let systemNotification2 = systemNotification;
  systemNotification2.id = "{8ef9a628-f0f4-44b4-820d-c117573c33e3}";
  systemNotification2.tag = tag;

  let msgGetReply = "Notification:GetAll:Return:OK";
  let msgGetNotifHandler = {
    receiveMessage: function(message) {
      if (message.name === msgGetReply) {
        cpmm.removeMessageListener(msgGetReply, msgGetNotifHandler);
        let notifications = message.data.notifications;
        
        do_check_eq(1, notifications.length);
        
        compareNotification(systemNotification2, notifications[0]);
        run_next_test();
      }
    }
  };

  cpmm.addMessageListener(msgGetReply, msgGetNotifHandler);

  let msgSaveReply = "Notification:Save:Return:OK";
  let msgSaveCalls = 0;
  let msgSaveHandler = function(message) {
    msgSaveCalls++;
    
    if (msgSaveCalls === 2) {
      cpmm.sendAsyncMessage("Notification:GetAll", {
        origin: systemNotification1.origin,
        requestID: message.data.requestID + 2 
      });
    }
  };

  addAndSend("Notification:Save", msgSaveReply, msgSaveHandler, {
    origin: systemNotification1.origin,
    notification: systemNotification1,
    requestID: requestID 
  }, false);

  addAndSend("Notification:Save", msgSaveReply, msgSaveHandler, {
    origin: systemNotification2.origin,
    notification: systemNotification2,
    requestID: (requestID + 1) 
  }, false);
});


add_test(function test_send_two_get_two() {
  let requestID = 20;
  let tag = "voicemail";

  let systemNotification1 = systemNotification;
  systemNotification1.tag = tag;

  let calendarNotification2 = calendarNotification;
  calendarNotification2.tag = tag;

  let msgGetReply = "Notification:GetAll:Return:OK";
  let msgGetCalls = 0;
  let msgGetHandler = {
    receiveMessage: function(message) {
      if (message.name === msgGetReply) {
        msgGetCalls++;
        let notifications = message.data.notifications;

        
        do_check_eq(1, notifications.length);

        
        if (msgGetCalls === 1) {
          compareNotification(systemNotification1, notifications[0]);
        }

        
        if (msgGetCalls === 2) {
          cpmm.removeMessageListener(msgGetReply, msgGetHandler);
          compareNotification(calendarNotification2, notifications[0]);
          run_next_test();
        }
      }
    }
  };
  cpmm.addMessageListener(msgGetReply, msgGetHandler);

  let msgSaveReply = "Notification:Save:Return:OK";
  let msgSaveCalls = 0;
  let msgSaveHandler = {
    receiveMessage: function(message) {
      if (message.name === msgSaveReply) {
        msgSaveCalls++;
        if (msgSaveCalls === 2) {
          cpmm.removeMessageListener(msgSaveReply, msgSaveHandler);

          
          cpmm.sendAsyncMessage("Notification:GetAll", {
            origin: systemNotification1.origin,
            requestID: message.data.requestID + 1 
          });

          cpmm.sendAsyncMessage("Notification:GetAll", {
            origin: calendarNotification2.origin,
            requestID: message.data.requestID + 2 
          });
        }
      }
    }
  };
  cpmm.addMessageListener(msgSaveReply, msgSaveHandler);

  cpmm.sendAsyncMessage("Notification:Save", {
    origin: systemNotification1.origin,
    notification: systemNotification1,
    requestID: requestID 
  });

  cpmm.sendAsyncMessage("Notification:Save", {
    origin: calendarNotification2.origin,
    notification: calendarNotification2,
    requestID: (requestID + 1) 
  });
});
