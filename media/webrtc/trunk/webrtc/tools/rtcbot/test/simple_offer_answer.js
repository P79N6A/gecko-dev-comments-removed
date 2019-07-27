











function testOfferAnswer(test, bot1, bot2) {
  test.wait( [ bot1.createPeerConnection.bind(bot1, null),
               bot2.createPeerConnection.bind(bot2, null) ],
            run);

  function run(pc1, pc2) {
    test.log("Establishing call.");
    pc1.createOffer(gotOffer);

    function gotOffer(offer) {
      test.log("Got offer");
      expectedCall();
      pc1.setLocalDescription(offer, expectedCall, test.fail);
      pc2.setRemoteDescription(offer, expectedCall, test.fail);
      pc2.createAnswer(gotAnswer, test.fail);
    }

    function gotAnswer(answer) {
      test.log("Got answer");
      expectedCall();
      pc2.setLocalDescription(answer, expectedCall, test.fail);
      pc1.setRemoteDescription(answer, expectedCall, test.fail);
    }

    
    
    var expectedCalls = 0;
    function expectedCall() {
      if (++expectedCalls == 6)
        test.done();
    }
  }
}

registerBotTest('testOfferAnswer/chrome-chrome',
                testOfferAnswer, ['chrome', 'chrome']);
