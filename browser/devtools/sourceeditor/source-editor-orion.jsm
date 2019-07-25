







































"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/source-editor-ui.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");

const ORION_SCRIPT = "chrome://browser/content/orion.js";
const ORION_IFRAME = "data:text/html;charset=utf8,<!DOCTYPE html>" +
  "<html style='height:100%' dir='ltr'>" +
  "<head><link rel='stylesheet'" +
  " href='chrome://browser/skin/devtools/orion-container.css'></head>" +
  "<body style='height:100%;margin:0;overflow:hidden'>" +
  "<div id='editor' style='height:100%'></div>" +
  "</body></html>";

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";





const ORION_THEMES = {
  mozilla: ["chrome://browser/skin/devtools/orion.css"],
};





const ORION_EVENTS = {
  ContextMenu: "ContextMenu",
  TextChanged: "ModelChanged",
  Selection: "Selection",
  Focus: "Focus",
  Blur: "Blur",
  MouseOver: "MouseOver",
  MouseOut: "MouseOut",
  MouseMove: "MouseMove",
};




const ORION_ANNOTATION_TYPES = {
  currentBracket: "orion.annotation.currentBracket",
  matchingBracket: "orion.annotation.matchingBracket",
  breakpoint: "orion.annotation.breakpoint",
  task: "orion.annotation.task",
  currentLine: "orion.annotation.currentLine",
};




const DEFAULT_KEYBINDINGS = [
  {
    action: "undo",
    code: Ci.nsIDOMKeyEvent.DOM_VK_Z,
    accel: true,
  },
  {
    action: "redo",
    code: Ci.nsIDOMKeyEvent.DOM_VK_Z,
    accel: true,
    shift: true,
  },
  {
    action: "Unindent Lines",
    code: Ci.nsIDOMKeyEvent.DOM_VK_TAB,
    shift: true,
  },
];

var EXPORTED_SYMBOLS = ["SourceEditor"];










function SourceEditor() {
  

  SourceEditor.DEFAULTS.tabSize =
    Services.prefs.getIntPref(SourceEditor.PREFS.TAB_SIZE);
  SourceEditor.DEFAULTS.expandTab =
    Services.prefs.getBoolPref(SourceEditor.PREFS.EXPAND_TAB);

  this._onOrionSelection = this._onOrionSelection.bind(this);

  this._eventTarget = {};
  this._eventListenersQueue = [];
  this.ui = new SourceEditorUI(this);
}

SourceEditor.prototype = {
  _view: null,
  _iframe: null,
  _model: null,
  _undoStack: null,
  _linesRuler: null,
  _annotationRuler: null,
  _overviewRuler: null,
  _styler: null,
  _annotationStyler: null,
  _annotationModel: null,
  _dragAndDrop: null,
  _currentLineAnnotation: null,
  _mode: null,
  _expandTab: null,
  _tabSize: null,
  _iframeWindow: null,
  _eventTarget: null,
  _eventListenersQueue: null,

  




  ui: null,

  



  parentElement: null,

  












  init: function SE_init(aElement, aConfig, aCallback)
  {
    if (this._iframe) {
      throw new Error("SourceEditor is already initialized!");
    }

    let doc = aElement.ownerDocument;

    this._iframe = doc.createElementNS(XUL_NS, "iframe");
    this._iframe.flex = 1;

    let onIframeLoad = (function() {
      this._iframe.removeEventListener("load", onIframeLoad, true);
      this._onIframeLoad();
    }).bind(this);

    this._iframe.addEventListener("load", onIframeLoad, true);

    this._iframe.setAttribute("src", ORION_IFRAME);

    aElement.appendChild(this._iframe);
    this.parentElement = aElement;

    this._config = {};
    for (let key in SourceEditor.DEFAULTS) {
      this._config[key] = key in aConfig ?
                          aConfig[key] :
                          SourceEditor.DEFAULTS[key];
    }

    
    
    if (aConfig.placeholderText) {
      this._config.initialText = aConfig.placeholderText;
      Services.console.logStringMessage("SourceEditor.init() was called with the placeholderText option which is deprecated, please use initialText.");
    }

    this._onReadyCallback = aCallback;
    this.ui.init();
  },

  



  _onIframeLoad: function SE__onIframeLoad()
  {
    this._iframeWindow = this._iframe.contentWindow.wrappedJSObject;
    let window = this._iframeWindow;
    let config = this._config;

    Services.scriptloader.loadSubScript(ORION_SCRIPT, window, "utf8");

    let TextModel = window.require("orion/textview/textModel").TextModel;
    let TextView = window.require("orion/textview/textView").TextView;

    this._expandTab = config.expandTab;
    this._tabSize = config.tabSize;

    let theme = config.theme;
    let stylesheet = theme in ORION_THEMES ? ORION_THEMES[theme] : theme;

    this._model = new TextModel(config.initialText);
    this._view = new TextView({
      model: this._model,
      parent: "editor",
      stylesheet: stylesheet,
      tabSize: this._tabSize,
      expandTab: this._expandTab,
      readonly: config.readOnly,
      themeClass: "mozilla" + (config.readOnly ? " readonly" : ""),
    });

    let onOrionLoad = function() {
      this._view.removeEventListener("Load", onOrionLoad);
      this._onOrionLoad();
    }.bind(this);

    this._view.addEventListener("Load", onOrionLoad);
    if (config.highlightCurrentLine || Services.appinfo.OS == "Linux") {
      this._view.addEventListener("Selection", this._onOrionSelection);
    }

    let KeyBinding = window.require("orion/textview/keyBinding").KeyBinding;
    let TextDND = window.require("orion/textview/textDND").TextDND;
    let Rulers = window.require("orion/textview/rulers");
    let LineNumberRuler = Rulers.LineNumberRuler;
    let AnnotationRuler = Rulers.AnnotationRuler;
    let OverviewRuler = Rulers.OverviewRuler;
    let UndoStack = window.require("orion/textview/undoStack").UndoStack;
    let AnnotationModel = window.require("orion/textview/annotations").AnnotationModel;

    this._annotationModel = new AnnotationModel(this._model);

    if (config.showAnnotationRuler) {
      this._annotationRuler = new AnnotationRuler(this._annotationModel, "left",
        {styleClass: "ruler annotations"});
      this._annotationRuler.onClick = this._annotationRulerClick.bind(this);
      this._annotationRuler.addAnnotationType(ORION_ANNOTATION_TYPES.breakpoint);
      this._annotationRuler.setMultiAnnotation({
        html: "<div class='annotationHTML multiple'></div>"
      });
      this._annotationRuler.setMultiAnnotationOverlay({
        html: "<div class='annotationHTML overlay'></div>"
      });
      this._view.addRuler(this._annotationRuler);
    }

    if (config.showLineNumbers) {
      let rulerClass = this._annotationRuler ?
                       "ruler lines linesWithAnnotations" :
                       "ruler lines";

      this._linesRuler = new LineNumberRuler(this._annotationModel, "left",
        {styleClass: rulerClass}, {styleClass: "rulerLines odd"},
        {styleClass: "rulerLines even"});

      this._linesRuler.onClick = this._linesRulerClick.bind(this);
      this._linesRuler.onDblClick = this._linesRulerDblClick.bind(this);
      this._view.addRuler(this._linesRuler);
    }

    if (config.showOverviewRuler) {
      this._overviewRuler = new OverviewRuler(this._annotationModel, "right",
        {styleClass: "ruler overview"});
      this._overviewRuler.onClick = this._overviewRulerClick.bind(this);

      this._overviewRuler.addAnnotationType(ORION_ANNOTATION_TYPES.matchingBracket);
      this._overviewRuler.addAnnotationType(ORION_ANNOTATION_TYPES.currentBracket);
      this._overviewRuler.addAnnotationType(ORION_ANNOTATION_TYPES.breakpoint);
      this._overviewRuler.addAnnotationType(ORION_ANNOTATION_TYPES.task);
      this._view.addRuler(this._overviewRuler);
    }

    this.setMode(config.mode);

    this._undoStack = new UndoStack(this._view, config.undoLimit);

    this._dragAndDrop = new TextDND(this._view, this._undoStack);

    let actions = {
      "undo": [this.undo, this],
      "redo": [this.redo, this],
      "tab": [this._doTab, this],
      "Unindent Lines": [this._doUnindentLines, this],
      "enter": [this._doEnter, this],
      "Find...": [this.ui.find, this.ui],
      "Find Next Occurrence": [this.ui.findNext, this.ui],
      "Find Previous Occurrence": [this.ui.findPrevious, this.ui],
      "Goto Line...": [this.ui.gotoLine, this.ui],
    };

    for (let name in actions) {
      let action = actions[name];
      this._view.setAction(name, action[0].bind(action[1]));
    }

    let keys = (config.keys || []).concat(DEFAULT_KEYBINDINGS);
    keys.forEach(function(aKey) {
      let binding = new KeyBinding(aKey.code, aKey.accel, aKey.shift, aKey.alt);
      this._view.setKeyBinding(binding, aKey.action);

      if (aKey.callback) {
        this._view.setAction(aKey.action, aKey.callback);
      }
    }, this);

    this._initEventTarget();
  },

  




  _initEventTarget: function SE__initEventTarget()
  {
    let EventTarget =
      this._iframeWindow.require("orion/textview/eventTarget").EventTarget;
    EventTarget.addMixin(this._eventTarget);

    this._eventListenersQueue.forEach(function(aRequest) {
      if (aRequest[0] == "add") {
        this.addEventListener(aRequest[1], aRequest[2]);
      } else {
        this.removeEventListener(aRequest[1], aRequest[2]);
      }
    }, this);

    this._eventListenersQueue = [];
  },

  







  _dispatchEvent: function SE__dispatchEvent(aEvent)
  {
    this._eventTarget.dispatchEvent(aEvent);
  },

  




  _onOrionLoad: function SE__onOrionLoad()
  {
    this.ui.onReady();
    if (this._onReadyCallback) {
      this._onReadyCallback(this);
      this._onReadyCallback = null;
    }
  },

  




  _doTab: function SE__doTab()
  {
    let indent = "\t";
    let selection = this.getSelection();
    let model = this._model;
    let firstLine = model.getLineAtOffset(selection.start);
    let firstLineStart = model.getLineStart(firstLine);
    let lastLineOffset = selection.end > selection.start ?
                         selection.end - 1 : selection.end;
    let lastLine = model.getLineAtOffset(lastLineOffset);

    if (this._expandTab) {
      let offsetFromLineStart = firstLine == lastLine ?
                                selection.start - firstLineStart : 0;
      let spaces = this._tabSize - (offsetFromLineStart % this._tabSize);
      indent = (new Array(spaces + 1)).join(" ");
    }

    
    if (firstLine != lastLine) {
      let lines = [""];
      let lastLineEnd = model.getLineEnd(lastLine, true);
      let selectedLines = lastLine - firstLine + 1;

      for (let i = firstLine; i <= lastLine; i++) {
        lines.push(model.getLine(i, true));
      }

      this.startCompoundChange();

      this.setText(lines.join(indent), firstLineStart, lastLineEnd);

      let newSelectionStart = firstLineStart == selection.start ?
                              selection.start : selection.start + indent.length;
      let newSelectionEnd = selection.end + (selectedLines * indent.length);

      this._view.setSelection(newSelectionStart, newSelectionEnd);

      this.endCompoundChange();
      return true;
    }

    return false;
  },

  




  _doUnindentLines: function SE__doUnindentLines()
  {
    let indent = "\t";

    let selection = this.getSelection();
    let model = this._model;
    let firstLine = model.getLineAtOffset(selection.start);
    let lastLineOffset = selection.end > selection.start ?
                         selection.end - 1 : selection.end;
    let lastLine = model.getLineAtOffset(lastLineOffset);

    if (this._expandTab) {
      indent = (new Array(this._tabSize + 1)).join(" ");
    }

    let lines = [];
    for (let line, i = firstLine; i <= lastLine; i++) {
      line = model.getLine(i, true);
      if (line.indexOf(indent) != 0) {
        return true;
      }
      lines.push(line.substring(indent.length));
    }

    let firstLineStart = model.getLineStart(firstLine);
    let lastLineStart = model.getLineStart(lastLine);
    let lastLineEnd = model.getLineEnd(lastLine, true);

    this.startCompoundChange();

    this.setText(lines.join(""), firstLineStart, lastLineEnd);

    let selectedLines = lastLine - firstLine + 1;
    let newSelectionStart = firstLineStart == selection.start ?
                            selection.start :
                            Math.max(firstLineStart,
                                     selection.start - indent.length);
    let newSelectionEnd = selection.end - (selectedLines * indent.length) +
                          (selection.end == lastLineStart + 1 ? 1 : 0);
    if (firstLine == lastLine) {
      newSelectionEnd = Math.max(lastLineStart, newSelectionEnd);
    }
    this._view.setSelection(newSelectionStart, newSelectionEnd);

    this.endCompoundChange();

    return true;
  },

  




  _doEnter: function SE__doEnter()
  {
    let selection = this.getSelection();
    if (selection.start != selection.end) {
      return false;
    }

    let model = this._model;
    let lineIndex = model.getLineAtOffset(selection.start);
    let lineText = model.getLine(lineIndex, true);
    let lineStart = model.getLineStart(lineIndex);
    let index = 0;
    let lineOffset = selection.start - lineStart;
    while (index < lineOffset && /[ \t]/.test(lineText.charAt(index))) {
      index++;
    }

    if (!index) {
      return false;
    }

    let prefix = lineText.substring(0, index);
    index = lineOffset;
    while (index < lineText.length &&
           /[ \t]/.test(lineText.charAt(index++))) {
      selection.end++;
    }

    this.setText(this.getLineDelimiter() + prefix, selection.start,
                 selection.end);
    return true;
  },

  








  _onOrionSelection: function SE__onOrionSelection(aEvent)
  {
    if (this._config.highlightCurrentLine) {
      this._highlightCurrentLine(aEvent);
    }

    if (Services.appinfo.OS == "Linux") {
      let text = this.getText(aEvent.newValue.start, aEvent.newValue.end);
      if (!text) {
        return;
      }

      clipboardHelper.copyStringToClipboard(text,
                                            Ci.nsIClipboard.kSelectionClipboard);
    }
  },

  






  _highlightCurrentLine: function SE__highlightCurrentLine(aEvent)
  {
    let annotationModel = this._annotationModel;
    let model = this._model;
    let oldAnnotation = this._currentLineAnnotation;
    let newSelection = aEvent.newValue;

    let collapsed = newSelection.start == newSelection.end;
    if (!collapsed) {
      if (oldAnnotation) {
        annotationModel.removeAnnotation(oldAnnotation);
        this._currentLineAnnotation = null;
      }
      return;
    }

    let line = model.getLineAtOffset(newSelection.start);
    let lineStart = model.getLineStart(line);
    let lineEnd = model.getLineEnd(line);

    let title = oldAnnotation ? oldAnnotation.title :
                SourceEditorUI.strings.GetStringFromName("annotation.currentLine");

    this._currentLineAnnotation = {
      start: lineStart,
      end: lineEnd,
      type: ORION_ANNOTATION_TYPES.currentLine,
      title: title,
      html: "<div class='annotationHTML currentLine'></div>",
      overviewStyle: {styleClass: "annotationOverview currentLine"},
      lineStyle: {styleClass: "annotationLine currentLine"},
    };

    annotationModel.replaceAnnotations(oldAnnotation ? [oldAnnotation] : null,
                                       [this._currentLineAnnotation]);
  },

  










  _linesRulerClick: function SE__linesRulerClick(aLineIndex, aEvent)
  {
    if (aLineIndex === undefined) {
      return;
    }

    if (aEvent.shiftKey) {
      let model = this._model;
      let selection = this.getSelection();
      let selectionLineStart = model.getLineAtOffset(selection.start);
      let selectionLineEnd = model.getLineAtOffset(selection.end);
      let newStart = aLineIndex <= selectionLineStart ?
                     model.getLineStart(aLineIndex) : selection.start;
      let newEnd = aLineIndex <= selectionLineStart ?
                   selection.end : model.getLineEnd(aLineIndex);
      this.setSelection(newStart, newEnd);
    } else {
      this.setCaretPosition(aLineIndex);
    }
  },

  









  _linesRulerDblClick: function SE__linesRulerDblClick(aLineIndex)
  {
    if (aLineIndex === undefined) {
      return;
    }

    let newStart = this._model.getLineStart(aLineIndex);
    let newEnd = this._model.getLineEnd(aLineIndex);
    this.setSelection(newStart, newEnd);
  },

  




  _highlightAnnotations: function SE__highlightAnnotations()
  {
    if (this._annotationStyler) {
      this._annotationStyler.destroy();
      this._annotationStyler = null;
    }

    let AnnotationStyler =
      this._iframeWindow.require("orion/textview/annotations").AnnotationStyler;

    let styler = new AnnotationStyler(this._view, this._annotationModel);
    this._annotationStyler = styler;

    styler.addAnnotationType(ORION_ANNOTATION_TYPES.matchingBracket);
    styler.addAnnotationType(ORION_ANNOTATION_TYPES.currentBracket);
    styler.addAnnotationType(ORION_ANNOTATION_TYPES.task);

    if (this._config.highlightCurrentLine) {
      styler.addAnnotationType(ORION_ANNOTATION_TYPES.currentLine);
    }
  },

  













  _getAnnotationsByType: function SE__getAnnotationsByType(aType, aStart, aEnd)
  {
    let annotations = this._annotationModel.getAnnotations(aStart, aEnd);
    let annotation, result = [];
    while (annotation = annotations.next()) {
      if (annotation.type == ORION_ANNOTATION_TYPES[aType]) {
        result.push(annotation);
      }
    }

    return result;
  },

  








  _annotationRulerClick: function SE__annotationRulerClick(aLineIndex, aEvent)
  {
    if (aLineIndex === undefined || aLineIndex == -1) {
      return;
    }

    let lineStart = this._model.getLineStart(aLineIndex);
    let lineEnd = this._model.getLineEnd(aLineIndex);
    let annotations = this._getAnnotationsByType("breakpoint", lineStart, lineEnd);
    if (annotations.length > 0) {
      this.removeBreakpoint(aLineIndex);
    } else {
      this.addBreakpoint(aLineIndex);
    }
  },

  









  _overviewRulerClick: function SE__overviewRulerClick(aLineIndex, aEvent)
  {
    if (aLineIndex === undefined || aLineIndex == -1) {
      return;
    }

    let model = this._model;
    let lineStart = model.getLineStart(aLineIndex);
    let lineEnd = model.getLineEnd(aLineIndex);
    let annotations = this._annotationModel.getAnnotations(lineStart, lineEnd);
    let annotation = annotations.next();

    
    
    if (!annotation || lineStart == annotation.start && lineEnd == annotation.end) {
      this.setSelection(lineStart, lineStart);
    } else {
      this.setSelection(annotation.start, annotation.end);
    }
  },

  





  get editorElement() {
    return this._iframe;
  },

  









  addEventListener: function SE_addEventListener(aEventType, aCallback)
  {
    if (this._view && aEventType in ORION_EVENTS) {
      this._view.addEventListener(ORION_EVENTS[aEventType], aCallback);
    } else if (this._eventTarget.addEventListener) {
      this._eventTarget.addEventListener(aEventType, aCallback);
    } else {
      this._eventListenersQueue.push(["add", aEventType, aCallback]);
    }
  },

  










  removeEventListener: function SE_removeEventListener(aEventType, aCallback)
  {
    if (this._view && aEventType in ORION_EVENTS) {
      this._view.removeEventListener(ORION_EVENTS[aEventType], aCallback);
    } else if (this._eventTarget.removeEventListener) {
      this._eventTarget.removeEventListener(aEventType, aCallback);
    } else {
      this._eventListenersQueue.push(["remove", aEventType, aCallback]);
    }
  },

  


  undo: function SE_undo()
  {
    return this._undoStack.undo();
  },

  


  redo: function SE_redo()
  {
    return this._undoStack.redo();
  },

  





  canUndo: function SE_canUndo()
  {
    return this._undoStack.canUndo();
  },

  





  canRedo: function SE_canRedo()
  {
    return this._undoStack.canRedo();
  },

  


  resetUndo: function SE_resetUndo()
  {
    this._undoStack.reset();
  },

  




  startCompoundChange: function SE_startCompoundChange()
  {
    this._undoStack.startCompoundChange();
  },

  


  endCompoundChange: function SE_endCompoundChange()
  {
    this._undoStack.endCompoundChange();
  },

  


  focus: function SE_focus()
  {
    this._view.focus();
  },

  





  getTopIndex: function SE_getTopIndex()
  {
    return this._view.getTopIndex();
  },

  





  setTopIndex: function SE_setTopIndex(aTopIndex)
  {
    this._view.setTopIndex(aTopIndex);
  },

  





  hasFocus: function SE_hasFocus()
  {
    return this._view.hasFocus();
  },

  












  getText: function SE_getText(aStart, aEnd)
  {
    return this._view.getText(aStart, aEnd);
  },

  





  getCharCount: function SE_getCharCount()
  {
    return this._model.getCharCount();
  },

  





  getSelectedText: function SE_getSelectedText()
  {
    let selection = this.getSelection();
    return this.getText(selection.start, selection.end);
  },

  











  setText: function SE_setText(aText, aStart, aEnd)
  {
    this._view.setText(aText, aStart, aEnd);
  },

  


  dropSelection: function SE_dropSelection()
  {
    this.setCaretOffset(this.getCaretOffset());
  },

  







  setSelection: function SE_setSelection(aStart, aEnd)
  {
    this._view.setSelection(aStart, aEnd, true);
  },

  






  getSelection: function SE_getSelection()
  {
    return this._view.getSelection();
  },

  





  getCaretOffset: function SE_getCaretOffset()
  {
    return this._view.getCaretOffset();
  },

  





  setCaretOffset: function SE_setCaretOffset(aOffset)
  {
    this._view.setCaretOffset(aOffset, true);
  },

  







  getCaretPosition: function SE_getCaretPosition()
  {
    let offset = this.getCaretOffset();
    let line = this._model.getLineAtOffset(offset);
    let lineStart = this._model.getLineStart(line);
    let column = offset - lineStart;
    return {line: line, col: column};
  },

  







  setCaretPosition: function SE_setCaretPosition(aLine, aColumn)
  {
    this.setCaretOffset(this._model.getLineStart(aLine) + (aColumn || 0));
  },

  





  getLineCount: function SE_getLineCount()
  {
    return this._model.getLineCount();
  },

  





  getLineDelimiter: function SE_getLineDelimiter()
  {
    return this._model.getLineDelimiter();
  },

  





  getIndentationString: function SE_getIndentationString()
  {
    if (this._expandTab) {
      return (new Array(this._tabSize + 1)).join(" ");
    }
    return "\t";
  },

  





  setMode: function SE_setMode(aMode)
  {
    if (this._styler) {
      this._styler.destroy();
      this._styler = null;
    }

    let window = this._iframeWindow;

    switch (aMode) {
      case SourceEditor.MODES.JAVASCRIPT:
      case SourceEditor.MODES.CSS:
        let TextStyler =
          window.require("examples/textview/textStyler").TextStyler;

        this._styler = new TextStyler(this._view, aMode, this._annotationModel);
        this._styler.setFoldingEnabled(false);
        break;

      case SourceEditor.MODES.HTML:
      case SourceEditor.MODES.XML:
        let TextMateStyler =
          window.require("orion/editor/textMateStyler").TextMateStyler;
        let HtmlGrammar =
          window.require("orion/editor/htmlGrammar").HtmlGrammar;
        this._styler = new TextMateStyler(this._view, new HtmlGrammar());
        break;
    }

    this._highlightAnnotations();
    this._mode = aMode;
  },

  





  getMode: function SE_getMode()
  {
    return this._mode;
  },

  




  set readOnly(aValue)
  {
    this._view.setOptions({
      readonly: aValue,
      themeClass: "mozilla" + (aValue ? " readonly" : ""),
    });
  },

  



  get readOnly()
  {
    return this._view.getOptions("readonly");
  },

  







  addBreakpoint: function SE_addBreakpoint(aLineIndex, aCondition)
  {
    let lineStart = this._model.getLineStart(aLineIndex);
    let lineEnd = this._model.getLineEnd(aLineIndex);

    let annotations = this._getAnnotationsByType("breakpoint", lineStart, lineEnd);
    if (annotations.length > 0) {
      return;
    }

    let lineText = this._model.getLine(aLineIndex);
    let title = SourceEditorUI.strings.
                formatStringFromName("annotation.breakpoint.title",
                                     [lineText], 1);

    let annotation = {
      type: ORION_ANNOTATION_TYPES.breakpoint,
      start: lineStart,
      end: lineEnd,
      breakpointCondition: aCondition,
      title: title,
      style: {styleClass: "annotation breakpoint"},
      html: "<div class='annotationHTML breakpoint'></div>",
      overviewStyle: {styleClass: "annotationOverview breakpoint"},
      rangeStyle: {styleClass: "annotationRange breakpoint"}
    };
    this._annotationModel.addAnnotation(annotation);

    let event = {
      type: SourceEditor.EVENTS.BREAKPOINT_CHANGE,
      added: [{line: aLineIndex, condition: aCondition}],
      removed: [],
    };

    this._dispatchEvent(event);
  },

  







  removeBreakpoint: function SE_removeBreakpoint(aLineIndex)
  {
    let lineStart = this._model.getLineStart(aLineIndex);
    let lineEnd = this._model.getLineEnd(aLineIndex);

    let event = {
      type: SourceEditor.EVENTS.BREAKPOINT_CHANGE,
      added: [],
      removed: [],
    };

    let annotations = this._getAnnotationsByType("breakpoint", lineStart, lineEnd);

    annotations.forEach(function(annotation) {
      this._annotationModel.removeAnnotation(annotation);
      event.removed.push({line: aLineIndex,
                          condition: annotation.breakpointCondition});
    }, this);

    if (event.removed.length > 0) {
      this._dispatchEvent(event);
    }

    return event.removed.length > 0;
  },

  






  getBreakpoints: function SE_getBreakpoints()
  {
    let annotations = this._getAnnotationsByType("breakpoint", 0,
                                                 this.getCharCount());
    let breakpoints = [];

    annotations.forEach(function(annotation) {
      breakpoints.push({line: this._model.getLineAtOffset(annotation.start),
                        condition: annotation.breakpointCondition});
    }, this);

    return breakpoints;
  },

  


  destroy: function SE_destroy()
  {
    if (this._config.highlightCurrentLine || Services.appinfo.OS == "Linux") {
      this._view.removeEventListener("Selection", this._onOrionSelection);
    }
    this._onOrionSelection = null;

    this._view.destroy();
    this.ui.destroy();
    this.ui = null;

    this.parentElement.removeChild(this._iframe);
    this.parentElement = null;
    this._iframeWindow = null;
    this._iframe = null;
    this._undoStack = null;
    this._styler = null;
    this._linesRuler = null;
    this._annotationRuler = null;
    this._overviewRuler = null;
    this._dragAndDrop = null;
    this._annotationModel = null;
    this._annotationStyler = null;
    this._currentLineAnnotation = null;
    this._eventTarget = null;
    this._eventListenersQueue = null;
    this._view = null;
    this._model = null;
    this._config = null;
    this._lastFind = null;
  },
};
