



































EXPORTED_SYMBOLS = [ "PluralForm" ];












const Cc = Components.classes;
const Ci = Components.interfaces;

const kIntlProperties = "chrome://global/locale/intl.properties";



let gFunctions = [
  function(n) 0,
  function(n) n!=1?1:0,
  function(n) n>1?1:0,
  function(n) n%10==1&&n%100!=11?1:n!=0?2:0,
  function(n) n==1?0:n==2?1:2,
  function(n) n==1?0:n==0||n%100>0&&n%100<20?1:2,
  function(n) n%10==1&&n%100!=11?0:n%10>=2&&(n%100<10||n%100>=20)?2:1,
  function(n) n%10==1&&n%100!=11?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2,
  function(n) n==1?0:n>=2&&n<=4?1:2,
  function(n) n==1?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2,
  function(n) n%100==1?0:n%100==2?1:n%100==3||n%100==4?2:3
];

let PluralForm = {
  








  get: (function initGetPluralForm()
  {
    
    
    
    

    
    let ruleNum = Number(Cc["@mozilla.org/intl/stringbundle;1"].
      getService(Ci.nsIStringBundleService).createBundle(kIntlProperties).
      GetStringFromName("pluralRule"));

    
    if (ruleNum < 0 || ruleNum >= gFunctions.length || isNaN(ruleNum)) {
      log(["Invalid rule number: ", ruleNum, " -- defaulting to 0"]);
      ruleNum = 0;
    }

    
    let pluralFunc = gFunctions[ruleNum];
    return function(aNum, aWords) {
      
      let index = pluralFunc(aNum ? Number(aNum) : 0);
      let words = aWords ? aWords.split(/;/) : [""];

      let ret = words[index];

      
      if ((ret == undefined) || (ret == "")) {
        
        log(["Index #", index, " of '", aWords, "' for value ", aNum,
            " is invalid -- plural rule #", ruleNum]);

        
        ret = words[0];
      }

      return ret;
    };
  })(),
};







function log(aMsg)
{
  let msg = "PluralForm.jsm: " + (aMsg.join ? aMsg.join("") : aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
    logStringMessage(msg);
  dump(msg + "\n");
}
