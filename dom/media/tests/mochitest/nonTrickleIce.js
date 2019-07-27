



function makeOffererNonTrickle(chain) {
  chain.replace('PC_LOCAL_SETUP_ICE_HANDLER', [
    ['PC_LOCAL_SETUP_NOTRICKLE_ICE_HANDLER',
      function (test) {
        test.pcLocalWaitingForEndOfTrickleIce = false;
        
        
        test.pcLocal.setupIceCandidateHandler(test, function () {
            
          } , function (label) {
            if (test.pcLocalWaitingForEndOfTrickleIce) {
              
              
              
              
              info("Looks like we were still waiting for Trickle to finish");
              
              test.next();
            }
          });
        
        
        test.next();
      }
    ]
  ]);
  chain.replace('PC_REMOTE_GET_OFFER', [
    ['PC_REMOTE_WAIT_FOR_OFFER',
      function (test) {
        if (test.pcLocal.endOfTrickleIce) {
          info("Trickle ICE finished already");
          test.next();
        } else {
          info("Waiting for trickle ICE to finish");
          test.pcLocalWaitingForEndOfTrickleIce = true;
          
          
          
        }
      }
    ],
    ['PC_REMOTE_GET_FULL_OFFER',
      function (test) {
        test._local_offer = test.pcLocal.localDescription;
        test._offer_constraints = test.pcLocal.constraints;
        test._offer_options = test.pcLocal.offerOptions;
        test.next();
      }
    ]
  ]);
  chain.insertAfter('PC_REMOTE_SANE_REMOTE_SDP', [
    ['PC_REMOTE_REQUIRE_REMOTE_SDP_CANDIDATES',
      function (test) {
        info("test.pcLocal.localDescription.sdp: " + JSON.stringify(test.pcLocal.localDescription.sdp));
        info("test._local_offer.sdp" + JSON.stringify(test._local_offer.sdp));
        ok(!test.localRequiresTrickleIce, "Local does NOT require trickle");
        ok(test._local_offer.sdp.contains("a=candidate"), "offer has ICE candidates")
        
        test.next();
      }
    ]
  ]);
}

function makeAnswererNonTrickle(chain) {
  chain.replace('PC_REMOTE_SETUP_ICE_HANDLER', [
    ['PC_REMOTE_SETUP_NOTRICKLE_ICE_HANDLER',
      function (test) {
        test.pcRemoteWaitingForEndOfTrickleIce = false;
        
        
        test.pcRemote.setupIceCandidateHandler(test, function () {
          
          }, function (label) {
            if (test.pcRemoteWaitingForEndOfTrickleIce) {
              
              
              
              
              info("Looks like we were still waiting for Trickle to finish");
              
              test.next();
            }
          });
        
        
        test.next();
      }
    ]
  ]);
  chain.replace('PC_LOCAL_GET_ANSWER', [
    ['PC_LOCAL_WAIT_FOR_ANSWER',
      function (test) {
        if (test.pcRemote.endOfTrickleIce) {
          info("Trickle ICE finished already");
          test.next();
        } else {
          info("Waiting for trickle ICE to finish");
          test.pcRemoteWaitingForEndOfTrickleIce = true;
          
          
          
        }
      }
    ],
    ['PC_LOCAL_GET_FULL_ANSWER',
      function (test) {
        test._remote_answer = test.pcRemote.localDescription;
        test._answer_constraints = test.pcRemote.constraints;
        test.next();
      }
    ]
  ]);
  chain.insertAfter('PC_LOCAL_SANE_REMOTE_SDP', [
    ['PC_LOCAL_REQUIRE_REMOTE_SDP_CANDIDATES',
      function (test) {
        info("test.pcRemote.localDescription.sdp: " + JSON.stringify(test.pcRemote.localDescription.sdp));
        info("test._remote_answer.sdp" + JSON.stringify(test._remote_answer.sdp));
        ok(!test.remoteRequiresTrickleIce, "Remote does NOT require trickle");
        ok(test._remote_answer.sdp.contains("a=candidate"), "answer has ICE candidates")
        
        test.next();
      }
    ]
  ]);
}
