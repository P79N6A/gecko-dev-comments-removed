
















function MixedContentTestCase(scenario, description, sanityChecker) {
  var insecureProtocol = "http";
  var secureProtocol = "https";

  var sameOriginHost = location.hostname;
  var crossOriginHost = "{{domains[www1]}}";

  
  var insecurePort = getNormalizedPort(parseInt("{{ports[http][0]}}", 10));
  var securePort = getNormalizedPort(parseInt("{{ports[https][0]}}", 10));

  var resourcePath = "/mixed-content/generic/expect.py";

  
  var endpoint = {
    "same-origin":
      location.origin + resourcePath,
    "same-host-https":
      secureProtocol + "://" + sameOriginHost + securePort + resourcePath,
    "same-host-http":
      insecureProtocol + "://" + sameOriginHost + insecurePort + resourcePath,
    "cross-origin-https":
      secureProtocol + "://" + crossOriginHost + securePort + resourcePath,
    "cross-origin-http":
      insecureProtocol + "://" + crossOriginHost + insecurePort + resourcePath
  };

  
  var resourceMap = {
    "a-tag": requestViaAnchor,
    "area-tag": requestViaArea,
    "fetch-request": requestViaFetch,
    "form-tag": requestViaForm,
    "iframe-tag": requestViaIframe,
    "img-tag":  requestViaImage,
    "script-tag": requestViaScript,
    "worker-request": requestViaWorker,
    "xhr-request": requestViaXhr,
    "audio-tag": requestViaAudio,
    "video-tag": requestViaVideo,
    "picture-tag": requestViaPicture,
    "object-tag": requestViaObject,
    "link-css-tag": requestViaLinkStylesheet,
    "link-prefetch-tag": requestViaLinkPrefetch
  };

  sanityChecker.checkScenario(scenario, resourceMap);

  
  var contentType = {
    "a-tag": "text/html",
    "area-tag": "text/html",
    "fetch-request": "application/json",
    "form-tag": "text/html",
    "iframe-tag": "text/html",
    "img-tag":  "image/png",
    "script-tag": "text/javascript",
    "worker-request": "application/javascript",
    "xhr-request": "application/json",
    "audio-tag": "audio/mpeg",
    "video-tag": "video/mp4",
    "picture-tag": "image/png",
    "object-tag": "text/html",
    "link-css-tag": "text/css",
    "link-prefetch-tag": "text/html"
  };

  var mixed_content_test = async_test(description);

  function runTest() {
    var testCompleted = false;

    
    
    
    
    setTimeout(function() {
      mixed_content_test.step(function() {
        assert_true(testCompleted, "Expected test to complete.");
        mixed_content_test.done();
      })
    }, 1000);

    var key = guid();
    var value = guid();
    var announceResourceRequestUrl = endpoint['same-origin'] +
                                     "?action=put&key=" + key +
                                     "&value=" + value;
    var assertResourceRequestUrl = endpoint['same-origin'] +
                                  "?action=take&key=" + key;
    var resourceRequestUrl = endpoint[scenario.origin] + "?redirection=" +
                             scenario.redirection + "&action=purge&key=" +
                             key + "&content_type=" +
                             contentType[scenario.subresource];

    xhrRequest(announceResourceRequestUrl)
      .then(function(response) {
        
        
        return resourceMap[scenario.subresource](resourceRequestUrl);
      })
      .then(function() {
        mixed_content_test.step(function() {
          assert_equals("allowed", scenario.expectation,
                        "The triggered event should match '" +
                        scenario.expectation + "'.");
        }, "Check if success event was triggered.");

        
        return xhrRequest(assertResourceRequestUrl);
      }, function(error) {
        mixed_content_test.step(function() {
          assert_equals("blocked", scenario.expectation,
                        "The triggered event should match '" +
                        scenario.expectation + "'.");
          
          
          
        }, "Check if error event was triggered.");

        
        return xhrRequest(assertResourceRequestUrl);
      })
      .then(function(response) {
         
         
         mixed_content_test.step(function() {
           assert_equals(response.status, scenario.expectation,
                  "The resource request should be '" + scenario.expectation +
                  "'.");
         }, "Check if request was sent.");
         mixed_content_test.done();
         testCompleted = true;
      });

  }  

  return {start: runTest};
}  
