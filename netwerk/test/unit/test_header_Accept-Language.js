



const Cc = Components.classes;
const Ci = Components.interfaces;

var testpath = "/bug672448";

function run_test() {
  let intlPrefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService).getBranch("intl.");

  
  let oldPref = intlPrefs.getCharPref("accept_languages");

  
  let acceptLangTests = [
    "qaa", 
    "qaa,qab", 
    "qaa,qab,qac,qad", 
    "qaa,qab,qac,qad,qae,qaf,qag,qah", 
    "qaa,qab,qac,qad,qae,qaf,qag,qah,qai,qaj", 
    "qaa,qab,qac,qad,qae,qaf,qag,qah,qai,qaj,qak", 
    "qaa,qab,qac,qad,qae,qaf,qag,qah,qai,qaj,qak,qal,qam,qan,qao,qap,qaq,qar,qas,qat,qau", 
    oldPref, 
  ];

  let acceptLangTestsNum = acceptLangTests.length;

  for (let i = 0; i < acceptLangTestsNum; i++) {
    
    intlPrefs.setCharPref("accept_languages", acceptLangTests[i]);

    
    test_accepted_languages();
  }
}

function test_accepted_languages() {
  let channel = setupChannel(testpath);

  let AcceptLanguage = channel.getRequestHeader("Accept-Language");

  let acceptedLanguages = AcceptLanguage.split(",");

  let acceptedLanguagesLength = acceptedLanguages.length;

  for (let i = 0; i < acceptedLanguagesLength; i++) {
    let acceptedLanguage, qualityValue;

    try {
      
      [_, acceptedLanguage, qualityValue] = acceptedLanguages[i].trim().match(/^([a-z0-9_-]*?)(?:;q=(1(?:\.0{0,3})?|0(?:\.[0-9]{0,3})))?$/i);
    } catch(e) {
      do_throw("Invalid language tag or quality value: " + e);
    }

    if (i == 0) {
      
      do_check_eq(qualityValue, undefined);
    } else {
      let decimalPlaces;

      
      
      if (acceptedLanguagesLength < 10) {
        do_check_true(qualityValue.length == 3);

        decimalPlaces = 1;
      } else {
        do_check_true(qualityValue.length >= 3);
        do_check_true(qualityValue.length <= 4);

        decimalPlaces = 2;
      }

      
      do_check_eq(parseFloat(qualityValue).toFixed(decimalPlaces), (1.0 - ((1 / acceptedLanguagesLength) * i)).toFixed(decimalPlaces));
    }
  }
}

function setupChannel(path) {
  let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  let chan = ios.newChannel("http://localhost:4444" + path, "", null);
  chan.QueryInterface(Ci.nsIHttpChannel);
  return chan;
}
