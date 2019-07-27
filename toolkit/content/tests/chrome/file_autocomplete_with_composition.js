


function nsDoTestsForAutoCompleteWithComposition(aDescription,
                                                 aWindow,
                                                 aTarget,
                                                 aAutoCompleteController,
                                                 aIsFunc,
                                                 aGetTargetValueFunc,
                                                 aOnFinishFunc)
{
  this._description = aDescription;
  this._window = aWindow;
  this._target = aTarget;
  this._controller = aAutoCompleteController;

  this._is = aIsFunc;
  this._getTargetValue = aGetTargetValueFunc;
  this._onFinish = aOnFinishFunc;

  this._target.focus();

  this._DefaultCompleteDefaultIndex =
    this._controller.input.completeDefaultIndex;

  this._doTests();
}

nsDoTestsForAutoCompleteWithComposition.prototype = {
  _window: null,
  _target: null,
  _controller: null,
  _DefaultCompleteDefaultIndex: false,
  _description: "",

  _is: null,
  _getTargetValue: function () { return "not initialized"; },
  _onFinish: null,

  _doTests: function ()
  {
    if (++this._testingIndex == this._tests.length) {
      this._controller.input.completeDefaultIndex =
        this._DefaultCompleteDefaultIndex;
      this._onFinish();
      return;
    }

    var test = this._tests[this._testingIndex];
    if (this._controller.input.completeDefaultIndex != test.completeDefaultIndex) {
      this._controller.input.completeDefaultIndex = test.completeDefaultIndex;
    }
    test.execute(this._window);

    var timeout = this._controller.input.timeout + 10;
    this._waitResult(timeout);
  },

  _waitResult: function (aTimes)
  {
    var obj = this;
    if (aTimes-- > 0) {
      setTimeout(function () { obj._waitResult(aTimes); }, 0);
    } else {
      setTimeout(function () { obj._checkResult(); }, 0);
    }
  },

  _checkResult: function ()
  {
    var test = this._tests[this._testingIndex];
    this._is(this._getTargetValue(), test.value,
             this._description + ", " + test.description + ": value");
    this._is(this._controller.searchString, test.searchString,
             this._description + ", " + test.description +": searchString");
    this._is(this._controller.input.popupOpen, test.popup,
             this._description + ", " + test.description + ": popupOpen");
    this._doTests();
  },

  _testingIndex: -1,
  _tests: [
    
    
    
    { description: "compositionstart shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("m", { type: "keydown", shiftKey: true }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "M",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "M", searchString: ""
    },
    { description: "modifying composition string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "Mo",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mo", searchString: ""
    },
    { description: "compositionend should open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_RETURN", { type: "keyup" }, aWindow);
      }, popup: true, value: "Mo", searchString: "Mo"
    },
    
    
    { description: "compositionstart should close the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("z", { type: "keydown" }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "z",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Moz", searchString: "Mo"
    },
    { description: "modifying composition string shouldn't reopen the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "zi",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mozi", searchString: "Mo"
    },
    { description: "compositionend should research the result and open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_RETURN", { type: "keyup" }, aWindow);
      }, popup: true, value: "Mozi", searchString: "Mozi"
    },
    
    { description: "compositionstart should reclose the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("l", { type: "keydown" }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "l",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mozil", searchString: "Mozi"
    },
    { description: "modifying composition string shouldn't reopen the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "ll",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mozill", searchString: "Mozi"
    },
    { description: "modifying composition string to empty string shouldn't reopen the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "",
              "clauses":
              [
                { "length": 0, "attr": 0 }
              ]
            },
            "caret": { "start": 0, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mozi", searchString: "Mozi"
    },
    { description: "cancled compositionend should reopen the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommit", data: "" }, aWindow);
        synthesizeKey("VK_ESCAPE", { type: "keyup" }, aWindow);
      }, popup: true, value: "Mozi", searchString: "Mozi"
    },
    
    
    { description: "compositionstart with selected string should close the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("VK_LEFT", { shiftKey: true }, aWindow);
        synthesizeKey("VK_LEFT", { shiftKey: true }, aWindow);
        synthesizeKey("z", { type: "keydown" }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "z",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Moz", searchString: "Mozi"
    },
    { description: "modifying composition string shouldn't reopen the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "zi",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mozi", searchString: "Mozi"
    },
    { description: "modifying composition string to empty string shouldn't reopen the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "",
              "clauses":
              [
                { "length": 0, "attr": 0 }
              ]
            },
            "caret": { "start": 0, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mo", searchString: "Mozi"
    },
    { description: "canceled compositionend should seach the result with the latest value",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_ESCAPE", { type: "keyup" }, aWindow);
      }, popup: true, value: "Mo", searchString: "Mo"
    },
    
    { description: "the value becomes empty by backspace, the popup should be closed",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
      }, popup: false, value: "", searchString: ""
    },
    
    { description: "compositionstart shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("m", { type: "keydown", shiftKey: true }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "M",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "M", searchString: ""
    },
    { description: "modifying composition string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "Mo",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mo", searchString: ""
    },
    { description: "modifying composition string to empty string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "",
              "clauses":
              [
                { "length": 0, "attr": 0 }
              ]
            },
            "caret": { "start": 0, "length": 0 }
          }, aWindow);
      }, popup: false, value: "", searchString: ""
    },
    { description: "canceled compositionend shouldn't open the popup if it was closed",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_ESCAPE", { type: "keyup" }, aWindow);
      }, popup: false, value: "", searchString: ""
    },
    
    { description: "DOWN key should open the popup even if the value is empty",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("VK_DOWN", {}, aWindow);
      }, popup: true, value: "", searchString: ""
    },
    
    
    { description: "compositionstart shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("m", { type: "keydown", shiftKey: true }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "M",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "M", searchString: ""
    },
    { description: "modifying composition string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "Mo",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mo", searchString: ""
    },
    { description: "modifying composition string to empty string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "",
              "clauses":
              [
                { "length": 0, "attr": 0 }
              ]
            },
            "caret": { "start": 0, "length": 0 }
          }, aWindow);
      }, popup: false, value: "", searchString: ""
    },
    { description: "canceled compositionend should open the popup if it was opened",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_ESCAPE", { type: "keyup" }, aWindow);
      }, popup: true, value: "", searchString: ""
    },
    
    { description: "ESCAPE should close the popup after typing something",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("M", { shiftKey: true }, aWindow);
        synthesizeKey("o", { shiftKey: true }, aWindow);
        synthesizeKey("VK_ESCAPE", {}, aWindow);
      }, popup: false, value: "Mo", searchString: "Mo"
    },
    
    
    
    { description: "compositionstart shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("z", { type: "keydown", shiftKey: true }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "z",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Moz", searchString: "Mo"
    },
    { description: "modifying composition string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "zi",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mozi", searchString: "Mo"
    },
    { description: "modifying composition string to empty string shouldn't open the popup",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "",
              "clauses":
              [
                { "length": 0, "attr": 0 }
              ]
            },
            "caret": { "start": 0, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mo", searchString: "Mo"
    },
    { description: "canceled compositionend shouldn't open the popup if the popup was closed",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_ESCAPE", { type: "keyup" }, aWindow);
      }, popup: true, value: "Mo", searchString: "Mo"
    },
    
    { description: "house keeping for next tests",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
      }, popup: false, value: "", searchString: ""
    },
    
    { description: "compositionstart shouldn't open the popup (completeDefaultIndex is true)",
      completeDefaultIndex: true,
      execute: function (aWindow) {
        synthesizeKey("m", { type: "keydown", shiftKey: true }, aWindow);
        synthesizeCompositionChange(
          { "composition":
            { "string": "M",
              "clauses":
              [
                { "length": 1, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 1, "length": 0 }
          }, aWindow);
      }, popup: false, value: "M", searchString: ""
    },
    { description: "modifying composition string shouldn't open the popup (completeDefaultIndex is true)",
      completeDefaultIndex: true,
      execute: function (aWindow) {
        synthesizeCompositionChange(
          { "composition":
            { "string": "Mo",
              "clauses":
              [
                { "length": 2, "attr": COMPOSITION_ATTR_RAWINPUT }
              ]
            },
            "caret": { "start": 2, "length": 0 }
          }, aWindow);
      }, popup: false, value: "Mo", searchString: ""
    },
    { description: "compositionend should open the popup (completeDefaultIndex is true)",
      completeDefaultIndex: true,
      execute: function (aWindow) {
        synthesizeComposition({ type: "compositioncommitasis" }, aWindow);
        synthesizeKey("VK_RETURN", { type: "keyup" }, aWindow);
      }, popup: true, value: "Mozilla", searchString: "Mo"
    },
    
    { description: "house keeping for next tests",
      completeDefaultIndex: false,
      execute: function (aWindow) {
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
        synthesizeKey("VK_BACK_SPACE", {}, aWindow);
      }, popup: false, value: "", searchString: ""
    }
  ]
};
