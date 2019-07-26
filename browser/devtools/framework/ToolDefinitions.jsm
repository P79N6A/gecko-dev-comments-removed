



"use strict";

this.EXPORTED_SYMBOLS = [ "defaultTools" ];

Components.utils.import("resource:///modules/WebConsolePanel.jsm");
Components.utils.import("resource:///modules/devtools/DebuggerPanel.jsm");
Components.utils.import("resource:///modules/devtools/StyleEditorDefinition.jsm");
Components.utils.import("resource:///modules/devtools/InspectorDefinition.jsm");

this.defaultTools = [
  StyleEditorDefinition,
  WebConsoleDefinition,
  DebuggerDefinition,
  InspectorDefinition,
];
