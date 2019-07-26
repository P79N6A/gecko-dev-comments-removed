


MARIONETTE_TIMEOUT = 60000;

const MMS_MAX_LENGTH_SUBJECT = 40;

SpecialPowers.addPermission("sms", true, document);
SpecialPowers.setBoolPref("dom.sms.enabled", true);

let tasks = {
  
  
  _tasks: [],
  _nextTaskIndex: 0,

  push: function push(func) {
    this._tasks.push(func);
  },

  next: function next() {
    let index = this._nextTaskIndex++;
    let task = this._tasks[index];
    try {
      task();
    } catch (ex) {
      ok(false, "test task[" + index + "] throws: " + ex);
      
      if (index != this._tasks.length - 1) {
        this.finish();
      }
    }
  },

  finish: function finish() {
    this._tasks[this._tasks.length - 1]();
  },

  run: function run() {
    this.next();
  }
};

let mozMobileMessage;

function getAllMessages(callback, filter, reverse) {
  if (!filter) {
    filter = new MozSmsFilter;
  }
  let messages = [];
  let request = mozMobileMessage.getMessages(filter, reverse || false);
  request.onsuccess = function(event) {
    if (request.result) {
      messages.push(request.result);
      request.continue();
      return;
    }

    window.setTimeout(callback.bind(null, messages), 0);
  }
}

function deleteAllMessages() {
  getAllMessages(function deleteAll(messages) {
    let message = messages.shift();
    if (!message) {
      ok(true, "all messages deleted");
      tasks.next();
      return;
    }

    let request = mozMobileMessage.delete(message.id);
    request.onsuccess = deleteAll.bind(null, messages);
    request.onerror = function (event) {
      ok(false, "failed to delete all messages");
      tasks.finish();
    }
  });
}

function testInvalidAddressForSMS(aInvalidAddr)  {
  log("mozMobileMessage.send(...) should get 'InvalidAddressError' error " +
      "when attempting to send SMS to: " + aInvalidAddr);

  let request = mozMobileMessage.send(aInvalidAddr, "Test");

  request.onerror = function(event) {
    log("Received 'onerror' DOMRequest event.");
    let error = event.target.error;
    ok(error instanceof DOMError, "should be a valid DOMError object");
    ok(error.name === "InvalidAddressError", "should be 'InvalidAddressError'");
    tasks.next();
  };
}

function testInvalidAddressForMMS(aInvalidAddrs)  {
  log("mozMobileMessage.sendMMS(...) should get 'InvalidAddressError' error " +
      "when attempting to send MMS to: " + aInvalidAddrs);

  let request = mozMobileMessage.sendMMS({
    subject: "Test",
    receivers: aInvalidAddrs,
    attachments: [],
  });

  request.onerror = function(event) {
    log("Received 'onerror' DOMRequest event.");
    let error = event.target.error;
    ok(error instanceof DOMError, "should be a valid DOMError object");
    ok(error.name === "InvalidAddressError", "should be 'InvalidAddressError'");
    tasks.next();
  };
}

tasks.push(function () {
  log("Verifying initial state.");

  mozMobileMessage = window.navigator.mozMobileMessage;
  ok(mozMobileMessage instanceof MozMobileMessageManager);

  tasks.next();
});


tasks.push(testInvalidAddressForSMS.bind(this, "&%&"));
tasks.push(testInvalidAddressForSMS.bind(this, ""));


tasks.push(testInvalidAddressForMMS.bind(this, ["&%&"]));
tasks.push(testInvalidAddressForMMS.bind(this, [""]));
tasks.push(testInvalidAddressForMMS.bind(this, ["123", "&%&", "456"]));

tasks.push(deleteAllMessages);


tasks.push(function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
});

tasks.run();
