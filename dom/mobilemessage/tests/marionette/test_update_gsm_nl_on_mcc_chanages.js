


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const kPrefLastKnownSimMcc = "ril.lastKnownSimMcc";




const MSG_TURKISH = "çorba kaşığı";




const MSG_TURKISH_LONG = "Sinan Akçıl-Ebru Şallı cephesinde sular durulmuyor. \
Çiftin ayrılığına dair yeni iddialar ortaya atıldı. İlk iddiaya göre; Sinan \
Akçıl, Ebru Şallı'dan 1.5..";



const MSG_TURKISH_MULTI_SEGS = "ABD'de Başkan Barack Obama'nın kendisini \
Twitter'dan takip ettiğini söyleyen Kam Brock adlı kadının psikiyatri tedavisi \
görmeye zorlandığı bildirildi. Obama";

function getSegmentInfoForText(aBody) {
  return new Promise(function(resolve, reject) {
    let domRequest = manager.getSegmentInfoForText(aBody);
    domRequest.onsuccess = function() {
      let segmentInfo = domRequest.result;
      log("getSegmentInfoForText success: " + JSON.stringify(segmentInfo));
      resolve(segmentInfo);
    };
    domRequest.onerror = function(){
      log("getSegmentInfoForText error");
      reject();
    }
  });
}

function verifySegmentInfo(segmentInfo, expectedSegmentInfo, msg) {
  is(segmentInfo.segments, expectedSegmentInfo.segments, msg);
  is(segmentInfo.charsPerSegment, expectedSegmentInfo.charsPerSegment, msg);
  is(segmentInfo.charsAvailableInLastSegment,
    expectedSegmentInfo.charsAvailableInLastSegment, msg);
};








startTestCommon(function testCaseMain() {
  return Promise.resolve()
    
    .then(() => pushPrefEnv({set: [[kPrefLastKnownSimMcc, "310"]]}))

    
    .then(() => getSegmentInfoForText(MSG_TURKISH))
    .then((segmentInfo) => verifySegmentInfo(segmentInfo,
      {segments: 1, charsPerSegment: 70,
        charsAvailableInLastSegment: 70 - MSG_TURKISH.length},
      "US / UCS-2 / short Turkish message."))

    
    .then(() => getSegmentInfoForText(MSG_TURKISH_LONG))
    .then((segmentInfo) => verifySegmentInfo(segmentInfo,
      {segments: 3, charsPerSegment: 67,
        charsAvailableInLastSegment: 67 - MSG_TURKISH_LONG.length % 67},
      "US / UCS-2 / long Turkish message."))

    
    .then(() => pushPrefEnv({set: [[kPrefLastKnownSimMcc, "286"]]}))

    
    .then(() => getSegmentInfoForText(MSG_TURKISH))
    .then((segmentInfo) => verifySegmentInfo(segmentInfo,
      {segments: 1, charsPerSegment: 155,
        charsAvailableInLastSegment: 155 - MSG_TURKISH.length},
      "Turkey / GSM 7 bits / short Turkish message."))

    
    .then(() => getSegmentInfoForText(MSG_TURKISH_LONG))
    .then((segmentInfo) => verifySegmentInfo(segmentInfo,
      {segments: 1, charsPerSegment: 155, charsAvailableInLastSegment: 0},
      "Turkey / GSM 7 bits / longest single segment Turkish message."))

    
    .then(() => getSegmentInfoForText(MSG_TURKISH_MULTI_SEGS))
    .then((segmentInfo) => verifySegmentInfo(segmentInfo,
      {segments: 2, charsPerSegment: 149,
        charsAvailableInLastSegment: 149 - MSG_TURKISH_MULTI_SEGS.length % 149},
      "Turkey / GSM 7 bits / shortest dual segments Turkish message."));
});
