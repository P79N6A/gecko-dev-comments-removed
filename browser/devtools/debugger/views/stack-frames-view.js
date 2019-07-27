


"use strict";




function StackFramesView(DebuggerController, DebuggerView) {
  dumpn("StackFramesView was instantiated");

  this.StackFrames = DebuggerController.StackFrames;
  this.DebuggerView = DebuggerView;

  this._onStackframeRemoved = this._onStackframeRemoved.bind(this);
  this._onSelect = this._onSelect.bind(this);
  this._onScroll = this._onScroll.bind(this);
  this._afterScroll = this._afterScroll.bind(this);
}

StackFramesView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the StackFramesView");

    this.widget = new BreadcrumbsWidget(document.getElementById("stackframes"));
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("scroll", this._onScroll, true);
    window.addEventListener("resize", this._onScroll, true);

    this.autoFocusOnFirstItem = false;
    this.autoFocusOnSelection = false;

    
    this._mirror = this.DebuggerView.StackFramesClassicList;
  },

  


  destroy: function() {
    dumpn("Destroying the StackFramesView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("scroll", this._onScroll, true);
    window.removeEventListener("resize", this._onScroll, true);
  },

  













  addFrame: function(aTitle, aUrl, aLine, aDepth, aIsBlackBoxed) {
    
    
    if (aIsBlackBoxed) {
      if (this._prevBlackBoxedUrl == aUrl) {
        return;
      }
      this._prevBlackBoxedUrl = aUrl;
    } else {
      this._prevBlackBoxedUrl = null;
    }

    
    let frameView = this._createFrameView.apply(this, arguments);

    
    this.push([frameView], {
      index: 0, 
      attachment: {
        title: aTitle,
        url: aUrl,
        line: aLine,
        depth: aDepth
      },
      
      
      finalize: this._onStackframeRemoved
    });

    
    this._mirror.addFrame(aTitle, aUrl, aLine, aDepth);
  },

  



  set selectedDepth(aDepth) {
    this.selectedItem = aItem => aItem.attachment.depth == aDepth;
  },

  






  get selectedDepth() {
    return this.selectedItem.attachment.depth;
  },

  


  dirty: false,

  















  _createFrameView: function(aTitle, aUrl, aLine, aDepth, aIsBlackBoxed) {
    let container = document.createElement("hbox");
    container.id = "stackframe-" + aDepth;
    container.className = "dbg-stackframe";

    let frameDetails = SourceUtils.trimUrlLength(
      SourceUtils.getSourceLabel(aUrl),
      STACK_FRAMES_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_SOURCE_URL_TRIM_SECTION);

    if (aIsBlackBoxed) {
      container.classList.add("dbg-stackframe-black-boxed");
    } else {
      let frameTitleNode = document.createElement("label");
      frameTitleNode.className = "plain dbg-stackframe-title breadcrumbs-widget-item-tag";
      frameTitleNode.setAttribute("value", aTitle);
      container.appendChild(frameTitleNode);

      frameDetails += SEARCH_LINE_FLAG + aLine;
    }

    let frameDetailsNode = document.createElement("label");
    frameDetailsNode.className = "plain dbg-stackframe-details breadcrumbs-widget-item-id";
    frameDetailsNode.setAttribute("value", frameDetails);
    container.appendChild(frameDetailsNode);

    return container;
  },

  





  _onStackframeRemoved: function(aItem) {
    dumpn("Finalizing stackframe item: " + aItem.stringify());

    
    let depth = aItem.attachment.depth;
    this._mirror.remove(this._mirror.getItemForAttachment(e => e.depth == depth));

    
    this._prevBlackBoxedUrl = null;
  },

  


  _onSelect: function(e) {
    let stackframeItem = this.selectedItem;
    if (stackframeItem) {
      
      let depth = stackframeItem.attachment.depth;

      
      this.suppressSelectionEvents = true;
      this._mirror.selectedItem = e => e.attachment.depth == depth;
      this.suppressSelectionEvents = false;

      DebuggerController.StackFrames.selectFrame(depth);
    }
  },

  


  _onScroll: function() {
    
    if (!this.dirty) {
      return;
    }
    
    setNamedTimeout("stack-scroll", STACK_FRAMES_SCROLL_DELAY, this._afterScroll);
  },

  


  _afterScroll: function() {
    let scrollPosition = this.widget.getAttribute("scrollPosition");
    let scrollWidth = this.widget.getAttribute("scrollWidth");

    
    
    if (scrollPosition - scrollWidth / 10 < 1) {
      this.ensureIndexIsVisible(CALL_STACK_PAGE_SIZE - 1);
      this.dirty = false;

      
      DebuggerController.StackFrames.addMoreFrames();
    }
  },

  _mirror: null,
  _prevBlackBoxedUrl: null
});

DebuggerView.StackFrames = new StackFramesView(DebuggerController, DebuggerView);
