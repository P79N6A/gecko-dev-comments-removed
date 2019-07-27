


"use strict";

const globalMM = Components.classes["@mozilla.org/globalmessagemanager;1"].
                 getService(Components.interfaces.nsIMessageListenerManager);




const PATH = __URI__.replace('framescript/FrameScriptManager.jsm', '');



let LOADER_ID = 0;
this.getNewLoaderID = () => {
  return PATH + ":" + LOADER_ID++;
}

const frame_script = function(contentFrame, PATH) {
  let { registerContentFrame } = Components.utils.import(PATH + 'framescript/content.jsm', {});
  registerContentFrame(contentFrame);
}
globalMM.loadFrameScript("data:,(" + frame_script.toString() + ")(this, " + JSON.stringify(PATH) + ");", true);

this.EXPORTED_SYMBOLS = ['getNewLoaderID'];
