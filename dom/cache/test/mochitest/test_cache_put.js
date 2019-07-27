var url = 'test_cache.js';
var cache;
var fetchResponse;
Promise.all([fetch(url),
             caches.open('putter' + context)]).then(function(results) {
  fetchResponse = results[0];
  cache = results[1];
  return cache.put(url, fetchResponse.clone());
}).then(function(result) {
  is(undefined, result, 'Successful put() should resolve undefined');
  return cache.match(url);
}).then(function(response) {
  ok(response, 'match() should find resppnse that was previously put()');
  ok(response.url.endsWith(url), 'matched response should match original url');
  return Promise.all([fetchResponse.text(),
                      response.text()]);
}).then(function(results) {
  
  if (results[0] !== results[1]) {
    is(results[0], results[1], 'stored response body should match original');
  }
  return cache.delete('putter' + context);
}).then(function() {
  testDone();
});
