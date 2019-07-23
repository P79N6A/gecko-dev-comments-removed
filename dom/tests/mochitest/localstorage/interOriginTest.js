var slaveLoadsPending = 1;

var slaveOrigin = "";
var slave = null;

var failureRegExp = new RegExp("^FAILURE");
const slavePath = "/tests/dom/tests/mochitest/localstorage/";

window.addEventListener("message", onMessageReceived, false);

function onMessageReceived(event)
{
  switch (event.data)
  {
    
    case "frame loaded":
      if (--slaveLoadsPending)
        break;

      

    
    case "perf":
      if (event.data == "perf")
        doStep();

      slave.postMessage("step", slaveOrigin);
      break;

    
    case "done":
      localStorage.clear();
      slaveLoadsPending = 1;
      doNextTest();
      break;

    
    default:
      SimpleTest.ok(!event.data.match(failureRegExp), event.data);
      break;
  }
}
