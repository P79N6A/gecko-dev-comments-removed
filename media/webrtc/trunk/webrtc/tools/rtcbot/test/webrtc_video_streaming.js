












function testOneWayVideo(test, bot1, bot2) {
  var report = test.createStatisticsReport("webrtc_video_streaming");

  test.wait([
      createPeerConnection.bind(bot1),
      createPeerConnection.bind(bot2) ],
    onPeerConnectionCreated);

  function createPeerConnection(done) {
    test.createTurnConfig(onTurnConfig.bind(this), test.fail);

    function onTurnConfig(config) {
      this.createPeerConnection(config, done, test.fail);
    };
  }

  function onPeerConnectionCreated(pc1, pc2) {
    test.log("RTC Peers created.");
    pc1.addEventListener('addstream', test.fail);
    pc2.addEventListener('addstream', onAddStream);
    pc1.addEventListener('icecandidate', onIceCandidate.bind(pc2));
    pc2.addEventListener('icecandidate', onIceCandidate.bind(pc1));

    bot1.getUserMedia({video:true, audio:true}, onUserMediaSuccess, test.fail);

    function onUserMediaSuccess(stream) {
      test.log("User has granted access to local media.");
      pc1.addStream(stream);
      bot1.showStream(stream.id, true, true);

      createOfferAndAnswer(pc1, pc2);
    }
  }

  function onAddStream(event) {
    test.log("On Add stream.");
    bot2.showStream(event.stream.id, true, false);
  }

  function onIceCandidate(event) {
    if(event.candidate) {
      test.log(event.candidate.candidate);
      this.addIceCandidate(event.candidate,
         onAddIceCandidateSuccess, test.fail);
    }

    function onAddIceCandidateSuccess() {
      test.log("Candidate added successfully");
    }
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
      collectStats();
    }

    function onSetSessionDescriptionSuccess() {
      test.log("Set session description success.");
    }

    function collectStats() {
      report.collectStatsFromPeerConnection("bot1", pc1);
      report.collectStatsFromPeerConnection("bot2", pc2);

      setTimeout(function() {
        report.finish(test.done);
        }, 10000);
    }
  }
}

registerBotTest('testOneWayVideo/chrome-chrome',
                testOneWayVideo, ['chrome', 'chrome']);
