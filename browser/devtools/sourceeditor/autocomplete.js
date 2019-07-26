




const cssAutoCompleter = require("devtools/sourceeditor/css-autocompleter");
const { AutocompletePopup } = require("devtools/shared/autocomplete-popup");

const privates = new WeakMap();





function setupAutoCompletion(ctx, walker) {
  let { cm, ed, Editor } = ctx;

  let win = ed.container.contentWindow.wrappedJSObject;

  let completer = null;
  if (ed.config.mode == Editor.modes.css)
    completer = new cssAutoCompleter({walker: walker});

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

    return win.CodeMirror.Pass;
  };

  let keyMap = {
    "Tab": cycle,
    "Down": cycle,
    "Shift-Tab": cycle.bind(this, true),
    "Up": cycle.bind(this, true),
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

      return win.CodeMirror.Pass;
    }
  };
  keyMap[Editor.accel("Space")] = cm => autoComplete(ctx);
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

  switch (event.keyCode) {
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
