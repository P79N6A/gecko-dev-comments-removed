



"use strict";

loader.lazyRequireGetter(this, "Services");
loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");
loader.lazyImporter(this, "Task", "resource://gre/modules/Task.jsm");














exports.viewSourceInStyleEditor = Task.async(function *(toolbox, sourceURL, sourceLine) {
  let panel = yield toolbox.loadTool("styleeditor");

  try {
    let selected = panel.UI.once("editor-selected");
    yield panel.selectStyleSheet(sourceURL, sourceLine);
    yield toolbox.selectTool("styleeditor");
    yield selected;
    return true;
  } catch (e) {
    exports.viewSource(toolbox, sourceURL, sourceLine);
    return false;
  }
});














exports.viewSourceInDebugger = Task.async(function *(toolbox, sourceURL, sourceLine) {
  
  
  
  let debuggerAlreadyOpen = toolbox.getPanel("jsdebugger");
  let { panelWin: dbg } = yield toolbox.loadTool("jsdebugger");

  if (!debuggerAlreadyOpen) {
    yield dbg.once(dbg.EVENTS.SOURCES_ADDED);
  }

  let { DebuggerView } = dbg;
  let { Sources } = DebuggerView;

  let item = Sources.getItemForAttachment(a => a.source.url === sourceURL);
  if (item) {
    yield toolbox.selectTool("jsdebugger");
    yield DebuggerView.setEditorLocation(item.attachment.source.actor, sourceLine, { noDebug: true });
    return true;
  }

  
  exports.viewSource(toolbox, sourceURL, sourceLine);
  return false;
});









exports.viewSourceInScratchpad = Task.async(function *(sourceURL, sourceLine) {
  
  let wins = Services.wm.getEnumerator("devtools:scratchpad");

  while (wins.hasMoreElements()) {
    let win = wins.getNext();

    if (!win.closed && win.Scratchpad.uniqueName === sourceURL) {
      win.focus();
      win.Scratchpad.editor.setCursor({ line: sourceLine, ch: 0 });
      return;
    }
  }

  
  for (let [, toolbox] of gDevTools) {
    let scratchpadPanel = toolbox.getPanel("scratchpad");
    if (scratchpadPanel) {
      let { scratchpad } = scratchpadPanel;
      if (scratchpad.uniqueName === sourceURL) {
        toolbox.selectTool("scratchpad");
        toolbox.raise();
        scratchpad.editor.focus();
        scratchpad.editor.setCursor({ line: sourceLine, ch: 0 });
        return;
      }
    }
  }
});










exports.viewSource = Task.async(function *(toolbox, sourceURL, sourceLine) {
  let utils = toolbox.gViewSourceUtils;
  utils.viewSource(sourceURL, null, toolbox.doc, sourceLine || 0);
});
