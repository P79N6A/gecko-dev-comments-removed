








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;





function executeSoon(aFunc)
{
  let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  
  tm.mainThread.dispatch({
    run: function()
    {
      aFunc();
    }
  }, Ci.nsIThread.DISPATCH_NORMAL);
}
