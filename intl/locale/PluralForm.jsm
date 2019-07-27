



this.EXPORTED_SYMBOLS = [ "PluralForm" ];
























const Cc = Components.classes;
const Ci = Components.interfaces;

const kIntlProperties = "chrome://global/locale/intl.properties";




let gFunctions = [
  
  [1, (n) => 0],
  
  [2, (n) => n!=1?1:0],
  
  [2, (n) => n>1?1:0],
  
  [3, (n) => n%10==1&&n%100!=11?1:n!=0?2:0],
  
  [4, (n) => n==1||n==11?0:n==2||n==12?1:n>0&&n<20?2:3],
  
  [3, (n) => n==1?0:n==0||n%100>0&&n%100<20?1:2],
  
  [3, (n) => n%10==1&&n%100!=11?0:n%10>=2&&(n%100<10||n%100>=20)?2:1],
  
  [3, (n) => n%10==1&&n%100!=11?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2],
  
  [3, (n) => n==1?0:n>=2&&n<=4?1:2],
  
  [3, (n) => n==1?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2],
  
  [4, (n) => n%100==1?0:n%100==2?1:n%100==3||n%100==4?2:3],
  
  [5, (n) => n==1?0:n==2?1:n>=3&&n<=6?2:n>=7&&n<=10?3:4],
  
  [6, (n) => n==0?5:n==1?0:n==2?1:n%100>=3&&n%100<=10?2:n%100>=11&&n%100<=99?3:4],
  
  [4, (n) => n==1?0:n==0||n%100>0&&n%100<=10?1:n%100>10&&n%100<20?2:3],
  
  [3, (n) => n%10==1?0:n%10==2?1:2],
  
  [2, (n) => n%10==1&&n%100!=11?0:1],
  
  [5, (n) => n%10==1&&n%100!=11&&n%100!=71&&n%100!=91?0:n%10==2&&n%100!=12&&n%100!=72&&n%100!=92?1:(n%10==3||n%10==4||n%10==9)&&n%100!=13&&n%100!=14&&n%100!=19&&n%100!=73&&n%100!=74&&n%100!=79&&n%100!=93&&n%100!=94&&n%100!=99?2:n%1000000==0&&n!=0?3:4],
];

this.PluralForm = {
  








  get get()
  {
    
    
    
    

    
    delete PluralForm.numForms;
    delete PluralForm.get;

    
    let ruleNum = Number(Cc["@mozilla.org/intl/stringbundle;1"].
      getService(Ci.nsIStringBundleService).createBundle(kIntlProperties).
      GetStringFromName("pluralRule"));

    
    [PluralForm.get, PluralForm.numForms] = PluralForm.makeGetter(ruleNum);
    return PluralForm.get;
  },

  







  makeGetter: function(aRuleNum)
  {
    
    if (aRuleNum < 0 || aRuleNum >= gFunctions.length || isNaN(aRuleNum)) {
      log(["Invalid rule number: ", aRuleNum, " -- defaulting to 0"]);
      aRuleNum = 0;
    }

    
    let [numForms, pluralFunc] = gFunctions[aRuleNum];

    
    
    return [function(aNum, aWords) {
      
      let index = pluralFunc(aNum ? Number(aNum) : 0);
      let words = aWords ? aWords.split(/;/) : [""];

      
      let ret = index < words.length ? words[index] : undefined;

      
      if ((ret == undefined) || (ret == "")) {
        
        let caller = Components.stack.caller ? Components.stack.caller.name : "top";

        
        log(["Index #", index, " of '", aWords, "' for value ", aNum,
            " is invalid -- plural rule #", aRuleNum, "; called by ", caller]);

        
        ret = words[0];
      }

      return ret;
    }, () => numForms];
  },

  




  get numForms()
  {
    
    PluralForm.get();
    return PluralForm.numForms;
  },
};







function log(aMsg)
{
  let msg = "PluralForm.jsm: " + (aMsg.join ? aMsg.join("") : aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
    logStringMessage(msg);
  dump(msg + "\n");
}
