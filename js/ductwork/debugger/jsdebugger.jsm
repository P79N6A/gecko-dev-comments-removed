




this.EXPORTED_SYMBOLS = [ "addDebuggerToGlobal" ];














const init = Components.classes["@mozilla.org/jsdebugger;1"].createInstance(Components.interfaces.IJSDebugger);
this.addDebuggerToGlobal = function addDebuggerToGlobal(global) {
  init.addClass(global);
};
