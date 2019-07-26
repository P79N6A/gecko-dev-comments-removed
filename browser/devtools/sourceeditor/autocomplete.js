




const cssAutoCompleter = require("devtools/sourceeditor/css-autocompleter");
const { AutocompletePopup } = require("devtools/shared/autocomplete-popup");

const CM_TERN_SCRIPTS = [
  "chrome://browser/content/devtools/codemirror/tern.js",
  "chrome://browser/content/devtools/codemirror/show-hint.js"
];

const privates = new WeakMap();




function setupAutoCompletion(ctx, options) {
  let { cm, ed, Editor } = ctx;

  let win = ed.container.contentWindow.wrappedJSObject;
  let {CodeMirror} = win;

  let completer = null;
  let autocompleteKey = "Ctrl-" +
                        Editor.keyFor("autocompletion", { noaccel: true });
  if (ed.config.mode == Editor.modes.js) {
    let defs = [
      "tern/browser",
      "tern/ecma5",
    ].map(require);

    CM_TERN_SCRIPTS.forEach(ed.loadScript, ed);
    win.tern = require("tern/tern");
    cm.tern = new CodeMirror.TernServer({ defs: defs });
    cm.on("cursorActivity", (cm) => {
      cm.tern.updateArgHints(cm);
    });

    let keyMap = {};

    keyMap[autocompleteKey] = (cm) => {
      cm.tern.getHint(cm, (data) => {
        CodeMirror.on(data, "shown", () => ed.emit("before-suggest"));
        CodeMirror.on(data, "close", () => ed.emit("after-suggest"));
        CodeMirror.on(data, "select", () => ed.emit("suggestion-entered"));
        CodeMirror.showHint(cm, (cm, cb) => cb(data), { async: true });
      });
    };

    keyMap[Editor.keyFor("showInformation", { noaccel: true })] = (cm) => {
      cm.tern.showType(cm, null, () => {
        ed.emit("show-information");
      });
    };

    cm.addKeyMap(keyMap);

    
    return;
  } else if (ed.config.mode == Editor.modes.css) {
    completer = new cssAutoCompleter({walker: options.walker});
  }

  let popup = new AutocompletePopup(win.parent.document, {
    position: "after_start",
    fixedWidth: true,
    theme: "auto",
    autoSelect: true
  });

  let cycle = (reverse) => {
    if (popup && popup.isOpen) {
      cycleSuggestions(ed, reverse == true);
      return;
    }

    return CodeMirror.Pass;
  };

  let keyMap = {
    "Tab": cycle,
    "Down": cycle,
    "Shift-Tab": cycle.bind(null, true),
    "Up": cycle.bind(null, true),
    "Enter": () => {
      if (popup && popup.isOpen) {
        if (!privates.get(ed).suggestionInsertedOnce) {
          privates.get(ed).insertingSuggestion = true;
          let {label, preLabel, text} = popup.getItemAtIndex(0);
          let cur = ed.getCursor();
          ed.replaceText(text.slice(preLabel.length), cur, cur);
        }
        popup.hidePopup();
        
        ed.emit("popup-hidden");
        return;
      }

      return CodeMirror.Pass;
    }
  };
  keyMap[autocompleteKey] = cm => autoComplete(ctx);
  cm.addKeyMap(keyMap);

  cm.on("keydown", (cm, e) => onEditorKeypress(ctx, e));
  ed.on("change", () => autoComplete(ctx));
  ed.on("destroy", () => {
    cm.off("keydown", (cm, e) => onEditorKeypress(ctx, e));
    ed.off("change", () => autoComplete(ctx));
    popup.destroy();
    popup = null;
    completer = null;
  });

  privates.set(ed, {
    popup: popup,
    completer: completer,
    insertingSuggestion: false,
    suggestionInsertedOnce: false
  });
}




function autoComplete({ ed, cm }) {
  let private = privates.get(ed);
  let { completer, popup } = private;
  if (!completer || private.insertingSuggestion || private.doNotAutocomplete) {
    private.insertingSuggestion = false;
    return;
  }
  let cur = ed.getCursor();
  completer.complete(cm.getRange({line: 0, ch: 0}, cur), cur)
    .then(suggestions => {
    if (!suggestions || !suggestions.length || suggestions[0].preLabel == null) {
      private.suggestionInsertedOnce = false;
      popup.hidePopup();
      ed.emit("after-suggest");
      return;
    }
    
    
    
    

    let cursorElement = cm.display.cursorDiv.querySelector(".CodeMirror-cursor");
    let left = suggestions[0].preLabel.length * cm.defaultCharWidth() + 4;
    popup.hidePopup();
    popup.setItems(suggestions);
    popup.openPopup(cursorElement, -1 * left, 0);
    private.suggestionInsertedOnce = false;
    
    ed.emit("after-suggest");
  });
}





function cycleSuggestions(ed, reverse) {
  let private = privates.get(ed);
  let { popup, completer } = private;
  let cur = ed.getCursor();
  private.insertingSuggestion = true;
  if (!private.suggestionInsertedOnce) {
    private.suggestionInsertedOnce = true;
    let firstItem;
    if (reverse) {
      firstItem = popup.getItemAtIndex(popup.itemCount - 1);
      popup.selectPreviousItem();
    } else {
      firstItem = popup.getItemAtIndex(0);
      if (firstItem.label == firstItem.preLabel && popup.itemCount > 1) {
        firstItem = popup.getItemAtIndex(1);
        popup.selectNextItem();
      }
    }
    if (popup.itemCount == 1)
      popup.hidePopup();
    ed.replaceText(firstItem.text.slice(firstItem.preLabel.length), cur, cur);
  } else {
    let fromCur = {
      line: cur.line,
      ch  : cur.ch - popup.selectedItem.text.length
    };
    if (reverse)
      popup.selectPreviousItem();
    else
      popup.selectNextItem();
    ed.replaceText(popup.selectedItem.text, fromCur, cur);
  }
  
  ed.emit("suggestion-entered");
}





function onEditorKeypress({ ed, Editor }, event) {
  let private = privates.get(ed);

  
  if (ed.hasMultipleSelections()) {
    private.doNotAutocomplete = true;
    private.popup.hidePopup();
    return;
  }

  if ((event.ctrlKey || event.metaKey) && event.keyCode == event.DOM_VK_SPACE) {
    
    
    
    private.doNotAutocomplete = false;
    return;
  }

  if (event.ctrlKey || event.metaKey || event.altKey) {
    private.doNotAutocomplete = true;
    private.popup.hidePopup();
    return;
  }

  switch (event.keyCode) {
    case event.DOM_VK_RETURN:
      private.doNotAutocomplete = true;
      break;

    case event.DOM_VK_ESCAPE:
      if (private.popup.isOpen)
        event.preventDefault();
    case event.DOM_VK_LEFT:
    case event.DOM_VK_RIGHT:
    case event.DOM_VK_HOME:
    case event.DOM_VK_END:
      private.doNotAutocomplete = true;
      private.popup.hidePopup();
      break;

    case event.DOM_VK_BACK_SPACE:
    case event.DOM_VK_DELETE:
      if (ed.config.mode == Editor.modes.css)
        private.completer.invalidateCache(ed.getCursor().line)
      private.doNotAutocomplete = true;
      private.popup.hidePopup();
      break;

    default:
      private.doNotAutocomplete = false;
  }
}




function getPopup({ ed }) {
  return privates.get(ed).popup;
}





function getInfoAt({ ed }, caret) {
  let completer = privates.get(ed).completer;
  if (completer && completer.getInfoAt)
    return completer.getInfoAt(ed.getText(), caret);

  return null;
}



module.exports.setupAutoCompletion = setupAutoCompletion;
module.exports.getAutocompletionPopup = getPopup;
module.exports.getInfoAt = getInfoAt;
