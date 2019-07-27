


MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = 'head.js';












function createMessages(aMessages) {
  let promise = Promise.resolve();
  aMessages.forEach((aMessage) => {
    promise = promise.then((aMessage.incoming) ?
      () => sendTextSmsToEmulatorAndWait(aMessage.address, aMessage.text) :
      () => sendSmsWithSuccess(aMessage.address, aMessage.text));
  });

  return promise;
}

function checkThreads(aMessages, aNotMerged) {
  return getAllThreads().then((aThreads) => {
    let threadCount = aThreads.length;

    if (aNotMerged) {
      
      aThreads.reverse();
      is(threadCount, aMessages.length, "Number of Threads.");
      for (let i = 0; i < threadCount; i++) {
        let thread = aThreads[i];
        let message = aMessages[i];
        is(thread.unreadCount, message.incoming ? 1 : 0, "Unread Count.");
        is(thread.participants.length, 1, "Number of Participants.");
        is(thread.participants[0], message.address, "Participants.");
        is(thread.body, message.text, "Thread Body.");
      }

      return;
    }

    let lastBody = aMessages[aMessages.length - 1].text;
    let unreadCount = 0;
    let mergedThread = aThreads[0];
    aMessages.forEach((aMessage) => {
      if (aMessage.incoming) {
        unreadCount++;
      }
    });
    is(threadCount, 1, "Number of Threads.");
    is(mergedThread.unreadCount, unreadCount, "Unread Count.");
    is(mergedThread.participants.length, 1, "Number of Participants.");
    is(mergedThread.participants[0], aMessages[0].address, "Participants.");
    
    
    
    
    is(mergedThread.body, lastBody, "Thread Body.");
  });
}

function testGetThreads(aMessages, aNotMerged) {
  aNotMerged = !!aNotMerged;
  log("aMessages: " + JSON.stringify(aMessages));
  log("aNotMerged: " + aNotMerged);
  return createMessages(aMessages)
    .then(() => checkThreads(aMessages, aNotMerged))
    .then(() => deleteAllMessages());
}

startTestCommon(function testCaseMain() {
  
  
  
  
  return testGetThreads([{ incoming: false, address: "5555211001", text: "thread 1" }])
  
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "5555211002", text: "thread 2-1" },
                                { incoming: false, address: "+15555211002", text: "thread 2-2" }]))
  
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "+15555211003", text: "thread 3-1" },
                                { incoming: false, address: "5555211003", text: "thread 3-2" }]))
  
  
  
  
    .then(() => testGetThreads([{ incoming: true, address: "5555211004", text: "thread 4" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: true, address: "5555211005", text: "thread 5-1" },
                                { incoming: true, address: "+15555211005", text: "thread 5-2" },]))
  
  
  
    .then(() => testGetThreads([{ incoming: true, address: "+15555211006", text: "thread 6-1" },
                                { incoming: true, address: "5555211006", text: "thread 6-2" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "5555211007", text: "thread 7-1" },
                                { incoming: false, address: "+15555211007", text: "thread 7-2" },
                                { incoming: true, address: "5555211007", text: "thread 7-3" },
                                { incoming: true, address: "+15555211007", text: "thread 7-4" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: true, address: "5555211008", text: "thread 8-1" },
                                { incoming: true, address: "+15555211008", text: "thread 8-2" },
                                { incoming: false, address: "5555211008", text: "thread 8-3" },
                                { incoming: false, address: "+15555211008", text: "thread 8-4" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "+15555211009", text: "thread 9-1" },
                                { incoming: false, address: "01115555211009", text: "thread 9-2" },
                                { incoming: false, address: "5555211009", text: "thread 9-3" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "+15555211010", text: "thread 10-1" },
                                { incoming: false, address: "5555211010", text: "thread 10-2" },
                                { incoming: false, address: "01115555211010", text: "thread 10-3" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "01115555211011", text: "thread 11-1" },
                                { incoming: false, address: "5555211011", text: "thread 11-2" },
                                { incoming: false, address: "+15555211011", text: "thread 11-3" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "01115555211012", text: "thread 12-1" },
                                { incoming: false, address: "+15555211012", text: "thread 12-2" },
                                { incoming: false, address: "5555211012", text: "thread 12-3" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "5555211013", text: "thread 13-1" },
                                { incoming: false, address: "+15555211013", text: "thread 13-2" },
                                { incoming: false, address: "01115555211013", text: "thread 13-3" }]))
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "5555211014", text: "thread 14-1" },
                                { incoming: false, address: "01115555211014", text: "thread 14-2" },
                                { incoming: false, address: "+15555211014", text: "thread 14-3" }]))
  
  
  
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "5555211015", text: "thread 15-1" },
                                { incoming: false, address: "555211015", text: "thread 16-1" }],
                                true))
  
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "+551155211017", text: "thread 17-1" },
                                { incoming: false, address: "1155211017", text: "thread 17-2" }]))
  
  
  
  
    .then(() => testGetThreads([{ incoming: false, address: "1155211018", text: "thread 18-1" },
                                { incoming: false, address: "+551155211018", text: "thread 18-2" }]));
});
