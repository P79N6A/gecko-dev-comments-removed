


MARIONETTE_TIMEOUT = 60000;

const LEN_7BIT = 160;
const LEN_7BIT_WITH_8BIT_REF = 153;
const LEN_7BIT_WITH_16BIT_REF = 152;
const LEN_UCS2 = 70;
const LEN_UCS2_WITH_8BIT_REF = 67;
const LEN_UCS2_WITH_16BIT_REF = 66;

SpecialPowers.setBoolPref("dom.sms.enabled", true);
let currentStrict7BitEncoding = false;
SpecialPowers.setBoolPref("dom.sms.strict7BitEncoding",
                          currentStrict7BitEncoding);
SpecialPowers.addPermission("sms", true, document);

let manager = window.navigator.mozMobileMessage;
ok(manager instanceof MozMobileMessageManager,
   "manager is instance of " + manager.constructor);

function times(str, n) {
  return (new Array(n + 1)).join(str);
}

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

function addTest(text, strict7BitEncoding, expected) {
  tasks.push(function () {
    if (strict7BitEncoding != currentStrict7BitEncoding) {
      currentStrict7BitEncoding = strict7BitEncoding;
      SpecialPowers.setBoolPref("dom.sms.strict7BitEncoding",
                                currentStrict7BitEncoding);
    }

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

      is(result.segments, expected[0], "segments");
      is(result.charsPerSegment, expected[1], "charsPerSegment");
      is(result.charsAvailableInLastSegment, expected[2],
         "charsAvailableInLastSegment");

      tasks.next();
    };

    domRequest.onerror = function(e) {
      ok(false, "Failed to call getSegmentInfoForText().");
      tasks.finish();
    };
  });
}




addTest("a", false, [1, LEN_7BIT, LEN_7BIT - 1]);

addTest("\u20ac", false, [1, LEN_7BIT, LEN_7BIT - 2]);

addTest(" ", false, [1, LEN_7BIT, LEN_7BIT - 1]);

addTest("a\u20ac", false, [1, LEN_7BIT, LEN_7BIT - 3]);
addTest("a ", false, [1, LEN_7BIT, LEN_7BIT - 2]);
addTest("\u20aca", false, [1, LEN_7BIT, LEN_7BIT - 3]);
addTest("\u20ac ", false, [1, LEN_7BIT, LEN_7BIT - 3]);
addTest(" \u20ac", false, [1, LEN_7BIT, LEN_7BIT - 3]);
addTest(" a", false, [1, LEN_7BIT, LEN_7BIT - 2]);




addTest(times("a", LEN_7BIT), false, [1, LEN_7BIT, 0]);


addTest(times("a", LEN_7BIT + 1), false,
        [2, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 8]);

addTest(times("a", LEN_7BIT_WITH_8BIT_REF * 2), false,
        [2, LEN_7BIT_WITH_8BIT_REF, 0]);

addTest(times("a", LEN_7BIT_WITH_8BIT_REF * 2 + 1), false,
        [3, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 1]);

addTest(times("\u20ac", LEN_7BIT / 2), false, [1, LEN_7BIT, 0]);


addTest(times("\u20ac", LEN_7BIT / 2 + 1), false,
        [2, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 10]);



addTest(times("\u20ac", 1 + 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)), false,
        [3, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 2]);

addTest("a" + times("\u20ac", 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)), false,
        [2, LEN_7BIT_WITH_8BIT_REF, 1]);
addTest(times("\u20ac", 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)) + "a", false,
        [2, LEN_7BIT_WITH_8BIT_REF, 0]);




addTest("\u6afb", false, [1, LEN_UCS2, LEN_UCS2 - 1]);

addTest("\u6afba", false, [1, LEN_UCS2, LEN_UCS2 - 2]);
addTest("\u6afb\u20ac", false, [1, LEN_UCS2, LEN_UCS2 - 2]);
addTest("\u6afb ", false, [1, LEN_UCS2, LEN_UCS2 - 2]);




addTest(times("\u6afb", LEN_UCS2), false, [1, LEN_UCS2, 0]);


addTest(times("\u6afb", LEN_UCS2 + 1), false,
        [2, LEN_UCS2_WITH_8BIT_REF, LEN_UCS2_WITH_8BIT_REF - 4]);

addTest(times("\u6afb", LEN_UCS2_WITH_8BIT_REF * 2), false,
        [2, LEN_UCS2_WITH_8BIT_REF, 0]);

addTest(times("\u6afb", LEN_UCS2_WITH_8BIT_REF * 2 + 1), false,
        [3, LEN_UCS2_WITH_8BIT_REF, LEN_UCS2_WITH_8BIT_REF - 1]);




addTest("\u0041", true, [1, LEN_7BIT, LEN_7BIT - 1]);

addTest("\u00c0", true, [1, LEN_7BIT, LEN_7BIT - 1]);

addTest("\u00c0\u0041", true, [1, LEN_7BIT, LEN_7BIT - 2]);
addTest("\u0041\u00c0", true, [1, LEN_7BIT, LEN_7BIT - 2]);

addTest("\u1234", true, [1, LEN_7BIT, LEN_7BIT - 1]);



tasks.push(function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  SpecialPowers.clearUserPref("dom.sms.strict7BitEncoding");
  finish();
});

tasks.run();
