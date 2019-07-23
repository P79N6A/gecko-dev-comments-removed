




































var gTestfile = 'regress-348986.js';

var BUGNUMBER = 348986;
var summary = 'Recursion check of nested functions';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 



  var deepestFunction;

  var n = findActionMax(function(n) {
			  var prefix="function f(){";
			  var suffix="}";
			  var source = Array(n+1).join(prefix) + Array(n+1).join(suffix);
			  try {
			    deepestFunction = Function(source);
			    return true;
			  } catch (e) {
			    if (!(e instanceof InternalError))
			      throw e;
			    return false;
			  }

			});

  if (n == 0)
    throw "unexpected";

  print("Max nested function leveles:"+n);

  n = findActionMax(function(n) {
		      try {
			callAfterConsumingCStack(n, function() {});
			return true;
		      } catch (e) {
			if (!(e instanceof InternalError))
			  throw e;
			return false;
		      }
		    });

  print("Max callAfterConsumingCStack levels:"+n);




 
  n = Math.max(0, n - 10);
  try {
    var src = callAfterConsumingCStack(n, function() {
					 return deepestFunction.toSource();
				       });
    throw "Test failed to hit the recursion limit.";
  } catch (e) {
    if (!(e instanceof InternalError))
      throw e;
  }

  print('Done');
  expect = true;
  actual = true;
  reportCompare(expect, true, summary);

  exitFunc ('test');
}

function callAfterConsumingCStack(n, action)
{
  var testObj = { 
    get propertyWithGetter() {
      if (n == 0)
	return action();
      n--;
      return this.propertyWithGetter;
    }
  };
  return testObj.propertyWithGetter;
}




function findActionMax(action)
{
  var N, next, increase;

  n = 0;
  for (;;) {
    var next = (n == 0 ? 1 : n * 2);
    if (!isFinite(next) || !action(next))
      break;
    n = next;
  }
  if (n == 0)
    return 0;
	
  var increase = n / 2;
  for (;;) {
    var next = n + increase;
    if (next == n)
      break;
    if (isFinite(next) && action(next)) {
      n = next;
    } else if (increase == 1) {
      break;
    } else {
      increase = increase / 2;
    }	
  }
  return n;
}
