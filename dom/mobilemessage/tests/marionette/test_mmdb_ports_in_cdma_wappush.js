


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'mmdb_head.js';

const DBNAME = "test_mmdb_ports_in_cdma_wappush:" + newUUID();

const TEST_PDU = [
  {
    sender: "+0987654321",
    iccId: "1029384756",
    segmentRef: 0,
    segmentSeq: 1, 
    segmentMaxSeq: 2,
    encoding: 0x04, 
    data: [0, 1, 2],
    teleservice: 0x1004, 
    
    originatorPort: 9200, 
    destinationPort: 2948 
  },
  {
    sender: "+0987654321",
    iccId: "1029384756",
    segmentRef: 0,
    segmentSeq: 2, 
    segmentMaxSeq: 2,
    encoding: 0x04, 
    data: [3, 4, 5],
    teleservice: 0x1004, 
    
    originatorPort: Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID,
    destinationPort: Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID
  }
];

function testSaveCdmaWapPush(aMmdb, aReverse) {
  log("testSaveCdmaWapPush(), aReverse: " + aReverse);

  let testPDUs = aReverse ? Array.from(TEST_PDU).reverse() : TEST_PDU;
  let lengthOfFullData = 0;
  let promises = [];

  for (let pdu of testPDUs) {
    lengthOfFullData += pdu.data.length;
    promises.push(saveSmsSegment(aMmdb, pdu));
  };

  return Promise.all(promises)
    .then((aResults) => {
      
      let completeMsg = aResults[1][1];

      is(completeMsg.originatorPort, TEST_PDU[0].originatorPort, "originatorPort");
      is(completeMsg.destinationPort, TEST_PDU[0].destinationPort, "destinationPort");

      is(completeMsg.fullData.length, lengthOfFullData, "fullData.length");
      for (let i = 0; i < lengthOfFullData; i++) {
        is(completeMsg.fullData[i], i, "completeMsg.fullData[" + i + "]");
      }
    });
}

startTestBase(function testCaseMain() {

  let mmdb = newMobileMessageDB();
  return initMobileMessageDB(mmdb, DBNAME, 0)

    .then(() => testSaveCdmaWapPush(mmdb, false))
    .then(() => testSaveCdmaWapPush(mmdb, true))

    .then(() => closeMobileMessageDB(mmdb));
});
