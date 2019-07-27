


const MARIONETTE_TIMEOUT = 60000;
const MARIONETTE_HEAD_JS = 'head.js';




function checkEventStatus(aEvent, aServiceId, aHasMessages, aMessageCount,
                          aReturnNumber, aReturnMessage) {
  let status = aEvent.status;
  ok(true, "status = " + JSON.stringify(status));
  ok(status instanceof MozVoicemailStatus);

  checkVoicemailStatus(status, 0, aHasMessages, aMessageCount, aReturnNumber,
                       aReturnMessage);

  compareVoicemailStatus(voicemail.getStatus(0), status);
  compareVoicemailStatus(voicemail.getStatus(), status);
}

function testLevel2Discard(aActive) {
  log("    Active: " + aActive);

  let sender = "+15125551235";
  let body = "1 new voicemail";
  let pdu = PDUBuilder.buildLevel2DiscardMwi(aActive, sender, body);
  return sendIndicatorPDUAndWait(pdu)
    .then((aResults) => checkEventStatus(aResults[0], 0, aActive,
                                         (aActive ? -1 : 0), sender, body));
}


function testLevel3Discard(aMessageCount, aBody) {
  log("    Message Count: " + aMessageCount + ", Body: " + aBody);

  let sender = "+15125551236";
  let pdu = PDUBuilder.buildLevel3DiscardMwi(aMessageCount, sender, aBody);
  return sendIndicatorPDUAndWait(pdu)
    .then((aResults) => checkEventStatus(aResults[0], 0, !!aMessageCount,
                                         aMessageCount, sender, aBody));
}

startTestCommon(function() {
  return Promise.resolve()

    .then(() => log("Testing Message Waiting Indication Group"))
    
    .then(() => log("  Discard Message"))
    .then(() => testLevel2Discard(true))
    .then(() => testLevel2Discard(false))

    .then(() => log("Testing Special SMS Message Indication"))
    .then(() => log("  Discard Message"))
    
    .then(() => testLevel3Discard(3, "3 voicemails"))
    .then(() => testLevel3Discard(0, "0 voicemail"))
    .then(() => testLevel3Discard(3, null))
    .then(() => testLevel3Discard(0, null));
});
