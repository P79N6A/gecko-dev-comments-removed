



window.addEventListener("message", function(event) {
    if (event.data == "childLoadComplete") {
      
      advance();
    } else if (event.data == "childOverload") {
      
      ok(false, "Too many load handlers called in test.");
      SimpleTest.finish();
    } else if (event.data.indexOf("fail-") == 0) {
      
      ok(false, "Child failed the test with error " + event.data.substr(5));
      SimpleTest.finish();
    }});






function doXHR(url, onSuccess, onFail) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (xhr.status == 200) {
      onSuccess(xhr);
    } else {
      onFail(xhr);
    }
  };
  xhr.open('GET', url, true);
  xhr.send(null);
}






function resetCounter() {
  doXHR('/tests/dom/base/test/bug704320_counter.sjs?reset',
        advance,
        function(xhr) {
          ok(false, "Need to be able to reset the request counter");
          SimpleTest.finish();
        });
}




function checkResults(testname, expected) {
  doXHR('/tests/dom/base/test/bug704320_counter.sjs?results',
        function(xhr) {
          var results = JSON.parse(xhr.responseText);
          info(xhr.responseText);

          ok('img' in results,
              testname + " test: some image loads required in results object.");
          is(results['img'].count, 2,
            testname + " Test: Expected 2 loads for image requests.");

          expected.forEach(function (ref) {
            ok(results['img'].referrers.indexOf(ref) >= 0,
                testname + " Test: Expected " + ref + " referrer policy in test, results were " + 
                JSON.stringify(results['img'].referrers) +".");
            });
          advance();
        },
        function(xhr) {
          ok(false, "Can't get results from the counter server.");
          SimpleTest.finish();
        });
}
