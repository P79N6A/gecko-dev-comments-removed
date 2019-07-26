





const source = "0x1.";
const completions = ["toExponential", "toFixed", "toString"];
Cu.import("resource://gre/modules/Task.jsm");

function test() {
  const options = { tabContent: "test scratchpad autocomplete" };
  openTabAndScratchpad(options)
    .then(Task.async(runTests))
    .then(finish, console.error);
}


function* runTests([win, sp]) {
  const {editor} = sp;
  const editorWin = editor.container.contentWindow;

  
  sp.setText(source);
  sp.editor.setCursor({ line: 0, ch: source.length });
  yield keyOnce("suggestion-entered", " ", { ctrlKey: true });

  
  const hints = editorWin.document.querySelector(".CodeMirror-hints");

  ok(hints,
     "The hint container should exist.")
  is(hints.childNodes.length, 3,
     "The hint container should have the completions.");

  let i = 0;
  for (let completion of completions) {
    let active = hints.querySelector(".CodeMirror-hint-active");
    is(active.textContent, completion,
       "Check that completion " + i++ + " is what is expected.");
    yield keyOnce("suggestion-entered", "VK_DOWN");
  }

  
  yield keyOnce("after-suggest", "VK_RETURN");

  is(sp.getText(), source + completions[0],
     "Autocompletion should work and select the right element.");

  
  sp.setText("5");
  yield keyOnce("show-information", " ", { shiftKey: true });

  
  const info = editorWin.document.querySelector(".CodeMirror-Tern-information");
  ok(info,
     "Info tooltip should appear.");
  is(info.textContent.slice(0, 6), "number",
     "Info tooltip should have expected contents.");

  function keyOnce(event, key, opts = {}) {
    const p = editor.once(event);
    EventUtils.synthesizeKey(key, opts, editorWin);
    return p;
  }
}
