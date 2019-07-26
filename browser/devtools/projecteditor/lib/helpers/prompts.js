










const { Cu, Cc, Ci } = require("chrome");
const { getLocalizedString } = require("projecteditor/helpers/l10n");
const prompts = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Ci.nsIPromptService);












function confirm(title, message) {
  var result = prompts.confirm(null, title || "Title of this Dialog", message || "Are you sure?");
  return result;
}
exports.confirm = confirm;

