var t = async_test(document.title);

var frameLoadsPending = 2;

var callMasterFrame = true;
var testDone = false;

var masterFrameOrigin = "";
var slaveFrameOrigin = "";

var failureRegExp = new RegExp("^FAILURE");

const framePath = "/tests/dom/tests/mochitest/localstorage/";

window.addEventListener("message", onMessageReceived, false);

function onMessageReceived(event)
{
  switch (event.data)
  {
    
    case "frame loaded":
      if (--frameLoadsPending)
        break;

      

    
    case "perf":
      if (callMasterFrame)
        masterFrame.postMessage("step", masterFrameOrigin);
      else
        slaveFrame.postMessage("step", slaveFrameOrigin);
      callMasterFrame = !callMasterFrame;
      break;

    
    case "done":
      if (testDone)
        break;

      testDone = true;
      t.done();
      break;

    
    default:
      t.step(function() {
        assert_true(!event.data.match(failureRegExp), event.data);
      });
      break;
  }
}
