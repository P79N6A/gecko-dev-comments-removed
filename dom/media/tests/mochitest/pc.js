




var PeerConnection = {
  pc1_offer : null,
  pc2_answer : null,

  






  handShake: function PC_handShake(aPCLocal, aPCRemote, aSuccessCallback) {

    function onCreateOfferSuccess(aOffer) {
      pc1_offer = aOffer;
      info("Calling setLocalDescription on local peer connection");
      aPCLocal.setLocalDescription(aOffer, onSetLocalDescriptionSuccess1,
                                   unexpectedCallbackAndFinish);
    }

    function onSetLocalDescriptionSuccess1() {
      info("Calling setRemoteDescription on remote peer connection");
      aPCRemote.setRemoteDescription(pc1_offer, onSetRemoteDescriptionSuccess1,
                                     unexpectedCallbackAndFinish);
    }

    function onSetRemoteDescriptionSuccess1() {
      info("Calling createAnswer on remote peer connection");
      aPCRemote.createAnswer(onCreateAnswerSuccess, unexpectedCallbackAndFinish);
    }

    function onCreateAnswerSuccess(aAnswer) {
      pc2_answer = aAnswer;
      info("Calling setLocalDescription on remote peer connection");
      aPCRemote.setLocalDescription(aAnswer, onSetLocalDescriptionSuccess2,
                                    unexpectedCallbackAndFinish);
    }

    function onSetLocalDescriptionSuccess2() {
      info("Calling setRemoteDescription on local peer connection");
      aPCLocal.setRemoteDescription(pc2_answer, onSetRemoteDescriptionSuccess2,
                                    unexpectedCallbackAndFinish);
    }

    function onSetRemoteDescriptionSuccess2() {
      aSuccessCallback();
    }

    info("Calling createOffer on local peer connection");
    aPCLocal.createOffer(onCreateOfferSuccess, unexpectedCallbackAndFinish);
  },

  









  findStream: function PC_findStream(aMediaStreamList, aMediaStream) {
    for (var index = 0; index < aMediaStreamList.length; index++) {
      if (aMediaStreamList[index] === aMediaStream) {
        return index;
      }
    }

    return -1
  }
};
