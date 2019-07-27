


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  
  ["imsi", "3102600",            true, true               ],
  
  ["imsi", "31026xx0",           true, true               ],
  ["imsi", "310260x0x",          true, true               ],
  ["imsi", "310260X00",          true, true               ],
  ["imsi", "310260XX1",          true, false              ],
  ["imsi", "31026012",           true, false              ],
  ["imsi", "310260000000000",    true, true               ],
  ["imsi", "310260000000000123", true, false              ],
  ["imsi", "",                   false, "InvalidParameter"],
  
  ["spn",  "Android",            true, true               ],
  ["spn",  "",                   false, "InvalidParameter"],
  ["spn",  "OneTwoThree",        true, false              ],
  
  ["gid",  "",                   false, "InvalidParameter"],
  ["gid",  "A1",                 true, false              ],
  ["gid",  "5A",                 true, true               ],
  ["gid",  "5a",                 true, true               ],
  ["gid",  "5a4d",               true, true               ],
  ["gid",  "5A4D",               true, true               ],
  ["gid",  "5a4d6c",             true, false              ]
];

function testMatchMvno(aIcc, aMvnoType, aMvnoData, aSuccess, aExpectedResult) {
  log("matchMvno: " + aMvnoType + ", " + aMvnoData);
  return aIcc.matchMvno(aMvnoType, aMvnoData)
    .then((aResult) => {
      log("onsuccess: " + aResult);
      ok(aSuccess, "onsuccess while error expected");
      is(aResult, aExpectedResult);
    }, (aError) => {
      log("onerror: " + aError.name);
      ok(!aSuccess, "onerror while success expected");
      is(aError.name, aExpectedResult);
    });
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testMatchMvno.apply(null, [icc].concat(data)));
  }
  return promise;
});
