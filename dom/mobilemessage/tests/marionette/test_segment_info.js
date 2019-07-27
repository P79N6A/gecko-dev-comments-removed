


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const LEN_7BIT = 160;
const LEN_7BIT_WITH_8BIT_REF = 153;
const LEN_7BIT_WITH_16BIT_REF = 152;
const LEN_UCS2 = 70;
const LEN_UCS2_WITH_8BIT_REF = 67;
const LEN_UCS2_WITH_16BIT_REF = 66;

function times(str, n) {
  return (new Array(n + 1)).join(str);
}

function test(text, segments, charsPerSegment, charsAvailableInLastSegment) {
  
  
  ok(true, "Testing '" + text + "' ...");

  let domRequest = manager.getSegmentInfoForText(text);
  ok(domRequest, "DOMRequest object returned.");

  return wrapDomRequestAsPromise(domRequest)
    .then(function(aEvent) {
      let result = aEvent.target.result;
      ok(result, "aEvent.target.result = " + JSON.stringify(result));

      is(result.segments, segments, "result.segments");
      is(result.charsPerSegment, charsPerSegment, "result.charsPerSegment");
      is(result.charsAvailableInLastSegment, charsAvailableInLastSegment,
         "result.charsAvailableInLastSegment");
    });
}

startTestCommon(function() {
  
  return pushPrefEnv({ set: [["dom.sms.strict7BitEncoding", false]] })

    
    
    
    .then(() => test("a",       1, LEN_7BIT, LEN_7BIT - 1))
    
    .then(() => test("\u20ac",  1, LEN_7BIT, LEN_7BIT - 2))
    
    .then(() => test(" ",       1, LEN_7BIT, LEN_7BIT - 1))
    
    .then(() => test("a\u20ac", 1, LEN_7BIT, LEN_7BIT - 3))
    .then(() => test("a ",      1, LEN_7BIT, LEN_7BIT - 2))
    .then(() => test("\u20aca", 1, LEN_7BIT, LEN_7BIT - 3))
    .then(() => test("\u20ac ", 1, LEN_7BIT, LEN_7BIT - 3))
    .then(() => test(" \u20ac", 1, LEN_7BIT, LEN_7BIT - 3))
    .then(() => test(" a",      1, LEN_7BIT, LEN_7BIT - 2))

    
    
    
    .then(() => test(times("a", LEN_7BIT), 1, LEN_7BIT, 0))
    
    
    .then(() => test(times("a", LEN_7BIT + 1),
                     2, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 8))
    
    .then(() => test(times("a", LEN_7BIT_WITH_8BIT_REF * 2),
                     2, LEN_7BIT_WITH_8BIT_REF, 0))
    
    .then(() => test(times("a", LEN_7BIT_WITH_8BIT_REF * 2 + 1),
                     3, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 1))
    
    .then(() => test(times("\u20ac", LEN_7BIT / 2), 1, LEN_7BIT, 0))
    
    
    .then(() => test(times("\u20ac", LEN_7BIT / 2 + 1),
                     2, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 10))
    
    
    
    .then(() => test(times("\u20ac", 1 + 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)),
                     3, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 2))
    
    .then(() => test("a" + times("\u20ac", 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)),
                     2, LEN_7BIT_WITH_8BIT_REF, 1))
    .then(() => test(times("\u20ac", 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)) + "a",
                     2, LEN_7BIT_WITH_8BIT_REF, 0))

    
    
    
    .then(() => test("\u6afb",       1, LEN_UCS2, LEN_UCS2 - 1))
    
    .then(() => test("\u6afba",      1, LEN_UCS2, LEN_UCS2 - 2))
    .then(() => test("\u6afb\u20ac", 1, LEN_UCS2, LEN_UCS2 - 2))
    .then(() => test("\u6afb ",      1, LEN_UCS2, LEN_UCS2 - 2))

    
    
    
    .then(() => test(times("\u6afb", LEN_UCS2), 1, LEN_UCS2, 0))
    
    
    .then(() => test(times("\u6afb", LEN_UCS2 + 1),
                     2, LEN_UCS2_WITH_8BIT_REF, LEN_UCS2_WITH_8BIT_REF - 4))
    
    .then(() => test(times("\u6afb", LEN_UCS2_WITH_8BIT_REF * 2),
                     2, LEN_UCS2_WITH_8BIT_REF, 0))
    
    .then(() => test(times("\u6afb", LEN_UCS2_WITH_8BIT_REF * 2 + 1),
                     3, LEN_UCS2_WITH_8BIT_REF, LEN_UCS2_WITH_8BIT_REF - 1))

    
    
    .then(() => pushPrefEnv({ set: [["dom.sms.strict7BitEncoding", true]] }))

    
    .then(() => test("\u0041",       1, LEN_7BIT, LEN_7BIT - 1))
    
    .then(() => test("\u00c0",       1, LEN_7BIT, LEN_7BIT - 1))
    
    .then(() => test("\u00c0\u0041", 1, LEN_7BIT, LEN_7BIT - 2))
    .then(() => test("\u0041\u00c0", 1, LEN_7BIT, LEN_7BIT - 2))
    
    .then(() => test("\u1234",       1, LEN_7BIT, LEN_7BIT - 1));
});
