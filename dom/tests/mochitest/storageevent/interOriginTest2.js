var frameLoadsPending = 2;

var callMasterFrame = true;
var testDone = false;

var masterFrameOrigin = "";
var slaveFrameOrigin = "";

var failureRegExp = new RegExp("^FAILURE");
var todoRegExp = new RegExp("^TODO");

const framePath = "/tests/dom/tests/mochitest/storageevent/";

window.addEventListener("message", onMessageReceived, false);

function onMessageReceived(event)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");

  switch (event.data)
  {
    
    case "frame loaded":
      if (--frameLoadsPending)
        break;

      

    
    case "perf":
      if (callMasterFrame)
        masterFrame.postMessage("step", masterFrameOrigin);
      else if (slaveFrame)
        slaveFrame.postMessage("step", slaveFrameOrigin);
      else if (masterFrame.slaveFrame)
        masterFrame.slaveFrame.postMessage("step", slaveFrameOrigin);
      callMasterFrame = !callMasterFrame;
      break;

    
    case "done":
      if (testDone)
        break;

      testDone = true;
      SimpleTest.finish();
      break;

    
    default:
      if (typeof event.data == "undefined")
        break; 
               
      if (event.data.match(todoRegExp))
        SimpleTest.todo(false, event.data);
      else
        SimpleTest.ok(!event.data.match(failureRegExp), event.data);
      break;
  }
}
