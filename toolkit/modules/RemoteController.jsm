




this.EXPORTED_SYMBOLS = ["RemoteController"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function RemoteController(browser)
{
  this._browser = browser;

  
  
  this._supportedCommands = { };
}

RemoteController.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIController]),

  isCommandEnabled: function(aCommand) {
    return this._supportedCommands[aCommand] || false;
  },

  supportsCommand: function(aCommand) {
    return aCommand in this._supportedCommands;
  },

  doCommand: function(aCommand) {
    this._browser.messageManager.sendAsyncMessage("ControllerCommands:Do", aCommand);
  },

  onEvent: function () {},

  
  
  enableDisableCommands: function(aAction,
                                  aEnabledLength, aEnabledCommands,
                                  aDisabledLength, aDisabledCommands) {
    
    this._supportedCommands = { };

    for (let c = 0; c < aEnabledLength; c++) {
      this._supportedCommands[aEnabledCommands[c]] = true;
    }

    for (let c = 0; c < aDisabledLength; c++) {
      this._supportedCommands[aDisabledCommands[c]] = false;
    }

    this._browser.ownerDocument.defaultView.updateCommands(aAction);
  }
};
