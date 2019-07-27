


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

let settings = [
  
  {tech: "gsm",   mask: "gsm/wcdma"},
  {tech: "wcdma", mask: "gsm/wcdma"},

  
  {tech: "gsm",   mask: "gsm"},

  
  {tech: "wcdma", mask: "wcdma"},

  
  {tech: "gsm",   mask: "gsm/wcdma-auto"},
  {tech: "wcdma", mask: "gsm/wcdma-auto"},

  
  {tech: "cdma",  mask: "cdma/evdo"},
  {tech: "evdo",  mask: "cdma/evdo"},

  
  {tech: "cdma",  mask: "cdma"},

  
  {tech: "evdo",  mask: "evdo"},

  
  {tech: "gsm",   mask: "gsm/wcdma/cdma/evdo"},
  {tech: "wcdma", mask: "gsm/wcdma/cdma/evdo"},
  {tech: "cdma",  mask: "gsm/wcdma/cdma/evdo"},
  {tech: "evdo",  mask: "gsm/wcdma/cdma/evdo"}
];

startTest(function() {

  let promise = settings.reduce((aPromise, aSetting) => {
    return aPromise.then(() => gChangeModemTech(aSetting.tech, aSetting.mask));
  }, Promise.resolve());

  return promise
    
    .catch(error => ok(false, "Promise reject: " + error))

    
    .then(() => gChangeModemTech("wcdma", "gsm/wcdma"))
    .catch(error => ok(false, "Fetal Error: Promise reject: " + error))

    .then(finish);
});
