



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




function checkIndividualResults(testname, expected) {
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




function checkExpectedGlobalResults() {
  var url = 'bug704320.sjs?action=get-test-results';
  doXHR(url,
	function(xhr) {
	      var response = JSON.parse(xhr.response);

	      for (type in response) {
		for (scheme in response[type]) {
		  for (policy in response[type][scheme]) {
		    var expectedResult = EXPECTED_RESULTS[type] === undefined ?
		      	EXPECTED_RESULTS['default'][scheme][policy] :
		      	EXPECTED_RESULTS[type][scheme][policy];
		    is(response[type][scheme][policy], expectedResult, type + ' ' + scheme + ' ' + policy);
		  }
		}
	      }
		advance();
	},
	function(xhr) {
          	ok(false, "Can't get results from the counter server.");
		SimpleTest.finish();
	});
}


var EXPECTED_RESULTS = {
  
  
  
  
  'link-ping': {
    
    'http-to-http': {
      'no-referrer': '',
      'unsafe-url': '',
      'origin': '',
      'origin-when-cross-origin': '',
      'no-referrer-when-downgrade': ''
    },
    'http-to-https': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=https&policy=unsafe-url',
      'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=https&policy=no-referrer-when-downgrade'
    },
    
    'https-to-http': {
      'no-referrer': '',
      'unsafe-url': '',
      'origin': '',
      'origin-when-cross-origin': '',
      'no-referrer-when-downgrade': ''
    },
    
    'https-to-https': {
      'no-referrer': '',
      'unsafe-url': '',
      'origin': '',
      'origin-when-cross-origin': '',
      'no-referrer-when-downgrade': ''
    }
  },
  
  'form': {
    'http-to-http': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=http&policy=unsafe-url&type=form',
      'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=http&policy=origin-when-cross-origin&type=form',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=http&policy=no-referrer-when-downgrade&type=form'
    },
    'http-to-https': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=https&policy=unsafe-url&type=form',
      'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=https&policy=no-referrer-when-downgrade&type=form'
    },
    'https-to-http': {
      'no-referrer': '',
      'unsafe-url': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=http&policy=unsafe-url&type=form',
      'origin': 'https://example.com',
      'origin-when-cross-origin': 'https://example.com',
      'no-referrer-when-downgrade': ''
    },
    'https-to-https': {
      'no-referrer': '',
      'unsafe-url': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=https&policy=unsafe-url&type=form',
      'origin': 'https://example.com',
     'origin-when-cross-origin': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=https&policy=origin-when-cross-origin&type=form',
      'no-referrer-when-downgrade': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=https&policy=no-referrer-when-downgrade&type=form'
    }
  },
  
  'window.location': {
    'http-to-http': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=http&policy=unsafe-url&type=window.location',
     'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=http&policy=origin-when-cross-origin&type=window.location',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=http&policy=no-referrer-when-downgrade&type=window.location'
    },
    'http-to-https': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=https&policy=unsafe-url&type=window.location',
      'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=http&scheme-to=https&policy=no-referrer-when-downgrade&type=window.location'
    },
    'https-to-http': {
      'no-referrer': '',
      'unsafe-url': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=http&policy=unsafe-url&type=window.location',
      'origin': 'https://example.com',
      'origin-when-cross-origin': 'https://example.com',
      'no-referrer-when-downgrade': ''
    },
    'https-to-https': {
      'no-referrer': '',
      'unsafe-url': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=https&policy=unsafe-url&type=window.location',
      'origin': 'https://example.com',
      'origin-when-cross-origin': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=https&policy=origin-when-cross-origin&type=window.location',
      'no-referrer-when-downgrade': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-2nd-level-iframe&scheme-from=https&scheme-to=https&policy=no-referrer-when-downgrade&type=window.location'
    }
  },
  'default': {
    'http-to-http': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=http&policy=unsafe-url',
      'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=http&policy=origin-when-cross-origin',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=http&policy=no-referrer-when-downgrade'
    },
    'http-to-https': {
      'no-referrer': '',
      'unsafe-url': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=https&policy=unsafe-url',
      'origin': 'http://example.com',
      'origin-when-cross-origin': 'http://example.com',
      'no-referrer-when-downgrade': 'http://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=http&scheme-to=https&policy=no-referrer-when-downgrade'
    },
    'https-to-http': {
      'no-referrer': '',
      'unsafe-url': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=https&scheme-to=http&policy=unsafe-url',
      'origin': 'https://example.com',
      'origin-when-cross-origin': 'https://example.com',
      'no-referrer-when-downgrade': ''
    },
    'https-to-https': {
      'no-referrer': '',
      'unsafe-url': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=https&scheme-to=https&policy=unsafe-url',
      'origin': 'https://example.com',
      'origin-when-cross-origin': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=https&scheme-to=https&policy=origin-when-cross-origin',
      'no-referrer-when-downgrade': 'https://example.com/tests/dom/base/test/bug704320.sjs?action=create-1st-level-iframe&scheme-from=https&scheme-to=https&policy=no-referrer-when-downgrade'
    }
  }
};
