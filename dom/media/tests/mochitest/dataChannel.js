









function getBlobContent(blob) {
  return new Promise(resolve => {
    var reader = new FileReader();
    
    reader.onloadend = event => resolve(event.target.result);
    reader.readAsText(blob);
  });
}

var commandsCreateDataChannel = [
  function PC_REMOTE_EXPECT_DATA_CHANNEL(test) {
    test.pcRemote.expectDataChannel();
  },

  function PC_LOCAL_CREATE_DATA_CHANNEL(test) {
    var channel = test.pcLocal.createDataChannel({});
    is(channel.binaryType, "blob", channel + " is of binary type 'blob'");
    is(channel.readyState, "connecting", channel + " is in state: 'connecting'");

    is(test.pcLocal.signalingState, STABLE,
       "Create datachannel does not change signaling state");
    return test.pcLocal.observedNegotiationNeeded;
  }
];

var commandsWaitForDataChannel = [
  function PC_LOCAL_VERIFY_DATA_CHANNEL_STATE(test) {
    return test.pcLocal.dataChannels[0].opened;
  },

  function PC_REMOTE_VERIFY_DATA_CHANNEL_STATE(test) {
    return test.pcRemote.nextDataChannel.then(channel => channel.opened);
  },
];

var commandsCheckDataChannel = [
  function SEND_MESSAGE(test) {
    var message = "Lorem ipsum dolor sit amet";

    return test.send(message).then(result => {
      is(result.data, message, "Message correctly transmitted from pcLocal to pcRemote.");
    });
  },

  function SEND_BLOB(test) {
    var contents = "At vero eos et accusam et justo duo dolores et ea rebum.";
    var blob = new Blob([contents], { "type" : "text/plain" });

    return test.send(blob).then(result => {
      ok(result.data instanceof Blob, "Received data is of instance Blob");
      is(result.data.size, blob.size, "Received data has the correct size.");

      return getBlobContent(result.data);
    }).then(recv_contents =>
            is(recv_contents, contents, "Received data has the correct content."));
  },

  function CREATE_SECOND_DATA_CHANNEL(test) {
    return test.createDataChannel({ }).then(result => {
      var sourceChannel = result.local;
      var targetChannel = result.remote;
      is(sourceChannel.readyState, "open", sourceChannel + " is in state: 'open'");
      is(targetChannel.readyState, "open", targetChannel + " is in state: 'open'");

      is(targetChannel.binaryType, "blob", targetChannel + " is of binary type 'blob'");
    });
  },

  function SEND_MESSAGE_THROUGH_LAST_OPENED_CHANNEL(test) {
    var channels = test.pcRemote.dataChannels;
    var message = "I am the Omega";

    return test.send(message).then(result => {
      is(channels.indexOf(result.channel), channels.length - 1, "Last channel used");
      is(result.data, message, "Received message has the correct content.");
    });
  },


  function SEND_MESSAGE_THROUGH_FIRST_CHANNEL(test) {
    var message = "Message through 1st channel";
    var options = {
      sourceChannel: test.pcLocal.dataChannels[0],
      targetChannel: test.pcRemote.dataChannels[0]
    };

    return test.send(message, options).then(result => {
      is(test.pcRemote.dataChannels.indexOf(result.channel), 0, "1st channel used");
      is(result.data, message, "Received message has the correct content.");
    });
  },


  function SEND_MESSAGE_BACK_THROUGH_FIRST_CHANNEL(test) {
    var message = "Return a message also through 1st channel";
    var options = {
      sourceChannel: test.pcRemote.dataChannels[0],
      targetChannel: test.pcLocal.dataChannels[0]
    };

    return test.send(message, options).then(result => {
      is(test.pcLocal.dataChannels.indexOf(result.channel), 0, "1st channel used");
      is(result.data, message, "Return message has the correct content.");
    });
  },

  function CREATE_NEGOTIATED_DATA_CHANNEL(test) {
    var options = {
      negotiated:true,
      id: 5,
      protocol: "foo/bar",
      ordered: false,
      maxRetransmits: 500
    };
    return test.createDataChannel(options).then(result => {
      var sourceChannel2 = result.local;
      var targetChannel2 = result.remote;
      is(sourceChannel2.readyState, "open", sourceChannel2 + " is in state: 'open'");
      is(targetChannel2.readyState, "open", targetChannel2 + " is in state: 'open'");

      is(targetChannel2.binaryType, "blob", targetChannel2 + " is of binary type 'blob'");

      is(sourceChannel2.id, options.id, sourceChannel2 + " id is:" + sourceChannel2.id);
      var reliable = !options.ordered ? false : (options.maxRetransmits || options.maxRetransmitTime);
      is(sourceChannel2.protocol, options.protocol, sourceChannel2 + " protocol is:" + sourceChannel2.protocol);
      is(sourceChannel2.reliable, reliable, sourceChannel2 + " reliable is:" + sourceChannel2.reliable);
      








      is(targetChannel2.id, options.id, targetChannel2 + " id is:" + targetChannel2.id);
      is(targetChannel2.protocol, options.protocol, targetChannel2 + " protocol is:" + targetChannel2.protocol);
      is(targetChannel2.reliable, reliable, targetChannel2 + " reliable is:" + targetChannel2.reliable);
      







    });
  },

  function SEND_MESSAGE_THROUGH_LAST_OPENED_CHANNEL2(test) {
    var channels = test.pcRemote.dataChannels;
    var message = "I am the walrus; Goo goo g'joob";

    return test.send(message).then(result => {
      is(channels.indexOf(result.channel), channels.length - 1, "Last channel used");
      is(result.data, message, "Received message has the correct content.");
    });
  }
];

function addInitialDataChannel(chain) {
  chain.insertBefore('PC_LOCAL_CREATE_OFFER', commandsCreateDataChannel);
  chain.insertBefore('PC_LOCAL_CHECK_MEDIA_TRACKS', commandsWaitForDataChannel);
  chain.removeAfter('PC_REMOTE_CHECK_ICE_CONNECTIONS');
  chain.append(commandsCheckDataChannel);
}
