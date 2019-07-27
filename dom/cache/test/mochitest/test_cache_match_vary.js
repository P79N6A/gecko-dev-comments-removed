var requestURL = "//mochi.test:8888/tests/dom/cache/test/mochitest/vary.sjs?" + context;
var name = "match-vary" + context;

function checkResponse(r, response, responseText) {
  ok(r !== response, "The objects should not be the same");
  is(r.url, response.url.replace("#fragment", ""),
     "The URLs should be the same");
  is(r.status, response.status, "The status codes should be the same");
  is(r.type, response.type, "The response types should be the same");
  is(r.ok, response.ok, "Both responses should have succeeded");
  is(r.statusText, response.statusText,
     "Both responses should have the same status text");
  is(r.headers.get("Vary"), response.headers.get("Vary"),
     "Both responses should have the same Vary header");
  return r.text().then(function(text) {
    is(text, responseText, "The response body should be correct");
  });
}







function setupTest(headers) {
  return setupTestMultipleEntries([headers]).then(function(test) {
    return {response: test.response[0],
            responseText: test.responseText[0],
            cache: test.cache};
  });
}
function setupTestMultipleEntries(headers) {
  ok(Array.isArray(headers), "headers should be an array");
  return new Promise(function(resolve, reject) {
    var response, responseText, cache;
    Promise.all(headers.map(function(h) {
      return fetch(new Request(requestURL, {headers: h}));
    })).then(function(r) {
        response = r;
        return Promise.all(response.map(function(r) {
          return r.text();
        }));
      }).then(function(text) {
        responseText = text;
        return caches.open(name);
      }).then(function(c) {
        cache = c;
        return Promise.all(headers.map(function(h) {
          return c.add(new Request(requestURL, {headers: h}));
        }));
      }).then(function() {
        resolve({response: response, responseText: responseText, cache: cache});
      }).catch(function(err) {
        reject(err);
      });
  });
}

function testBasics() {
  var test;
  return setupTest({"WhatToVary": "Cookie"})
    .then(function(t) {
      test = t;
      
      return test.cache.match(requestURL);
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    }).then(function() {
      
      return test.cache.match(new Request(requestURL, {headers: {"Cookie": "foo=bar"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with an unknown Vary header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"Cookie": "foo=bar"}}),
                              {ignoreVary: true});
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    });
}

function testBasicKeys() {
  function checkRequest(reqs) {
    is(reqs.length, 1, "One request expected");
    ok(reqs[0].url.indexOf(requestURL) >= 0, "The correct request expected");
    ok(reqs[0].headers.get("WhatToVary"), "Cookie", "The correct request headers expected");
  }
  var test;
  return setupTest({"WhatToVary": "Cookie"})
    .then(function(t) {
      test = t;
      
      return test.cache.keys(requestURL);
    }).then(function(r) {
      return checkRequest(r);
    }).then(function() {
      
      return test.cache.keys(new Request(requestURL, {headers: {"Cookie": "foo=bar"}}));
    }).then(function(r) {
      is(r.length, 0, "Searching for a request with an unknown Vary header should not succeed");
      
      return test.cache.keys(new Request(requestURL, {headers: {"Cookie": "foo=bar"}}),
                             {ignoreVary: true});
    }).then(function(r) {
      return checkRequest(r);
    });
}

function testStar() {
  var test;
  return setupTest({"WhatToVary": "*", "Cookie": "foo=bar"})
    .then(function(t) {
      test = t;
      
      return test.cache.match(new Request(requestURL, {headers: {"Cookie": "bar=baz"}}));
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    });
}

function testMatch() {
  var test;
  return setupTest({"WhatToVary": "Cookie", "Cookie": "foo=bar"})
    .then(function(t) {
      test = t;
      
      return test.cache.match(new Request(requestURL, {headers: {"Cookie": "bar=baz"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with a non-matching Cookie header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"Cookie": "foo=bar"}}));
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    });
}

function testStarAndAnotherHeader() {
  var test;
  return setupTest({"WhatToVary": "*,User-Agent"})
    .then(function(t) {
      test = t;
      
      return test.cache.match(new Request(requestURL, {headers: {"User-Agent": "MyUA"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with a non-matching User-Agent header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"User-Agent": "MyUA"}}),
                              {ignoreVary: true});
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    });
}

function testInvalidHeaderName() {
  var test;
  return setupTest({"WhatToVary": "Foo/Bar, User-Agent"})
    .then(function(t) {
      test = t;
      
      return test.cache.match(new Request(requestURL, {headers: {"User-Agent": "MyUA"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with a non-matching User-Agent header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"User-Agent": "MyUA"}}),
                              {ignoreVary: true});
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    }).then(function() {
      
      return test.cache.match(new Request(requestURL, {headers: {"Foo": "foobar"}}));
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    });
}

function testMultipleHeaders() {
  var test;
  return setupTest({"WhatToVary": "Referer,\tAccept-Encoding"})
    .then(function(t) {
      test = t;
      
      return test.cache.match(new Request(requestURL, {headers: {"Referer": "https://somesite.com/"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with a non-matching Referer header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"Referer": "https://somesite.com/"}}),
                              {ignoreVary: true});
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    }).then(function() {
      
      return test.cache.match(new Request(requestURL, {headers: {"Accept-Encoding": "myencoding"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with a non-matching Accept-Encoding header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"Accept-Encoding": "myencoding"}}),
                              {ignoreVary: true});
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    }).then(function() {
      
      return test.cache.match(new Request(requestURL, {headers: {"Referer": ""}}));
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    }).then(function() {
      
      return test.cache.match(new Request(requestURL, {headers: {"Accept-Encoding": ""}}));
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    }).then(function() {
      
      return test.cache.match(new Request(requestURL, {headers: {"Referer": "",
                                                                 "Accept-Encoding": "myencoding"}}));
    }).then(function(r) {
      is(typeof r, "undefined", "Searching for a request with a non-matching Accept-Encoding header should not succeed");
      
      return test.cache.match(new Request(requestURL, {headers: {"Referer": "",
                                                                 "Accept-Encoding": "myencoding"}}),
                              {ignoreVary: true});
    }).then(function(r) {
      return checkResponse(r, test.response, test.responseText);
    });
}

function testMultipleCacheEntries() {
  var test;
  return setupTestMultipleEntries([
    {"WhatToVary": "Accept-Language", "Accept-Language": "en-US"},
    {"WhatToVary": "Accept-Language", "Accept-Language": "en-US, fa-IR"},
  ]).then(function(t) {
    test = t;
    return test.cache.matchAll();
  }).then(function (r) {
    is(r.length, 2, "Two cache entries should be stored in the DB");
    
    return test.cache.matchAll(requestURL);
  }).then(function(r) {
    is(r.length, 0, "Searching for a request without specifying an Accept-Language header should not succeed");
    
    return test.cache.matchAll(requestURL, {ignoreVary: true});
  }).then(function(r) {
    return Promise.all([
      checkResponse(r[0], test.response[0], test.responseText[0]),
      checkResponse(r[1], test.response[1], test.responseText[1]),
    ]);
  }).then(function() {
    
    return test.cache.matchAll(new Request(requestURL, {headers: {"Accept-Language": "en-US"}}));
  }).then(function(r) {
    is(r.length, 1, "One cache entry should be found");
    return checkResponse(r[0], test.response[0], test.responseText[0]);
  }).then(function() {
    
    return test.cache.matchAll(new Request(requestURL, {headers: {"Accept-Language": "en-US, fa-IR"}}));
  }).then(function(r) {
    is(r.length, 1, "One cache entry should be found");
    return checkResponse(r[0], test.response[1], test.responseText[1]);
  }).then(function() {
    
    return test.cache.matchAll(new Request(requestURL, {headers: {"Accept-Language": "en-US"}}),
                               {ignoreVary: true});
  }).then(function(r) {
    return Promise.all([
      checkResponse(r[0], test.response[0], test.responseText[0]),
      checkResponse(r[1], test.response[1], test.responseText[1]),
    ]);
  }).then(function() {
    
    return test.cache.matchAll(new Request(requestURL, {headers: {"Accept-Language": "fa-IR"}}));
  }).then(function(r) {
    is(r.length, 0, "Searching for a request with a different Accept-Language header should not succeed");
    
    return test.cache.matchAll(new Request(requestURL, {headers: {"Accept-Language": "fa-IR"}}),
                               {ignoreVary: true});
  }).then(function(r) {
    is(r.length, 2, "Two cache entries should be found");
    return Promise.all([
      checkResponse(r[0], test.response[0], test.responseText[0]),
      checkResponse(r[1], test.response[1], test.responseText[1]),
    ]);
  });
}


function step(testPromise) {
  return testPromise.then(function() {
    caches.delete(name);
  });
}

step(testBasics()).then(function() {
  return step(testBasicKeys());
}).then(function() {
  return step(testStar());
}).then(function() {
  return step(testMatch());
}).then(function() {
  return step(testStarAndAnotherHeader());
}).then(function() {
  return step(testInvalidHeaderName());
}).then(function() {
  return step(testMultipleHeaders());
}).then(function() {
  return step(testMultipleCacheEntries());
}).then(function() {
  testDone();
});
