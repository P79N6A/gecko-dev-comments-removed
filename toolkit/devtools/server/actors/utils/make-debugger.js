





"use strict";

const EventEmitter = require("devtools/toolkit/event-emitter");
const Debugger = require("Debugger");

const { reportException } = require("devtools/toolkit/DevToolsUtils");




















































module.exports = function makeDebugger({ findDebuggees, shouldAddNewGlobalAsDebuggee }) {
  const dbg = new Debugger();
  EventEmitter.decorate(dbg);

  dbg.uncaughtExceptionHook = reportDebuggerHookException;

  dbg.onNewGlobalObject = global => {
    if (shouldAddNewGlobalAsDebuggee(global)) {
      safeAddDebuggee(dbg, global);
    }
  };

  dbg.addDebuggees = () => {
    for (let global of findDebuggees(dbg)) {
      safeAddDebuggee(dbg, global);
    }
  };

  return dbg;
};

const reportDebuggerHookException = e => reportException("Debugger Hook", e);




function safeAddDebuggee(dbg, global) {
  try {
    let wrappedGlobal = dbg.addDebuggee(global);
    if (wrappedGlobal) {
      dbg.emit("newGlobal", wrappedGlobal);
    }
  } catch (e) {
    
  }
}
