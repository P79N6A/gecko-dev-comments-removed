



XPCOMUtils.defineLazyModuleGetter(this, "Chat",
                                  "resource:///modules/Chat.jsm");

let openChatOrig = Chat.open;

const firstCallId = 4444333221;
const secondCallId = 1001100101;

function test_send_busy_on_call() {
  let actionReceived = false;

  let msgHandler = function(msg) {
    if (msg.messageType &&
        msg.messageType === "action" &&
        msg.event === "terminate" &&
        msg.reason === "busy") {
      actionReceived = true;
    }
  };

  let mockWebSocket = new MockWebSocketChannel({defaultMsgHandler: msgHandler});
  Services.io.offline = false;

  MozLoopService.register(mockPushHandler, mockWebSocket).then(() => {
    let opened = 0;
    Chat.open = function() {
      opened++;
    };

    mockPushHandler.notify(1);

    waitForCondition(() => {return actionReceived && opened > 0}).then(() => {
      do_check_true(opened === 1, "should open only one chat window");
      do_check_true(actionReceived, "should respond with busy/reject to second call");
      MozLoopService.releaseCallData(firstCallId);
      run_next_test();
    }, () => {
      do_throw("should have opened a chat window for first call and rejected second call");
    });

  });
}

add_test(test_send_busy_on_call); 
add_test(test_send_busy_on_call); 

function run_test()
{
  setupFakeLoopServer();

  
  
  
  

  let callsRespCount = 0;
  let callsResponses = [
    {calls: [{callId: firstCallId,
              websocketToken: "0deadbeef0",
              progressURL: "wss://localhost:5000/websocket"}]},
    {calls: [{callId: secondCallId,
              websocketToken: "1deadbeef1",
              progressURL: "wss://localhost:5000/websocket"}]},

    {calls: []},
    {calls: [{callId: firstCallId,
              websocketToken: "0deadbeef0",
              progressURL: "wss://localhost:5000/websocket"},
             {callId: secondCallId,
              websocketToken: "1deadbeef1",
              progressURL: "wss://localhost:5000/websocket"}]},
  ];

  loopServer.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });

  loopServer.registerPathHandler("/calls", (request, response) => {
    response.setStatusLine(null, 200, "OK");

    if (callsRespCount >= callsResponses.length) {
      callsRespCount = 0;
    }

    response.write(JSON.stringify(callsResponses[callsRespCount++]));
    response.processAsync();
    response.finish();
  });

  do_register_cleanup(function() {
    
    Chat.open = openChatOrig;

    
    Services.prefs.clearUserPref("loop.seenToS");
  });

  run_next_test();
}
