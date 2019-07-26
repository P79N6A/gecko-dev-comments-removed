







module.metadata = {
  "stability": "unstable"
};


let hash = {}, bestMatchingLocale = null;
try {
  let data = require("@l10n/data");
  hash = data.hash;
  bestMatchingLocale = data.bestMatchingLocale;
}
catch(e) {}


exports.get = function get(k) {
  return k in hash ? hash[k] : null;
}


exports.locale = function locale() {
  return bestMatchingLocale;
}

exports.language = function language() {
  return bestMatchingLocale ? bestMatchingLocale.split("-")[0].toLowerCase()
                            : null;
}
