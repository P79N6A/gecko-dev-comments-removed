









const { Cu, Cc, Ci } = require("chrome");
const { ViewHelpers } = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});
const ITCHPAD_STRINGS_URI = "chrome://browser/locale/devtools/projecteditor.properties";
const L10N = new ViewHelpers.L10N(ITCHPAD_STRINGS_URI).stringBundle;

function getLocalizedString (name) {
  try {
    return L10N.GetStringFromName(name);
  } catch (ex) {
    console.log("Error reading '" + name + "'");
    throw new Error("l10n error with " + name);
  }
}

exports.getLocalizedString = getLocalizedString;
