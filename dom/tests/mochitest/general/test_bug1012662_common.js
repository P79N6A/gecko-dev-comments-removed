


var WATCH_TIMEOUT = 300;


var gTestN0 = 0, gTestN1 = 0, gTestN2 = 0;
function testLoc() { return " " + gTestN0 + " - " + gTestN1 + " - " + gTestN2; }


var gCopyCount = 0, gCutCount = 0;
document.addEventListener('copy', function() {
  gCopyCount++;
});
document.addEventListener('cut', function() {
  gCutCount++;
});



function selectNode(aSelector, aCb) {
  var dn = document.querySelector(aSelector);
  var range = document.createRange();
  range.selectNodeContents(dn);
  window.getSelection().removeAllRanges();
  window.getSelection().addRange(range);
  if (aCb) {
    aCb();
  }
}

function selectInputNode(aSelector, aCb) {
  var dn = document.querySelector(aSelector);
  synthesizeMouse(dn, 10, 10, {});
  SimpleTest.executeSoon(function() {
    synthesizeKey("A", {accelKey: true});
    SimpleTest.executeSoon(aCb);
  });
}


function execCut(aShouldSucceed) {
  var cb = function(e) {
    e.preventDefault();
    document.removeEventListener('keydown', cb);

    is(aShouldSucceed, document.execCommand('cut'), "Keydown caused cut invocation" + testLoc());
  };
  return cb;
}
function execCopy(aShouldSucceed) {
  var cb = function(e) {
    e.preventDefault();
    document.removeEventListener('keydown', cb);

    is(aShouldSucceed, document.execCommand('copy'), "Keydown caused copy invocation" + testLoc());
  };
  return cb;
}


function cutCopyAll(aDoCut, aDoCopy, aDone, aNegate, aClipOverride, aJustClipboardNegate) {
  var execCommandAlwaysSucceed = !!(aClipOverride || aJustClipboardNegate);

  function waitForClipboard(aCond, aSetup, aNext, aNegateOne) {
    if (aClipOverride) {
      aCond = aClipOverride;
      aNegateOne = false;
    }
    if (aNegate || aNegateOne || aJustClipboardNegate) {
      SimpleTest.waitForClipboard(null, aSetup, aNext, aNext, "text/unicode", WATCH_TIMEOUT, true);
    } else {
      SimpleTest.waitForClipboard(aCond, aSetup, aNext, aNext);
    }
  }

  function validateCutCopy(aExpectedCut, aExpectedCopy) {
    if (aNegate) {
      aExpectedCut = aExpectedCopy = 0;
    } 

    is(aExpectedCut, gCutCount,
       (aExpectedCut > 0 ? "Expect cut callback to run" : "Expect cut callback not to run") + testLoc());
    is(aExpectedCopy, gCopyCount,
       (aExpectedCopy > 0 ? "Expect copy callback to run" : "Expect copy callback not to run") + testLoc());
    gCutCount = gCopyCount = 0;
  }

  function step(n) {
    function nextStep() { step(n + 1); }

    document.querySelector('span').textContent = 'span text';
    document.querySelector('input[type=text]').value = 'text text';
    document.querySelector('input[type=password]').value = 'password text';
    document.querySelector('textarea').value = 'textarea text';

    var contentEditableNode = document.querySelector('div[contentEditable=true]');
    if (contentEditableNode) {
      contentEditableNode.textContent = 'contenteditable text';
    }

    gTestN2 = n;
    switch (n) {
    case 0:
      
      selectNode('span');
      waitForClipboard("span text", function() {
        aDoCopy(true);
      }, nextStep);
      return;

    case 1:
      validateCutCopy(0, 1);

      
      selectNode('span');

      waitForClipboard("span text", function() {
        aDoCut(execCommandAlwaysSucceed);
      }, nextStep, true);
      return;

    case 2:
      validateCutCopy(1, 0);

      
      selectInputNode('input[type=text]', nextStep);
      return;

    case 3:
      waitForClipboard("text text", function() {
        selectInputNode('input[type=text]', function() { aDoCopy(true); });
      }, nextStep);
      return;

    case 4:
      validateCutCopy(0, 1);

      
      selectInputNode('input[type=text]', nextStep);
      return;

    case 5:
      waitForClipboard("text text", function() {
        aDoCut(true);
      }, nextStep);
      return;

    case 6:
      validateCutCopy(1, 0);

      
      selectInputNode('input[type=password]', nextStep);
      return;

    case 7:
      waitForClipboard(null, function() {
        aDoCopy(execCommandAlwaysSucceed);
      }, nextStep, true);
      return;

    case 8:
      validateCutCopy(0, 1);

      
      selectInputNode('input[type=password]', nextStep);
      return;

    case 9:
      waitForClipboard(null, function() {
        aDoCut(execCommandAlwaysSucceed);
      }, nextStep, true);
      return;

    case 10:
      validateCutCopy(1, 0);

      
      selectInputNode('textarea', nextStep);
      return;

    case 11:
      waitForClipboard("textarea text", function() {
        aDoCopy(true);
      }, nextStep);
      return;

    case 12:
      validateCutCopy(0, 1);

      
      selectInputNode('textarea', nextStep);
      return;

    case 13:
      waitForClipboard("textarea text", function() {
        aDoCut(true);
      }, nextStep);
      return;

    case 14:
      validateCutCopy(1, 0);

      
      document.querySelector('textarea').blur();

      waitForClipboard(null, function() {
        aDoCopy(true);
      }, nextStep, true);
      return;

    case 15:
      validateCutCopy(0, 1);

      
      waitForClipboard(null, function() {
        aDoCut(execCommandAlwaysSucceed);
      }, nextStep, true);
      return;

    case 16:
      validateCutCopy(1, 0);

      if (!document.querySelector('div[contentEditable=true]')) {
        
        step(-1);
        return;
      }
      break;

    case 17:
      
      waitForClipboard("contenteditable text", function() {
        selectNode('div[contentEditable=true]', function() {
          aDoCopy(true);
        });
      }, nextStep);
      return;

    case 18:
      validateCutCopy(0, 1);
      break;

    case 19:
      
      waitForClipboard("contenteditable text", function() {
        selectNode('div[contentEditable=true]', function() {
          aDoCut(true);
        });
      }, nextStep);
      return;

    case 20:
      validateCutCopy(1, 0);
      break;

    default:
      aDone();
      return;
    }

    SimpleTest.executeSoon(function() { step(n + 1); });
  }

  step(0);
}

function allMechanisms(aCb, aClipOverride, aNegateAll) {
  function testStep(n) {
    gTestN1 = n;
    switch (n) {
    case 0:
      
      cutCopyAll(function docut(aSucc) {
        synthesizeKey("X", {accelKey: true});
      }, function docopy(aSucc) {
        synthesizeKey("C", {accelKey: true});
      }, function done() { testStep(n + 1); }, false, aClipOverride, aNegateAll);
      return;

    case 1:
      
      cutCopyAll(function docut(aSucc) {
        document.addEventListener('keydown', execCut(aSucc));
        synthesizeKey("Q", {});
      }, function docopy(aSucc) {
        document.addEventListener('keydown', execCopy(aSucc));
        synthesizeKey("Q", {});
      }, function done() { testStep(n + 1); }, false, aClipOverride, aNegateAll);
      return;

    case 2:
      
      cutCopyAll(function doCut(aSucc) {
        is(false, document.execCommand('cut'), "Can't directly execCommand not in a user callback");
      }, function doCopy(aSucc) {
        is(false, document.execCommand('copy'), "Can't directly execCommand not in a user callback");
      }, function done() { testStep(n + 1); }, true, aClipOverride, aNegateAll);
      return;

    default:
      aCb();
      return;
    }

    SimpleTest.executeSoon(function() { testStep(n + 1); });
  }
  testStep(0);
}


SimpleTest.waitForExplicitFinish();
SimpleTest.requestLongerTimeout(5); 
SimpleTest.waitForFocus(function() {
  function justCancel(aEvent) {
    aEvent.preventDefault();
  }

  function override(aEvent) {
    aEvent.clipboardData.setData('text/plain', 'overridden');
    aEvent.preventDefault();
  }

  allMechanisms(function() {
    gTestN0 = 1;
    document.addEventListener('cut', override);
    document.addEventListener('copy', override);

    allMechanisms(function() {
      gTestN0 = 2;
      document.removeEventListener('cut', override);
      document.removeEventListener('copy', override);
      document.addEventListener('cut', justCancel);
      document.addEventListener('copy', justCancel);

      allMechanisms(function() {
        SimpleTest.finish();
      }, null, true);
    }, 'overridden');
  });
});
