






"use strict";

function run_test()
{
  let {EventLoopLagFront} = devtools.require("devtools/server/actors/eventlooplag");

  DebuggerServer.init();
  DebuggerServer.addBrowserActors();

  
  let threshold = 20;
  let interval = 10;


  let front;
  let client = new DebuggerClient(DebuggerServer.connectPipe());

  
  client.connect(function () {
    client.listTabs(function(resp) {
      front = new EventLoopLagFront(client, resp);
      front.start().then(success => {
        do_check_true(success);
        front.once("event-loop-lag", gotLagEvent);
        do_execute_soon(lag);
      });
    });
  });

  
  function lag() {
    let start = new Date();
    let duration = threshold + interval + 1;
    while (true) {
      if (((new Date()) - start) > duration) {
        break;
      }
    }
  }

  
  
  function gotLagEvent(time) {
    do_print("lag: " + time);
    do_check_true(time >= threshold);
    front.stop().then(() => {
      finishClient(client);
    });
  }

  do_test_pending();
}
