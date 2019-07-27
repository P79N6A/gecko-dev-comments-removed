


const GET_RESULT = sjs + 'action=get-test-results';
const RESET_STATE = sjs + 'action=resetState';

SimpleTest.waitForExplicitFinish();
var advance = function() { tests.next(); };





window.addEventListener("message", function(event) {
  if (event.data == "childLoadComplete") {
    
    advance();
  }
});





function doXHR(aUrl, onSuccess, onFail) {
  var xhr = new XMLHttpRequest();
  xhr.responseType = "json";
  xhr.onload = function () {
    onSuccess(xhr);
  };
  xhr.onerror = function () {
    onFail(xhr);
  };
  xhr.open('GET', aUrl, true);
  xhr.send(null);
}




function checkIndividualResults(aTestname, aExpectedReferrer, aName) {
  var onload = xhr => {
    var results = xhr.response;
    info(JSON.stringify(xhr.response));
    ok(aName in results, aName + " tests have to be performed.");
    is(results[aName].policy, aExpectedReferrer, aTestname + ' --- ' + results[aName].policy + ' (' + results[aName].referrer + ')');
    advance();
  };
  var onerror = xhr => {
    ok(false, "Can't get results from the counter server.");
    SimpleTest.finish();
  };
  doXHR(GET_RESULT, onload, onerror);
}

function resetState() {
  doXHR(RESET_STATE,
    advance,
    function(xhr) {
      ok(false, "error in reset state");
      SimpleTest.finish();
    });
}




var tests = (function() {

  
  yield SpecialPowers.pushPrefEnv({"set": [['network.http.enablePerElementReferrer', true]]}, advance);

  var iframe = document.getElementById("testframe");

  for (var j = 0; j < testCases.length; j++) {
    var actions = testCases[j].ACTION;
    var tests = testCases[j].TESTS;
    for (var k = 0; k < actions.length; k++) {
      var actionString = actions[k];
      for (var i = 0; i < tests.length; i++) {
        yield resetState();
        var searchParams = new URLSearchParams();
        searchParams.append(ACTION, actionString);
        searchParams.append(NAME, tests[i].NAME);
        for (var l of PARAMS) {
          if (tests[i][l]) {
            searchParams.append(window[l], tests[i][l]);
          }
        }
        yield iframe.src = sjs + searchParams.toString();
        yield checkIndividualResults(tests[i].DESC, tests[i].RESULT, tests[i].NAME);
      };
    };
  };

  
  yield SimpleTest.finish();
})();