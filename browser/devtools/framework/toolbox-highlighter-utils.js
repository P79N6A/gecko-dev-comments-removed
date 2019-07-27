



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {Promise: promise} = require("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");





















exports.getHighlighterUtils = function(toolbox) {
  if (!toolbox || !toolbox.target) {
    throw new Error("Missing or invalid toolbox passed to getHighlighterUtils");
    return;
  }

  
  let exported = {};

  
  let target = toolbox.target;

  
  let isPicking = false;

  


  exported.release = function() {
    toolbox = target = null;
  }

  





  let isRemoteHighlightable = exported.isRemoteHighlightable = function() {
    return target.client.traits.highlightable;
  }

  


  let supportsCustomHighlighters = exported.supportsCustomHighlighters = () => {
    return !!target.client.traits.customHighlighters;
  };

  






  let isInspectorInitialized = false;
  let requireInspector = generator => {
    return Task.async(function*(...args) {
      if (!isInspectorInitialized) {
        yield toolbox.initInspector();
        isInspectorInitialized = true;
      }
      return yield generator.apply(null, args);
    });
  };

  



  let togglePicker = exported.togglePicker = function() {
    if (isPicking) {
      return stopPicker();
    } else {
      return startPicker();
    }
  }

  








  let startPicker = exported.startPicker = requireInspector(function*() {
    if (isPicking) {
      return;
    }
    isPicking = true;

    toolbox.pickerButtonChecked = true;
    yield toolbox.selectTool("inspector");
    toolbox.on("select", stopPicker);

    if (isRemoteHighlightable()) {
      toolbox.walker.on("picker-node-hovered", onPickerNodeHovered);
      toolbox.walker.on("picker-node-picked", onPickerNodePicked);
      toolbox.walker.on("picker-node-canceled", onPickerNodeCanceled);

      yield toolbox.highlighter.pick();
      toolbox.emit("picker-started");
    } else {
      
      
      
      toolbox.emit("picker-started");
      let node = yield toolbox.walker.pick();
      onPickerNodePicked({node: node});
    }
  });

  





  let stopPicker = exported.stopPicker = requireInspector(function*() {
    if (!isPicking) {
      return;
    }
    isPicking = false;

    toolbox.pickerButtonChecked = false;

    if (isRemoteHighlightable()) {
      yield toolbox.highlighter.cancelPick();
      toolbox.walker.off("picker-node-hovered", onPickerNodeHovered);
      toolbox.walker.off("picker-node-picked", onPickerNodePicked);
      toolbox.walker.off("picker-node-canceled", onPickerNodeCanceled);
    } else {
      
      
      yield toolbox.walker.cancelPick();
    }

    toolbox.off("select", stopPicker);
    toolbox.emit("picker-stopped");
  });

  



  function onPickerNodeHovered(data) {
    toolbox.emit("picker-node-hovered", data.node);
  }

  



  function onPickerNodePicked(data) {
    toolbox.selection.setNodeFront(data.node, "picker-node-picked");
    stopPicker();
  }

  



  function onPickerNodeCanceled() {
    stopPicker();
    toolbox.frame.focus();
  }

  







  let highlightNodeFront = exported.highlightNodeFront = requireInspector(
  function*(nodeFront, options={}) {
    if (!nodeFront) {
      return;
    }

    if (isRemoteHighlightable()) {
      yield toolbox.highlighter.showBoxModel(nodeFront, options);
    } else {
      
      
      yield toolbox.walker.highlight(nodeFront);
    }

    toolbox.emit("node-highlight", nodeFront, options.toSource());
  });

  






  let highlightDomValueGrip = exported.highlightDomValueGrip = requireInspector(
  function*(valueGrip, options={}) {
    let nodeFront = yield gripToNodeFront(valueGrip);
    if (nodeFront) {
      yield highlightNodeFront(nodeFront, options);
    } else {
      throw new Error("The ValueGrip passed could not be translated to a NodeFront");
    }
  });

  




  let gripToNodeFront = exported.gripToNodeFront = requireInspector(
  function*(grip) {
    return yield toolbox.walker.getNodeActorFromObjectActor(grip.actor);
  });

  








  let unhighlight = exported.unhighlight = Task.async(
  function*(forceHide=false) {
    forceHide = forceHide || !gDevTools.testing;

    
    
    if (forceHide && toolbox.highlighter && isRemoteHighlightable()) {
      yield toolbox.highlighter.hideBoxModel();
    }

    toolbox.emit("node-unhighlight");
  });

  








  let getHighlighterByType = exported.getHighlighterByType = requireInspector(
  function*(typeName) {
    let highlighter = null;

    if (supportsCustomHighlighters()) {
      highlighter = yield toolbox.inspector.getHighlighterByType(typeName);
    }

    return highlighter || promise.reject("The target doesn't support " +
        `creating highlighters by types or ${typeName} is unknown`);

  });

  
  return exported;
};
