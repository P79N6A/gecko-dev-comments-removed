













function testTwoWayVideoStreaming(test, bot1, bot2, bot3) {
  var answersCount = 0;
  var statsCollector;

  test.wait([
      createBotPeerConnectionsWithLocalStream.bind(bot1),
      createBotPeerConnectionsWithLocalStream.bind(bot2),
      createBotPeerConnectionsWithLocalStream.bind(bot3)],
    onPeerConnectionCreated);

  
  function createBotPeerConnectionsWithLocalStream(done) {
    var peerConnections = [];

    this.getUserMedia({video:true, audio:true},
        onUserMediaSuccess.bind(this), test.fail);

    function onUserMediaSuccess(stream) {
      test.log("User has granted access to local media.");
      this.showStream(stream.id, true, true);

      test.createTurnConfig(onTurnConfig.bind(this), test.fail);

      function onTurnConfig(config) {
        this.createPeerConnection(config, addStream.bind(this),
            test.fail);
        this.createPeerConnection(config, addStream.bind(this),
            test.fail);
      }

      function addStream(pc) {
        pc.addStream(stream);
        pc.addEventListener('addstream', onAddStream.bind(this));

        peerConnections.push(pc);
        if(peerConnections.length == 2)
          done(peerConnections);
      }
    }
  }

  function onPeerConnectionCreated(peerConnections1,
      peerConnections2, peerConnections3) {
    test.log("RTC Peers created.");

    
    establichCall(peerConnections1[0], peerConnections2[1]);
    
    establichCall(peerConnections2[0], peerConnections3[1]);
    
    establichCall(peerConnections3[0], peerConnections1[1]);
  }

  function establichCall(pc1, pc2) {
    pc1.addEventListener('icecandidate', onIceCandidate.bind(pc2));
    pc2.addEventListener('icecandidate', onIceCandidate.bind(pc1));

    createOfferAndAnswer(pc1, pc2);
  }

  function onAddStream(event) {
    test.log("On Add stream.");
    this.showStream(event.stream.id, true, false);
  }

  function onIceCandidate(event) {
    if(event.candidate) {
      this.addIceCandidate(event.candidate,
         onAddIceCandidateSuccess, test.fail);
    };

    function onAddIceCandidateSuccess() {
      test.log("Candidate added successfully");
    };
  }

  function createOfferAndAnswer(pc1, pc2) {
    test.log("Creating offer.");
    pc1.createOffer(gotOffer, test.fail);

    function gotOffer(offer) {
      test.log("Got offer");
      pc1.setLocalDescription(offer, onSetSessionDescriptionSuccess, test.fail);
      pc2.setRemoteDescription(offer, onSetSessionDescriptionSuccess,
          test.fail);
      test.log("Creating answer");
      pc2.createAnswer(gotAnswer, test.fail);
    }

    function gotAnswer(answer) {
      test.log("Got answer");
      pc2.setLocalDescription(answer, onSetSessionDescriptionSuccess,
          test.fail);
      pc1.setRemoteDescription(answer, onSetSessionDescriptionSuccess,
          test.fail);

      answersCount++;
      if(answersCount == 3) {
        
        
        
        setTimeout(function() {
            test.done();
          }, 5000);
      }
    }

    function onSetSessionDescriptionSuccess() {
      test.log("Set session description success.");
    }
  }
}

registerBotTest('threeBotsVideoConference/android+android+chrome',
                testTwoWayVideoStreaming, ['android-chrome', 'android-chrome',
                'chrome']);
registerBotTest('threeBotsVideoConference/chrome-chrome-chrome',
                testTwoWayVideoStreaming, ['chrome', 'chrome', 'chrome']);
registerBotTest('threeBotsVideoConference/android-android-android',
                testTwoWayVideoStreaming, ['android-chrome', 'android-chrome',
                'android-chrome']);