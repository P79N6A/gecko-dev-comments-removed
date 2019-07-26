


MARIONETTE_TIMEOUT = 60000;


const PDU_MAX_USER_DATA_7BIT = 160;

SpecialPowers.setBoolPref("dom.sms.enabled", true);
SpecialPowers.addPermission("sms", true, document);

let manager = window.navigator.mozMobileMessage;
ok(manager instanceof MozMobileMessageManager,
   "manager is instance of " + manager.constructor);

let tasks = {
  
  
  _tasks: [],
  _nextTaskIndex: 0,

  push: function(func) {
    this._tasks.push(func);
  },

  next: function() {
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

  finish: function() {
    this._tasks[this._tasks.length - 1]();
  },

  run: function() {
    this.next();
  }
};

function addTest(text, segments, charsPerSegment, charsAvailableInLastSegment) {
  tasks.push(function() {
    log("Testing '" + text + "' ...");
    let domRequest = manager.getSegmentInfoForText(text);
    ok(domRequest, "DOMRequest object returned.");

    domRequest.onsuccess = function(e) {
      log("Received 'onsuccess' DOMRequest event.");

      let result = e.target.result;
      if (!result) {
        ok(false, "getSegmentInfoForText() result is not valid.");
        tasks.finish();
        return;
      }

      is(result.segments, segments, "info.segments");
      is(result.charsPerSegment, charsPerSegment, "info.charsPerSegment");
      is(result.charsAvailableInLastSegment, charsAvailableInLastSegment,
         "info.charsAvailableInLastSegment");

      tasks.next();
    };

    domRequest.onerror = function(e) {
      ok(false, "Failed to call getSegmentInfoForText().");
      tasks.finish();
    };
  });
}

addTest(null, 1, PDU_MAX_USER_DATA_7BIT,
        PDU_MAX_USER_DATA_7BIT - "null".length);


addTest(undefined, 1, PDU_MAX_USER_DATA_7BIT,
        PDU_MAX_USER_DATA_7BIT - "undefined".length);


addTest(0,   1, PDU_MAX_USER_DATA_7BIT, PDU_MAX_USER_DATA_7BIT - "0".length);
addTest(1.0, 1, PDU_MAX_USER_DATA_7BIT, PDU_MAX_USER_DATA_7BIT - "1".length);




addTest({}, 1, PDU_MAX_USER_DATA_7BIT,
        PDU_MAX_USER_DATA_7BIT - (("" + {}).length + 2));


let date = new Date();
addTest(date, 1, PDU_MAX_USER_DATA_7BIT,
        PDU_MAX_USER_DATA_7BIT - ("" + date).length);

addTest("", 1, PDU_MAX_USER_DATA_7BIT,
        PDU_MAX_USER_DATA_7BIT - "".length);


tasks.push(function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
});

tasks.run();
