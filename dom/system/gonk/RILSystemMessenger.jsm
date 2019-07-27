



"use strict";




this.RILSystemMessenger = function() {};
RILSystemMessenger.prototype = {

  







  broadcastMessage: function(aType, aMessage) {
    
  },

  


  notifyNewCall: function() {
    this.broadcastMessage("telephony-new-call", {});
  },

  


  notifyCallEnded: function(aServiceId, aNumber, aCdmaWaitingNumber, aEmergency,
                            aDuration, aOutgoing, aHangUpLocal) {
    let data = {
      serviceId: aServiceId,
      number: aNumber,
      emergency: aEmergency,
      duration: aDuration,
      direction: aOutgoing ? "outgoing" : "incoming",
      hangUpLocal: aHangUpLocal
    };

    if (aCdmaWaitingNumber != null) {
      data.secondNumber = aCdmaWaitingNumber;
    }

    this.broadcastMessage("telephony-call-ended", data);
  }
};

this.EXPORTED_SYMBOLS = [
  'RILSystemMessenger'
];
