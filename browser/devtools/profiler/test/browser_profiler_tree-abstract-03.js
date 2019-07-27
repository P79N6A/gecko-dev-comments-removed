







let { AbstractTreeItem } = Cu.import("resource:///modules/devtools/AbstractTreeItem.jsm", {});
let { Heritage } = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});

let test = Task.async(function*() {
  let container = document.createElement("vbox");
  gBrowser.selectedBrowser.parentNode.appendChild(container);

  

  let treeRoot = new MyCustomTreeItem(gDataSrc, { parent: null });
  treeRoot.attachTo(container);
  treeRoot.focus();

  EventUtils.sendKey("RIGHT");
  ok(treeRoot.expanded,
    "The root node is now expanded.");
  is(document.commandDispatcher.focusedElement, treeRoot.target,
    "The root node is still focused.");

  let fooItem = treeRoot.getChild(0);
  let barItem = treeRoot.getChild(1);

  EventUtils.sendKey("RIGHT");
  ok(!fooItem.expanded,
    "The 'foo' node is not expanded yet.");
  is(document.commandDispatcher.focusedElement, fooItem.target,
    "The 'foo' node is now focused.");

  EventUtils.sendKey("RIGHT");
  ok(fooItem.expanded,
    "The 'foo' node is now expanded.");
  is(document.commandDispatcher.focusedElement, fooItem.target,
    "The 'foo' node is still focused.");

  EventUtils.sendKey("RIGHT");
  ok(!barItem.expanded,
    "The 'bar' node is not expanded yet.");
  is(document.commandDispatcher.focusedElement, barItem.target,
    "The 'bar' node is now focused.");

  EventUtils.sendKey("RIGHT");
  ok(barItem.expanded,
    "The 'bar' node is now expanded.");
  is(document.commandDispatcher.focusedElement, barItem.target,
    "The 'bar' node is still focused.");

  let bazItem = barItem.getChild(0);

  EventUtils.sendKey("RIGHT");
  ok(!bazItem.expanded,
    "The 'baz' node is not expanded yet.");
  is(document.commandDispatcher.focusedElement, bazItem.target,
    "The 'baz' node is now focused.");

  EventUtils.sendKey("RIGHT");
  ok(bazItem.expanded,
    "The 'baz' node is now expanded.");
  is(document.commandDispatcher.focusedElement, bazItem.target,
    "The 'baz' node is still focused.");

  

  EventUtils.sendKey("RIGHT");
  is(document.commandDispatcher.focusedElement, bazItem.target,
    "The 'baz' node is still focused.");

  

  EventUtils.sendKey("DOWN");
  is(document.commandDispatcher.focusedElement, bazItem.target,
    "The 'baz' node is now refocused.");

  

  EventUtils.sendKey("UP");
  is(document.commandDispatcher.focusedElement, barItem.target,
    "The 'bar' node is now refocused.");

  EventUtils.sendKey("UP");
  is(document.commandDispatcher.focusedElement, fooItem.target,
    "The 'foo' node is now refocused.");

  EventUtils.sendKey("UP");
  is(document.commandDispatcher.focusedElement, treeRoot.target,
    "The root node is now refocused.");

  

  EventUtils.sendKey("DOWN");
  is(document.commandDispatcher.focusedElement, fooItem.target,
    "The 'foo' node is now refocused.");

  EventUtils.sendKey("DOWN");
  is(document.commandDispatcher.focusedElement, barItem.target,
    "The 'bar' node is now refocused.");

  EventUtils.sendKey("DOWN");
  is(document.commandDispatcher.focusedElement, bazItem.target,
    "The 'baz' node is now refocused.");

  

  EventUtils.sendKey("LEFT");
  ok(barItem.expanded,
    "The 'bar' node is still expanded.");
  is(document.commandDispatcher.focusedElement, barItem.target,
    "The 'bar' node is now refocused.");

  EventUtils.sendKey("LEFT");
  ok(!barItem.expanded,
    "The 'bar' node is not expanded anymore.");
  is(document.commandDispatcher.focusedElement, barItem.target,
    "The 'bar' node is still focused.");

  EventUtils.sendKey("LEFT");
  ok(treeRoot.expanded,
    "The root node is still expanded.");
  is(document.commandDispatcher.focusedElement, treeRoot.target,
    "The root node is now refocused.");

  EventUtils.sendKey("LEFT");
  ok(!treeRoot.expanded,
    "The root node is not expanded anymore.");
  is(document.commandDispatcher.focusedElement, treeRoot.target,
    "The root node is still focused.");

  

  EventUtils.sendKey("LEFT");
  is(document.commandDispatcher.focusedElement, treeRoot.target,
    "The root node is still focused.");

  

  EventUtils.sendKey("UP");
  is(document.commandDispatcher.focusedElement, treeRoot.target,
    "The root node is still focused.");

  container.remove();
  finish();
});

function MyCustomTreeItem(dataSrc, properties) {
  AbstractTreeItem.call(this, properties);
  this.itemDataSrc = dataSrc;
}

MyCustomTreeItem.prototype = Heritage.extend(AbstractTreeItem.prototype, {
  _displaySelf: function(document, arrowNode) {
    let node = document.createElement("hbox");
    node.MozMarginStart = (this.level * 10) + "px";
    node.appendChild(arrowNode);
    node.appendChild(document.createTextNode(this.itemDataSrc.label));
    return node;
  },
  _populateSelf: function(children) {
    for (let childDataSrc of this.itemDataSrc.children) {
      children.push(new MyCustomTreeItem(childDataSrc, {
        parent: this,
        level: this.level + 1
      }));
    }
  }
});

let gDataSrc = {
  label: "root",
  children: [{
    label: "foo",
    children: []
  }, {
    label: "bar",
    children: [{
      label: "baz",
      children: []
    }]
  }]
};
