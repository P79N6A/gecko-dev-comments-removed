var request = new Request("//mochi.test:8888/");
var response;
var c;
var responseText;

function checkResponse(r) {
  ok(r !== response, "The objects should not be the same");
  is(r.url, response.url, "The URLs should be the same");
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
  return caches.open("match-request");
}).then(function(cache) {
  c = cache;
  return c.add(request);
}).then(function() {
  return c.match(request);
}).then(function(r) {
  return checkResponse(r);
}).then(function() {
  return caches.match(request);
}).then(function(r) {
  return checkResponse(r);
}).then(function() {
  return caches.match(request, {cacheName: "match-request"});
}).then(function(r) {
  return checkResponse(r);
}).then(function() {
  return caches.match(request, {cacheName: "foobar"});
}).catch(function(err) {
  is(err.name, "NotFoundError", "Searching in the wrong cache should not succeed");
}).then(function() {
  return caches.delete("match-request");
}).then(function(success) {
  ok(success, "We should be able to delete the cache successfully");
  
  return c.match(request);
}).then(function(r) {
  return checkResponse(r);
}).then(function() {
  
  c = null;
  return caches.open("match-request");
}).then(function(cache) {
  return cache.match(request);
}).catch(function(err) {
  is(err.name, "NotFoundError", "Searching in the cache after deletion should not succeed");
}).then(function() {
  testDone();
});
