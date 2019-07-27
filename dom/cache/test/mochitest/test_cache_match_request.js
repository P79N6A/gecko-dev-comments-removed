var request = new Request("//mochi.test:8888/?" + context + "#fragment");
var unknownRequest = new Request("//mochi.test:8888/non/existing/path?" + context);
var response;
var c;
var responseText;
var name = "match-request" + context;

function checkResponse(r) {
  ok(r !== response, "The objects should not be the same");
  is(r.url, response.url.replace("#fragment", ""),
     "The URLs should be the same");
  is(r.status, response.status, "The status codes should be the same");
  is(r.type, response.type, "The response types should be the same");
  is(r.ok, response.ok, "Both responses should have succeeded");
  is(r.statusText, response.statusText,
     "Both responses should have the same status text");
  return r.text().then(function(text) {
    is(text, responseText, "The response body should be correct");
  });
}

fetch(new Request(request)).then(function(r) {
  response = r;
  return response.text();
}).then(function(text) {
  responseText = text;
  return testRequest(request, unknownRequest,
                     request.url.replace("#fragment", "#other"));
}).then(function() {
  return testRequest(request.url, unknownRequest.url,
                     request.url.replace("#fragment", "#other"));
}).then(function() {
  testDone();
});


function testRequest(request, unknownRequest, requestWithDifferentFragment) {
  return caches.open(name).then(function(cache) {
    c = cache;
    return c.add(request);
  }).then(function() {
    return Promise.all(
      ["HEAD", "POST", "PUT", "DELETE", "OPTIONS"]
        .map(function(method) {
          var r = new Request(request, {method: method});
          return c.add(r)
            .then(function() {
              ok(false, "Promise should be rejected");
            }, function(err) {
              is(err.name, "TypeError", "Adding a request with type '" + method + "' should fail");
            });
        })
    );
  }).then(function() {
    return c.match(request);
  }).then(function(r) {
    return checkResponse(r);
  }).then(function() {
    return caches.match(request);
  }).then(function(r) {
    return checkResponse(r);
  }).then(function() {
    return caches.match(requestWithDifferentFragment);
  }).then(function(r) {
    return checkResponse(r);
  }).then(function() {
    return caches.match(request, {cacheName: name});
  }).then(function(r) {
    return checkResponse(r);
  }).then(function() {
    return caches.match(request, {cacheName: name + "mambojambo"})
      .then(function() {
        ok(false, "Promise should be rejected");
      }, function(err) {
        is(err.name, "NotFoundError", "Searching in the wrong cache should not succeed");
      });
  }).then(function() {
    return c.match(unknownRequest);
  }).then(function(r) {
    is(typeof r, "undefined", "Searching for an unknown request should not succeed");
    return caches.match(unknownRequest);
  }).then(function(r) {
    is(typeof r, "undefined", "Searching for an unknown request should not succeed");
    return caches.match(unknownRequest, {cacheName: name});
  }).then(function(r) {
    is(typeof r, "undefined", "Searching for an unknown request should not succeed");
    return caches.delete(name);
  }).then(function(success) {
    ok(success, "We should be able to delete the cache successfully");
    
    return c.match(request);
  }).then(function(r) {
    return checkResponse(r);
  }).then(function() {
    
    c = null;
    return caches.open(name);
  }).then(function(cache) {
    return cache.match(request);
  }).then(function(r) {
    is(typeof r, "undefined", "Searching in the cache after deletion should not succeed");
  });
}
