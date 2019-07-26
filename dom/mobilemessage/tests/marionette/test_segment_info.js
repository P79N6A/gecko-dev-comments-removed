


MARIONETTE_TIMEOUT = 10000;

const LEN_7BIT = 160;
const LEN_7BIT_WITH_8BIT_REF = 153;
const LEN_7BIT_WITH_16BIT_REF = 152;
const LEN_UCS2 = 70;
const LEN_UCS2_WITH_8BIT_REF = 67;
const LEN_UCS2_WITH_16BIT_REF = 66;

SpecialPowers.setBoolPref("dom.sms.enabled", true);
let currentStrict7BitEncoding = false;
SpecialPowers.setBoolPref("dom.sms.strict7BitEncoding", currentStrict7BitEncoding);
SpecialPowers.addPermission("sms", true, document);

let sms = window.navigator.mozSms;
ok(sms instanceof MozSmsManager);

function times(str, n) {
  return (new Array(n + 1)).join(str);
}

function doTest(text, strict7BitEncoding, expected) {
  if (strict7BitEncoding != currentStrict7BitEncoding) {
    currentStrict7BitEncoding = strict7BitEncoding;
    SpecialPowers.setBoolPref("dom.sms.strict7BitEncoding", currentStrict7BitEncoding);
  }

  let result = sms.getSegmentInfoForText(text);
  ok(result, "result of GetSegmentInfoForText is valid");
  is(result.segments, expected[0], "segments");
  is(result.charsPerSegment, expected[1], "charsPerSegment");
  is(result.charsAvailableInLastSegment, expected[2], "charsAvailableInLastSegment");
}

function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  SpecialPowers.clearUserPref("dom.sms.strict7BitEncoding");
  finish();
}




doTest("a", false, [1, LEN_7BIT, LEN_7BIT - 1]);

doTest("\u20ac", false, [1, LEN_7BIT, LEN_7BIT - 2]);

doTest(" ", false, [1, LEN_7BIT, LEN_7BIT - 1]);

doTest("a\u20ac", false, [1, LEN_7BIT, LEN_7BIT - 3]);
doTest("a ", false, [1, LEN_7BIT, LEN_7BIT - 2]);
doTest("\u20aca", false, [1, LEN_7BIT, LEN_7BIT - 3]);
doTest("\u20ac ", false, [1, LEN_7BIT, LEN_7BIT - 3]);
doTest(" \u20ac", false, [1, LEN_7BIT, LEN_7BIT - 3]);
doTest(" a", false, [1, LEN_7BIT, LEN_7BIT - 2]);




doTest(times("a", LEN_7BIT), false, [1, LEN_7BIT, 0]);


doTest(times("a", LEN_7BIT + 1), false,
       [2, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 8]);

doTest(times("a", LEN_7BIT_WITH_8BIT_REF * 2), false,
       [2, LEN_7BIT_WITH_8BIT_REF, 0]);

doTest(times("a", LEN_7BIT_WITH_8BIT_REF * 2 + 1), false,
       [3, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 1]);

doTest(times("\u20ac", LEN_7BIT / 2), false, [1, LEN_7BIT, 0]);


doTest(times("\u20ac", LEN_7BIT / 2 + 1), false,
       [2, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 10]);



doTest(times("\u20ac", 1 + 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)), false,
       [3, LEN_7BIT_WITH_8BIT_REF, LEN_7BIT_WITH_8BIT_REF - 2]);

doTest("a" + times("\u20ac", 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)), false,
       [2, LEN_7BIT_WITH_8BIT_REF, 1]);
doTest(times("\u20ac", 2 * Math.floor(LEN_7BIT_WITH_8BIT_REF / 2)) + "a", false,
       [2, LEN_7BIT_WITH_8BIT_REF, 0]);




doTest("\u6afb", false, [1, LEN_UCS2, LEN_UCS2 - 1]);

doTest("\u6afba", false, [1, LEN_UCS2, LEN_UCS2 - 2]);
doTest("\u6afb\u20ac", false, [1, LEN_UCS2, LEN_UCS2 - 2]);
doTest("\u6afb ", false, [1, LEN_UCS2, LEN_UCS2 - 2]);




doTest(times("\u6afb", LEN_UCS2), false, [1, LEN_UCS2, 0]);


doTest(times("\u6afb", LEN_UCS2 + 1), false,
       [2, LEN_UCS2_WITH_8BIT_REF, LEN_UCS2_WITH_8BIT_REF - 4]);

doTest(times("\u6afb", LEN_UCS2_WITH_8BIT_REF * 2), false,
       [2, LEN_UCS2_WITH_8BIT_REF, 0]);

doTest(times("\u6afb", LEN_UCS2_WITH_8BIT_REF * 2 + 1), false,
       [3, LEN_UCS2_WITH_8BIT_REF, LEN_UCS2_WITH_8BIT_REF - 1]);




doTest("\u0041", true, [1, LEN_7BIT, LEN_7BIT - 1]);

doTest("\u00c0", true, [1, LEN_7BIT, LEN_7BIT - 1]);

doTest("\u00c0\u0041", true, [1, LEN_7BIT, LEN_7BIT - 2]);
doTest("\u0041\u00c0", true, [1, LEN_7BIT, LEN_7BIT - 2]);

doTest("\u1234", true, [1, LEN_7BIT, LEN_7BIT - 1]);

cleanUp();
