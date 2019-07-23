




































var bug = 340199;
var summary = 'User-defined __iterator__ can be called through XPCNativeWrappers';
var actual = 'Not called';
var expect = 'Not called';

printBugNumber (bug);
printStatus (summary);

if (typeof window != 'undefined' &&
    typeof XPCNativeWrapper != 'undefined')
{
  Object.prototype.__iterator__ = 
    function () { actual = "User code called"; print(actual); };

  try
  {
    for (var i in XPCNativeWrapper(window)) 
    {
      try 
      { 
        print(i);
      } 
      catch(ex)
      {
        print(ex);
      }
    }
  }
  catch(ex)
  {
  }
}
  
reportCompare(expect, actual, summary);
