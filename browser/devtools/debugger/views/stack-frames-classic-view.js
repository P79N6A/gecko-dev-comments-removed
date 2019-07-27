


"use strict";





function StackFramesClassicListView(DebuggerController, DebuggerView) {
  dumpn("StackFramesClassicListView was instantiated");

  this.DebuggerView = DebuggerView;
  this._onSelect = this._onSelect.bind(this);
}

StackFramesClassicListView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the StackFramesClassicListView");

    this.widget = new SideMenuWidget(document.getElementById("callstack-list"));
    this.widget.addEventListener("select", this._onSelect, false);

    this.emptyText = L10N.getStr("noStackFramesText");
    this.autoFocusOnFirstItem = false;
    this.autoFocusOnSelection = false;

    
    this._mirror = this.DebuggerView.StackFrames;
  },

  


  destroy: function() {
    dumpn("Destroying the StackFramesClassicListView");

    this.widget.removeEventListener("select", this._onSelect, false);
  },

  











  addFrame: function(aTitle, aUrl, aLine, aDepth) {
    
    let frameView = this._createFrameView.apply(this, arguments);

    
    this.push([frameView], {
      attachment: {
        depth: aDepth
      }
    });
  },

  













  _createFrameView: function(aTitle, aUrl, aLine, aDepth) {
    let container = document.createElement("hbox");
    container.id = "classic-stackframe-" + aDepth;
    container.className = "dbg-classic-stackframe";
    container.setAttribute("flex", "1");

    let frameTitleNode = document.createElement("label");
    frameTitleNode.className = "plain dbg-classic-stackframe-title";
    frameTitleNode.setAttribute("value", aTitle);
    frameTitleNode.setAttribute("crop", "center");

    let frameDetailsNode = document.createElement("hbox");
    frameDetailsNode.className = "plain dbg-classic-stackframe-details";

    let frameUrlNode = document.createElement("label");
    frameUrlNode.className = "plain dbg-classic-stackframe-details-url";
    frameUrlNode.setAttribute("value", SourceUtils.getSourceLabel(aUrl));
    frameUrlNode.setAttribute("crop", "center");
    frameDetailsNode.appendChild(frameUrlNode);

    let frameDetailsSeparator = document.createElement("label");
    frameDetailsSeparator.className = "plain dbg-classic-stackframe-details-sep";
    frameDetailsSeparator.setAttribute("value", SEARCH_LINE_FLAG);
    frameDetailsNode.appendChild(frameDetailsSeparator);

    let frameLineNode = document.createElement("label");
    frameLineNode.className = "plain dbg-classic-stackframe-details-line";
    frameLineNode.setAttribute("value", aLine);
    frameDetailsNode.appendChild(frameLineNode);

    container.appendChild(frameTitleNode);
    container.appendChild(frameDetailsNode);

    return container;
  },

  


  _onSelect: function(e) {
    let stackframeItem = this.selectedItem;
    if (stackframeItem) {
      
      
      let depth = stackframeItem.attachment.depth;
      this._mirror.selectedItem = e => e.attachment.depth == depth;
    }
  },

  _mirror: null
});

DebuggerView.StackFramesClassicList = new StackFramesClassicListView(DebuggerController,
                                                                     DebuggerView);
