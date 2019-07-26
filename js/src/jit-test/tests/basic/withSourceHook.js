


load(libdir + 'asserts.js');



if (typeof withSourceHook != 'function')
  quit(0);

var log = '';


withSourceHook(function (url) {
  log += 'o';
  assertEq(url, 'outer');
  return '(function outer() { 3; })';
}, function () {
  log += 'O';
  
  assertThrowsValue(function () {
    
    withSourceHook(function (url) {
      log += 'm';
      assertEq(url, 'middle');
      throw 'borborygmus'; 
    }, function () {
      log += 'M';
      
      
      assertEq(withSourceHook(function (url) {
                                log += 'i';
                                assertEq(url, 'inner');
                                return '(function inner() { 1; })';
                              }, function () {
                                log += 'I';
                                return evaluate('(function inner() { 2; })',
                                                { fileName: 'inner', sourcePolicy: 'LAZY_SOURCE' })
                                       .toSource();
                              }),
               '(function inner() { 1; })');
      
      evaluate('(function middle() { })',
               { fileName: 'middle', sourcePolicy: 'LAZY_SOURCE' })
      .toSource();
    });
  }, 'borborygmus');

  
  assertEq(evaluate('(function outer() { 4; })',
                    { fileName: 'outer', sourcePolicy: 'LAZY_SOURCE' })
           .toSource(),
           '(function outer() { 3; })');
});

assertEq(log, 'OMIimo');
