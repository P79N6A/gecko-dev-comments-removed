




















"use strict";


var qs = require('sdk/querystring');




var qsTestCases = [
  ['foo=918854443121279438895193',
   'foo=918854443121279438895193',
   {'foo': '918854443121279438895193'}],
  ['foo=bar', 'foo=bar', {'foo': 'bar'}],
  
  ['foo=1&bar=2', 'foo=1&bar=2', {'foo': '1', 'bar': '2'}],
  
  
  
  ['foo%3Dbaz=bar', 'foo%3Dbaz=bar', {'foo=baz': 'bar'}],
  ['foo=baz=bar', 'foo=baz%3Dbar', {'foo': 'baz=bar'}],
  







  
  
  ['foo=%EF%BF%BD', 'foo=%EF%BF%BD', {'foo': '\ufffd' }]
];


var qsColonTestCases = [
  ['foo:bar', 'foo:bar', {'foo': 'bar'}],
  
  ['foo:1&bar:2;baz:quux',
   'foo:1%26bar%3A2;baz:quux',
   {'foo': '1&bar:2', 'baz': 'quux'}],
  ['foo%3Abaz:bar', 'foo%3Abaz:bar', {'foo:baz': 'bar'}],
  ['foo:baz:bar', 'foo:baz%3Abar', {'foo': 'baz:bar'}]
];


var extendedFunction = function() {};
extendedFunction.prototype = {a: 'b'};
var qsWeirdObjects = [
  
  
  
  
  
  
  
  
  
  [{f: false, t: true}, 'f=false&t=true', {'f': 'false', 't': 'true'}],
  
  
  
];


var qsNoMungeTestCases = [
  ['', {}],
  
  ['blah=burp', {'blah': 'burp'}],
  
  ['frappucino=muffin&goat%5B%5D=scone&pond=moose',
   {'frappucino': 'muffin', 'goat[]': 'scone', 'pond': 'moose'}],
  ['trololol=yes&lololo=no', {'trololol': 'yes', 'lololo': 'no'}]
];

exports['test basic'] = function(assert) {
  assert.strictEqual('918854443121279438895193',
                   qs.parse('id=918854443121279438895193').id,
                   'prase id=918854443121279438895193');
};

exports['test that the canonical qs is parsed properly'] = function(assert) {
  qsTestCases.forEach(function(testCase) {
    assert.deepEqual(testCase[2], qs.parse(testCase[0]),
                     'parse ' + testCase[0]);
  });
};


exports['test that the colon test cases can do the same'] = function(assert) {
  qsColonTestCases.forEach(function(testCase) {
    assert.deepEqual(testCase[2], qs.parse(testCase[0], ';', ':'),
                     'parse ' + testCase[0] + ' -> ; :');
  });
};

exports['test the weird objects, that they get parsed properly'] = function(assert) {
  qsWeirdObjects.forEach(function(testCase) {
    assert.deepEqual(testCase[2], qs.parse(testCase[1]),
                     'parse ' + testCase[1]);
  });
};

exports['test non munge test cases'] = function(assert) {
  qsNoMungeTestCases.forEach(function(testCase) {
    assert.deepEqual(testCase[0], qs.stringify(testCase[1], '&', '=', false),
                     'stringify ' + JSON.stringify(testCase[1]) + ' -> & =');
  });
};

exports['test the nested qs-in-qs case'] = function(assert) {
  var f = qs.parse('a=b&q=x%3Dy%26y%3Dz');
  f.q = qs.parse(f.q);
  assert.deepEqual(f, { a: 'b', q: { x: 'y', y: 'z' } },
                   'parse a=b&q=x%3Dy%26y%3Dz');
};

exports['test nested in colon'] = function(assert) {
  var f = qs.parse('a:b;q:x%3Ay%3By%3Az', ';', ':');
  f.q = qs.parse(f.q, ';', ':');
  assert.deepEqual(f, { a: 'b', q: { x: 'y', y: 'z' } },
                   'parse a:b;q:x%3Ay%3By%3Az -> ; :');
};

exports['test stringifying'] = function(assert) {
  qsTestCases.forEach(function(testCase) {
    assert.equal(testCase[1], qs.stringify(testCase[2]),
                 'stringify ' + JSON.stringify(testCase[2]));
  });

  qsColonTestCases.forEach(function(testCase) {
    assert.equal(testCase[1], qs.stringify(testCase[2], ';', ':'),
                 'stringify ' + JSON.stringify(testCase[2]) + ' -> ; :');
  });

  qsWeirdObjects.forEach(function(testCase) {
    assert.equal(testCase[1], qs.stringify(testCase[0]),
                 'stringify ' + JSON.stringify(testCase[0]));
  });
};

exports['test stringifying nested'] = function(assert) {
  var f = qs.stringify({
    a: 'b',
    q: qs.stringify({
      x: 'y',
      y: 'z'
    })
  });
  assert.equal(f, 'a=b&q=x%3Dy%26y%3Dz',
               JSON.stringify({
                  a: 'b',
                  'qs.stringify -> q': {
                    x: 'y',
                    y: 'z'
                  }
                }));

  var threw = false;
  try { qs.parse(undefined); } catch(error) { threw = true; }
  assert.ok(!threw, "does not throws on undefined");
};

exports['test nested in colon'] = function(assert) {
  var f = qs.stringify({
    a: 'b',
    q: qs.stringify({
      x: 'y',
      y: 'z'
    }, ';', ':')
  }, ';', ':');
  assert.equal(f, 'a:b;q:x%3Ay%3By%3Az',
               'stringify ' + JSON.stringify({
                  a: 'b',
                  'qs.stringify -> q': {
                    x: 'y',
                    y: 'z'
                  }
                }) + ' -> ; : ');


  assert.deepEqual({}, qs.parse(), 'parse undefined');
};

require('sdk/test').run(exports);
