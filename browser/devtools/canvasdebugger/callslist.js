


"use strict";





let CallsListView = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    this.widget = new SideMenuWidget($("#calls-list"));
    this._slider = $("#calls-slider");
    this._searchbox = $("#calls-searchbox");
    this._filmstrip = $("#snapshot-filmstrip");

    this._onSelect = this._onSelect.bind(this);
    this._onSlideMouseDown = this._onSlideMouseDown.bind(this);
    this._onSlideMouseUp = this._onSlideMouseUp.bind(this);
    this._onSlide = this._onSlide.bind(this);
    this._onSearch = this._onSearch.bind(this);
    this._onScroll = this._onScroll.bind(this);
    this._onExpand = this._onExpand.bind(this);
    this._onStackFileClick = this._onStackFileClick.bind(this);
    this._onThumbnailClick = this._onThumbnailClick.bind(this);

    this.widget.addEventListener("select", this._onSelect, false);
    this._slider.addEventListener("mousedown", this._onSlideMouseDown, false);
    this._slider.addEventListener("mouseup", this._onSlideMouseUp, false);
    this._slider.addEventListener("change", this._onSlide, false);
    this._searchbox.addEventListener("input", this._onSearch, false);
    this._filmstrip.addEventListener("wheel", this._onScroll, false);
  },

  


  destroy: function() {
    this.widget.removeEventListener("select", this._onSelect, false);
    this._slider.removeEventListener("mousedown", this._onSlideMouseDown, false);
    this._slider.removeEventListener("mouseup", this._onSlideMouseUp, false);
    this._slider.removeEventListener("change", this._onSlide, false);
    this._searchbox.removeEventListener("input", this._onSearch, false);
    this._filmstrip.removeEventListener("wheel", this._onScroll, false);
  },

  





  showCalls: function(functionCalls) {
    this.empty();

    for (let i = 0, len = functionCalls.length; i < len; i++) {
      let call = functionCalls[i];

      let view = document.createElement("vbox");
      view.className = "call-item-view devtools-monospace";
      view.setAttribute("flex", "1");

      let contents = document.createElement("hbox");
      contents.className = "call-item-contents";
      contents.setAttribute("align", "center");
      contents.addEventListener("dblclick", this._onExpand);
      view.appendChild(contents);

      let index = document.createElement("label");
      index.className = "plain call-item-index";
      index.setAttribute("flex", "1");
      index.setAttribute("value", i + 1);

      let gutter = document.createElement("hbox");
      gutter.className = "call-item-gutter";
      gutter.appendChild(index);
      contents.appendChild(gutter);

      
      
      if (call.callerPreview) {
        let context = document.createElement("label");
        context.className = "plain call-item-context";
        context.setAttribute("value", call.callerPreview);
        contents.appendChild(context);

        let separator = document.createElement("label");
        separator.className = "plain call-item-separator";
        separator.setAttribute("value", ".");
        contents.appendChild(separator);
      }

      let name = document.createElement("label");
      name.className = "plain call-item-name";
      name.setAttribute("value", call.name);
      contents.appendChild(name);

      let argsPreview = document.createElement("label");
      argsPreview.className = "plain call-item-args";
      argsPreview.setAttribute("crop", "end");
      argsPreview.setAttribute("flex", "100");
      
      if (call.type == CallWatcherFront.METHOD_FUNCTION) {
        argsPreview.setAttribute("value", "(" + call.argsPreview + ")");
      } else {
        argsPreview.setAttribute("value", " = " + call.argsPreview);
      }
      contents.appendChild(argsPreview);

      let location = document.createElement("label");
      location.className = "plain call-item-location";
      location.setAttribute("value", getFileName(call.file) + ":" + call.line);
      location.setAttribute("crop", "start");
      location.setAttribute("flex", "1");
      location.addEventListener("mousedown", this._onExpand);
      contents.appendChild(location);

      
      this.push([view], {
        staged: true,
        attachment: {
          actor: call
        }
      });

      
      
      if (CanvasFront.DRAW_CALLS.has(call.name)) {
        view.setAttribute("draw-call", "");
      }
      if (CanvasFront.INTERESTING_CALLS.has(call.name)) {
        view.setAttribute("interesting-call", "");
      }
    }

    
    this.commit();
    window.emit(EVENTS.CALL_LIST_POPULATED);

    
    
    
    this._ignoreSliderChanges = true;
    this._slider.value = 0;
    this._slider.max = functionCalls.length - 1;
    this._ignoreSliderChanges = false;
  },

  






  showScreenshot: function(screenshot) {
    let { index, width, height, scaling, flipped, pixels } = screenshot;

    let screenshotNode = $("#screenshot-image");
    screenshotNode.setAttribute("flipped", flipped);
    drawBackground("screenshot-rendering", width, height, pixels);

    let dimensionsNode = $("#screenshot-dimensions");
    let actualWidth = (width / scaling) | 0;
    let actualHeight = (height / scaling) | 0;
    dimensionsNode.setAttribute("value",
      SHARED_L10N.getFormatStr("dimensions", actualWidth, actualHeight));

    window.emit(EVENTS.CALL_SCREENSHOT_DISPLAYED);
  },

  






  showThumbnails: function(thumbnails) {
    while (this._filmstrip.hasChildNodes()) {
      this._filmstrip.firstChild.remove();
    }
    for (let thumbnail of thumbnails) {
      this.appendThumbnail(thumbnail);
    }

    window.emit(EVENTS.THUMBNAILS_DISPLAYED);
  },

  






  appendThumbnail: function(thumbnail) {
    let { index, width, height, flipped, pixels } = thumbnail;

    let thumbnailNode = document.createElementNS(HTML_NS, "canvas");
    thumbnailNode.setAttribute("flipped", flipped);
    thumbnailNode.width = Math.max(CanvasFront.THUMBNAIL_SIZE, width);
    thumbnailNode.height = Math.max(CanvasFront.THUMBNAIL_SIZE, height);
    drawImage(thumbnailNode, width, height, pixels, { centered: true });

    thumbnailNode.className = "filmstrip-thumbnail";
    thumbnailNode.onmousedown = e => this._onThumbnailClick(e, index);
    thumbnailNode.setAttribute("index", index);
    this._filmstrip.appendChild(thumbnailNode);
  },

  







  set highlightedThumbnail(index) {
    let currHighlightedThumbnail = $(".filmstrip-thumbnail[index='" + index + "']");
    if (currHighlightedThumbnail == null) {
      return;
    }

    let prevIndex = this._highlightedThumbnailIndex
    let prevHighlightedThumbnail = $(".filmstrip-thumbnail[index='" + prevIndex + "']");
    if (prevHighlightedThumbnail) {
      prevHighlightedThumbnail.removeAttribute("highlighted");
    }

    currHighlightedThumbnail.setAttribute("highlighted", "");
    currHighlightedThumbnail.scrollIntoView();
    this._highlightedThumbnailIndex = index;
  },

  



  get highlightedThumbnail() {
    return this._highlightedThumbnailIndex;
  },

  


  _onSelect: function({ detail: callItem }) {
    if (!callItem) {
      return;
    }

    
    
    if (this.selectedIndex == this.itemCount - 1) {
      $("#resume").setAttribute("disabled", "true");
      $("#step-over").setAttribute("disabled", "true");
      $("#step-out").setAttribute("disabled", "true");
    } else {
      $("#resume").removeAttribute("disabled");
      $("#step-over").removeAttribute("disabled");
      $("#step-out").removeAttribute("disabled");
    }

    
    
    this._ignoreSliderChanges = true;
    this._slider.value = this.selectedIndex;
    this._ignoreSliderChanges = false;

    
    
    if (callItem.attachment.actor.isLoadedFromDisk) {
      return;
    }

    
    
    
    setConditionalTimeout("screenshot-display", SCREENSHOT_DISPLAY_DELAY, () => {
      return !this._isSliding;
    }, () => {
      let frameSnapshot = SnapshotsListView.selectedItem.attachment.actor
      let functionCall = callItem.attachment.actor;
      frameSnapshot.generateScreenshotFor(functionCall).then(screenshot => {
        this.showScreenshot(screenshot);
        this.highlightedThumbnail = screenshot.index;
      }).catch(Cu.reportError);
    });
  },

  


  _onSlideMouseDown: function() {
    this._isSliding = true;
  },

  


  _onSlideMouseUp: function() {
    this._isSliding = false;
  },

  


  _onSlide: function() {
    
    if (this._ignoreSliderChanges) {
      return;
    }
    let selectedFunctionCallIndex = this.selectedIndex = this._slider.value;

    
    
    let thumbnails = SnapshotsListView.selectedItem.attachment.thumbnails;
    let thumbnail = getThumbnailForCall(thumbnails, selectedFunctionCallIndex);

    
    
    if (thumbnail.index == this.highlightedThumbnail) {
      return;
    }
    
    
    if (thumbnail.index == -1) {
      thumbnail = thumbnails[0];
    }

    let { index, width, height, flipped, pixels } = thumbnail;
    this.highlightedThumbnail = index;

    let screenshotNode = $("#screenshot-image");
    screenshotNode.setAttribute("flipped", flipped);
    drawBackground("screenshot-rendering", width, height, pixels);
  },

  


  _onSearch: function(e) {
    let lowerCaseSearchToken = this._searchbox.value.toLowerCase();

    this.filterContents(e => {
      let call = e.attachment.actor;
      let name = call.name.toLowerCase();
      let file = call.file.toLowerCase();
      let line = call.line.toString().toLowerCase();
      let args = call.argsPreview.toLowerCase();

      return name.includes(lowerCaseSearchToken) ||
             file.includes(lowerCaseSearchToken) ||
             line.includes(lowerCaseSearchToken) ||
             args.includes(lowerCaseSearchToken);
    });
  },

  


  _onScroll: function(e) {
    this._filmstrip.scrollLeft += e.deltaX;
  },

  



  _onExpand: function(e) {
    let callItem = this.getItemForElement(e.target);
    let view = $(".call-item-view", callItem.target);

    
    
    
    if (view.hasAttribute("call-stack-populated")) {
      let isExpanded = view.getAttribute("call-stack-expanded") == "true";

      
      if (e.target.classList.contains("call-item-location")) {
        let { file, line } = callItem.attachment.actor;
        this._viewSourceInDebugger(file, line);
        return;
      }
      
      else {
        view.setAttribute("call-stack-expanded", !isExpanded);
        $(".call-item-stack", view).hidden = isExpanded;
        return;
      }
    }

    let list = document.createElement("vbox");
    list.className = "call-item-stack";
    view.setAttribute("call-stack-populated", "");
    view.setAttribute("call-stack-expanded", "true");
    view.appendChild(list);

    


    let display = stack => {
      for (let i = 1; i < stack.length; i++) {
        let call = stack[i];

        let contents = document.createElement("hbox");
        contents.className = "call-item-stack-fn";
        contents.style.MozPaddingStart = (i * STACK_FUNC_INDENTATION) + "px";

        let name = document.createElement("label");
        name.className = "plain call-item-stack-fn-name";
        name.setAttribute("value", "â†³ " + call.name + "()");
        contents.appendChild(name);

        let spacer = document.createElement("spacer");
        spacer.setAttribute("flex", "100");
        contents.appendChild(spacer);

        let location = document.createElement("label");
        location.className = "plain call-item-stack-fn-location";
        location.setAttribute("value", getFileName(call.file) + ":" + call.line);
        location.setAttribute("crop", "start");
        location.setAttribute("flex", "1");
        location.addEventListener("mousedown", e => this._onStackFileClick(e, call));
        contents.appendChild(location);

        list.appendChild(contents);
      }

      window.emit(EVENTS.CALL_STACK_DISPLAYED);
    };

    
    
    let functionCall = callItem.attachment.actor;
    if (functionCall.isLoadedFromDisk) {
      display(functionCall.stack);
    }
    
    else {
      callItem.attachment.actor.getDetails().then(fn => display(fn.stack));
    }
  },

  







  _onStackFileClick: function(e, { file, line }) {
    this._viewSourceInDebugger(file, line);
  },

  





  _onThumbnailClick: function(e, index) {
    this.selectedIndex = index;
  },

  


  _onResume: function() {
    
    let drawCall = getNextDrawCall(this.items, this.selectedItem);
    if (drawCall) {
      this.selectedItem = drawCall;
      return;
    }

    
    this._onStepOut();
  },

  


  _onStepOver: function() {
    this.selectedIndex++;
  },

  


  _onStepIn: function() {
    if (this.selectedIndex == -1) {
      this._onResume();
      return;
    }
    let callItem = this.selectedItem;
    let { file, line } = callItem.attachment.actor;
    this._viewSourceInDebugger(file, line);
  },

  


  _onStepOut: function() {
    this.selectedIndex = this.itemCount - 1;
  },

  


  _viewSourceInDebugger: function (file, line) {
    gToolbox.viewSourceInDebugger(file, line).then(success => {
      if (success) {
        window.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER);
      } else {
        window.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER);
      }
    });
  }
});
