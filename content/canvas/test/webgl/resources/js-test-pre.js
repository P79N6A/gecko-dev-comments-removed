

























function initNonKhronosFramework(waitUntilDone) {
  if (window.layoutTestController) {
    layoutTestController.overridePreference("WebKitWebGLEnabled", "1");
    layoutTestController.dumpAsText();
    if (waitUntilDone) {
      layoutTestController.waitUntilDone();
    }
  }
}

function nonKhronosFrameworkNotifyDone() {
  if (window.layoutTestController) {
    layoutTestController.notifyDone();
  }
}

function reportTestResultsToHarness(success, msg) {
  if (window.parent.webglTestHarness) {
    window.parent.webglTestHarness.reportResults(success, msg);
  }
}

function notifyFinishedToHarness() {
  if (window.parent.webglTestHarness) {
    window.parent.webglTestHarness.notifyFinished();
  }
}

function description(msg)
{
    
    var span = document.createElement("span");
    span.innerHTML = '<p>' + msg + '</p><p>On success, you will see a series of "<span class="pass">PASS</span>" messages, followed by "<span class="pass">TEST COMPLETE</span>".</p>';
    var description = document.getElementById("description");
    if (description.firstChild)
        description.replaceChild(span, description.firstChild);
    else
        description.appendChild(span);
}

function debug(msg)
{
    var span = document.createElement("span");
    document.getElementById("console").appendChild(span); 
    span.innerHTML = msg + '<br />';
}

function escapeHTML(text)
{
    return text.replace(/&/g, "&amp;").replace(/</g, "&lt;");
}

function testPassed(msg)
{
    reportTestResultsToHarness(true, msg);
    debug('<span><span class="pass">PASS</span> ' + escapeHTML(msg) + '</span>');
}

function testFailed(msg)
{
    reportTestResultsToHarness(false, msg);
    debug('<span><span class="fail">FAIL</span> ' + escapeHTML(msg) + '</span>');
}

function areArraysEqual(_a, _b)
{
    if (_a.length !== _b.length)
        return false;
    for (var i = 0; i < _a.length; i++)
        if (_a[i] !== _b[i])
            return false;
    return true;
}

function isMinusZero(n)
{
    
    
    return n === 0 && 1/n < 0;
}

function isResultCorrect(_actual, _expected)
{
    if (_expected === 0)
        return _actual === _expected && (1/_actual) === (1/_expected);
    if (_actual === _expected)
        return true;
    if (typeof(_expected) == "number" && isNaN(_expected))
        return typeof(_actual) == "number" && isNaN(_actual);
    if (Object.prototype.toString.call(_expected) == Object.prototype.toString.call([]))
        return areArraysEqual(_actual, _expected);
    return false;
}

function stringify(v)
{
    if (v === 0 && 1/v < 0)
        return "-0";
    else return "" + v;
}

function evalAndLog(_a)
{
  if (typeof _a != "string")
    debug("WARN: tryAndLog() expects a string argument");

  
  debug(_a);

  var _av;
  try {
     _av = eval(_a);
  } catch (e) {
    testFailed(_a + " threw exception " + e);
  }
}

function shouldBe(_a, _b)
{
  if (typeof _a != "string" || typeof _b != "string")
    debug("WARN: shouldBe() expects string arguments");
  var exception;
  var _av;
  try {
     _av = eval(_a);
  } catch (e) {
     exception = e;
  }
  var _bv = eval(_b);

  if (exception)
    testFailed(_a + " should be " + _bv + ". Threw exception " + exception);
  else if (isResultCorrect(_av, _bv))
    testPassed(_a + " is " + _b);
  else if (typeof(_av) == typeof(_bv))
    testFailed(_a + " should be " + _bv + ". Was " + stringify(_av) + ".");
  else
    testFailed(_a + " should be " + _bv + " (of type " + typeof _bv + "). Was " + _av + " (of type " + typeof _av + ").");
}

function shouldBeTrue(_a) { shouldBe(_a, "true"); }
function shouldBeFalse(_a) { shouldBe(_a, "false"); }
function shouldBeNaN(_a) { shouldBe(_a, "NaN"); }
function shouldBeNull(_a) { shouldBe(_a, "null"); }

function shouldBeEqualToString(a, b)
{
  var unevaledString = '"' + b.replace(/"/g, "\"") + '"';
  shouldBe(a, unevaledString);
}

function shouldEvaluateTo(actual, expected) {
  
  
  
  if (expected == null) {
    
    shouldBeNull(actual);
  } else if (typeof expected == "undefined") {
    shouldBeUndefined(actual);
  } else if (typeof expected == "function") {
    
    try {
      actualValue = eval(actual);
    } catch (e) {
      testFailed("Evaluating " + actual + ": Threw exception " + e);
      return;
    }
    shouldBe("'" + actualValue.toString().replace(/\n/g, "") + "'",
             "'" + expected.toString().replace(/\n/g, "") + "'");
  } else if (typeof expected == "object") {
    shouldBeTrue(actual + " == '" + expected + "'");
  } else if (typeof expected == "string") {
    shouldBe(actual, expected);
  } else if (typeof expected == "boolean") {
    shouldBe("typeof " + actual, "'boolean'");
    if (expected)
      shouldBeTrue(actual);
    else
      shouldBeFalse(actual);
  } else if (typeof expected == "number") {
    shouldBe(actual, stringify(expected));
  } else {
    debug(expected + " is unknown type " + typeof expected);
    shouldBeTrue(actual, "'"  +expected.toString() + "'");
  }
}

function shouldBeNonZero(_a)
{
  var exception;
  var _av;
  try {
     _av = eval(_a);
  } catch (e) {
     exception = e;
  }

  if (exception)
    testFailed(_a + " should be non-zero. Threw exception " + exception);
  else if (_av != 0)
    testPassed(_a + " is non-zero.");
  else
    testFailed(_a + " should be non-zero. Was " + _av);
}

function shouldBeNonNull(_a)
{
  var exception;
  var _av;
  try {
     _av = eval(_a);
  } catch (e) {
     exception = e;
  }

  if (exception)
    testFailed(_a + " should be non-null. Threw exception " + exception);
  else if (_av != null)
    testPassed(_a + " is non-null.");
  else
    testFailed(_a + " should be non-null. Was " + _av);
}

function shouldBeUndefined(_a)
{
  var exception;
  var _av;
  try {
     _av = eval(_a);
  } catch (e) {
     exception = e;
  }

  if (exception)
    testFailed(_a + " should be undefined. Threw exception " + exception);
  else if (typeof _av == "undefined")
    testPassed(_a + " is undefined.");
  else
    testFailed(_a + " should be undefined. Was " + _av);
}


function shouldThrow(_a, _e)
{
  var exception;
  var _av;
  try {
     _av = eval(_a);
  } catch (e) {
     exception = e;
  }

  var _ev;
  if (_e)
      _ev =  eval(_e);

  if (exception) {
    if (typeof _e == "undefined" || exception == _ev)
      testPassed(_a + " threw exception " + exception + ".");
    else
      testFailed(_a + " should throw " + (typeof _e == "undefined" ? "an exception" : _ev) + ". Threw exception " + exception + ".");
  } else if (typeof _av == "undefined")
    testFailed(_a + " should throw " + (typeof _e == "undefined" ? "an exception" : _ev) + ". Was undefined.");
  else
    testFailed(_a + " should throw " + (typeof _e == "undefined" ? "an exception" : _ev) + ". Was " + _av + ".");
}

function assertMsg(assertion, msg) {
    if (assertion) {
        testPassed(msg);
    } else {
        testFailed(msg);
    }
}

function gc() {
    if (typeof GCController !== "undefined")
        GCController.collect();
    else {
        function gcRec(n) {
            if (n < 1)
                return {};
            var temp = {i: "ab" + i + (i / 100000)};
            temp += "foo";
            gcRec(n-1);
        }
        for (var i = 0; i < 1000; i++)
            gcRec(10)
    }
}
