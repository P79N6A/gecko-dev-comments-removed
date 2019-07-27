


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
const EventEmitter = require("devtools/toolkit/event-emitter");
const { CallWatcherFront } = require("devtools/server/actors/call-watcher");
const { CanvasFront } = require("devtools/server/actors/canvas");
const Telemetry = require("devtools/shared/telemetry");
const telemetry = new Telemetry();

XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DevToolsUtils",
  "resource://gre/modules/devtools/DevToolsUtils.jsm");


const EVENTS = {
  
  UI_RESET: "CanvasDebugger:UIReset",

  
  SNAPSHOTS_LIST_CLEARED: "CanvasDebugger:SnapshotsListCleared",

  
  SNAPSHOT_RECORDING_STARTED: "CanvasDebugger:SnapshotRecordingStarted",
  SNAPSHOT_RECORDING_FINISHED: "CanvasDebugger:SnapshotRecordingFinished",

  
  SNAPSHOT_RECORDING_SELECTED: "CanvasDebugger:SnapshotRecordingSelected",

  
  
  CALL_LIST_POPULATED: "CanvasDebugger:CallListPopulated",

  
  
  CALL_STACK_DISPLAYED: "CanvasDebugger:CallStackDisplayed",

  
  
  CALL_SCREENSHOT_DISPLAYED: "CanvasDebugger:ScreenshotDisplayed",

  
  
  THUMBNAILS_DISPLAYED: "CanvasDebugger:ThumbnailsDisplayed",

  
  SOURCE_SHOWN_IN_JS_DEBUGGER: "CanvasDebugger:SourceShownInJsDebugger",
  SOURCE_NOT_FOUND_IN_JS_DEBUGGER: "CanvasDebugger:SourceNotFoundInJsDebugger"
};

const HTML_NS = "http://www.w3.org/1999/xhtml";
const STRINGS_URI = "chrome://browser/locale/devtools/canvasdebugger.properties";
const SHARED_STRINGS_URI = "chrome://browser/locale/devtools/shared.properties";

const SNAPSHOT_START_RECORDING_DELAY = 10; 
const SNAPSHOT_DATA_EXPORT_MAX_BLOCK = 1000; 
const SNAPSHOT_DATA_DISPLAY_DELAY = 10; 
const SCREENSHOT_DISPLAY_DELAY = 100; 
const STACK_FUNC_INDENTATION = 14; 





const CALLS_LIST_SERIALIZER_IDENTIFIER = "Recorded Animation Frame Snapshot";
const CALLS_LIST_SERIALIZER_VERSION = 1;
const CALLS_LIST_SLOW_SAVE_DELAY = 100; 




let gToolbox, gTarget, gFront;




function startupCanvasDebugger() {
  return promise.all([
    EventsHandler.initialize(),
    SnapshotsListView.initialize(),
    CallsListView.initialize()
  ]);
}




function shutdownCanvasDebugger() {
  return promise.all([
    EventsHandler.destroy(),
    SnapshotsListView.destroy(),
    CallsListView.destroy()
  ]);
}




let EventsHandler = {
  


  initialize: function() {
    telemetry.toolOpened("canvasdebugger");
    this._onTabNavigated = this._onTabNavigated.bind(this);
    gTarget.on("will-navigate", this._onTabNavigated);
    gTarget.on("navigate", this._onTabNavigated);
  },

  


  destroy: function() {
    telemetry.toolClosed("canvasdebugger");
    gTarget.off("will-navigate", this._onTabNavigated);
    gTarget.off("navigate", this._onTabNavigated);
  },

  


  _onTabNavigated: function(event) {
    if (event != "will-navigate") {
      return;
    }
    
    gFront.setup({ reload: false });

    
    SnapshotsListView.empty();
    CallsListView.empty();

    $("#record-snapshot").removeAttribute("checked");
    $("#record-snapshot").removeAttribute("disabled");
    $("#record-snapshot").hidden = false;

    $("#reload-notice").hidden = true;
    $("#empty-notice").hidden = false;
    $("#import-notice").hidden = true;

    $("#debugging-pane-contents").hidden = true;
    $("#screenshot-container").hidden = true;
    $("#snapshot-filmstrip").hidden = true;

    window.emit(EVENTS.UI_RESET);
  }
};




let SnapshotsListView = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    this.widget = new SideMenuWidget($("#snapshots-list"), {
      showArrows: true
    });

    this._onSelect = this._onSelect.bind(this);
    this._onClearButtonClick = this._onClearButtonClick.bind(this);
    this._onRecordButtonClick = this._onRecordButtonClick.bind(this);
    this._onImportButtonClick = this._onImportButtonClick.bind(this);
    this._onSaveButtonClick = this._onSaveButtonClick.bind(this);

    this.emptyText = L10N.getStr("noSnapshotsText");
    this.widget.addEventListener("select", this._onSelect, false);
  },

  


  destroy: function() {
    this.widget.removeEventListener("select", this._onSelect, false);
  },

  





  addSnapshot: function() {
    let contents = document.createElement("hbox");
    contents.className = "snapshot-item";

    let thumbnail = document.createElementNS(HTML_NS, "canvas");
    thumbnail.className = "snapshot-item-thumbnail";
    thumbnail.width = CanvasFront.THUMBNAIL_SIZE;
    thumbnail.height = CanvasFront.THUMBNAIL_SIZE;

    let title = document.createElement("label");
    title.className = "plain snapshot-item-title";
    title.setAttribute("value",
      L10N.getFormatStr("snapshotsList.itemLabel", this.itemCount + 1));

    let calls = document.createElement("label");
    calls.className = "plain snapshot-item-calls";
    calls.setAttribute("value",
      L10N.getStr("snapshotsList.loadingLabel"));

    let save = document.createElement("label");
    save.className = "plain snapshot-item-save";
    save.addEventListener("click", this._onSaveButtonClick, false);

    let spacer = document.createElement("spacer");
    spacer.setAttribute("flex", "1");

    let footer = document.createElement("hbox");
    footer.className = "snapshot-item-footer";
    footer.appendChild(save);

    let details = document.createElement("vbox");
    details.className = "snapshot-item-details";
    details.appendChild(title);
    details.appendChild(calls);
    details.appendChild(spacer);
    details.appendChild(footer);

    contents.appendChild(thumbnail);
    contents.appendChild(details);

    
    return this.push([contents], {
      attachment: {
        
        
        actor: null,
        calls: null,
        thumbnails: null,
        screenshot: null
      }
    });
  },

  









  customizeSnapshot: function(snapshotItem, snapshotActor, snapshotOverview) {
    
    
    snapshotItem.attachment.actor = snapshotActor;
    let functionCalls = snapshotItem.attachment.calls = snapshotOverview.calls;
    let thumbnails = snapshotItem.attachment.thumbnails = snapshotOverview.thumbnails;
    let screenshot = snapshotItem.attachment.screenshot = snapshotOverview.screenshot;

    let lastThumbnail = thumbnails[thumbnails.length - 1];
    let { width, height, flipped, pixels } = lastThumbnail;

    let thumbnailNode = $(".snapshot-item-thumbnail", snapshotItem.target);
    thumbnailNode.setAttribute("flipped", flipped);
    drawImage(thumbnailNode, width, height, pixels, { centered: true });

    let callsNode = $(".snapshot-item-calls", snapshotItem.target);
    let drawCalls = functionCalls.filter(e => CanvasFront.DRAW_CALLS.has(e.name));

    let drawCallsStr = PluralForm.get(drawCalls.length,
      L10N.getStr("snapshotsList.drawCallsLabel"));
    let funcCallsStr = PluralForm.get(functionCalls.length,
      L10N.getStr("snapshotsList.functionCallsLabel"));

    callsNode.setAttribute("value",
      drawCallsStr.replace("#1", drawCalls.length) + ", " +
      funcCallsStr.replace("#1", functionCalls.length));

    let saveNode = $(".snapshot-item-save", snapshotItem.target);
    saveNode.setAttribute("disabled", !!snapshotItem.isLoadedFromDisk);
    saveNode.setAttribute("value", snapshotItem.isLoadedFromDisk
      ? L10N.getStr("snapshotsList.loadedLabel")
      : L10N.getStr("snapshotsList.saveLabel"));

    
    if (!this.selectedItem) {
      this.selectedIndex = 0;
    }
  },

  


  _onSelect: function({ detail: snapshotItem }) {
    if (!snapshotItem) {
      return;
    }
    let { calls, thumbnails, screenshot } = snapshotItem.attachment;

    $("#reload-notice").hidden = true;
    $("#empty-notice").hidden = true;
    $("#import-notice").hidden = false;

    $("#debugging-pane-contents").hidden = true;
    $("#screenshot-container").hidden = true;
    $("#snapshot-filmstrip").hidden = true;

    Task.spawn(function*() {
      
      
      

      yield DevToolsUtils.waitForTime(SNAPSHOT_DATA_DISPLAY_DELAY);
      CallsListView.showCalls(calls);
      $("#debugging-pane-contents").hidden = false;
      $("#import-notice").hidden = true;

      yield DevToolsUtils.waitForTime(SNAPSHOT_DATA_DISPLAY_DELAY);
      CallsListView.showThumbnails(thumbnails);
      $("#snapshot-filmstrip").hidden = false;

      yield DevToolsUtils.waitForTime(SNAPSHOT_DATA_DISPLAY_DELAY);
      CallsListView.showScreenshot(screenshot);
      $("#screenshot-container").hidden = false;

      window.emit(EVENTS.SNAPSHOT_RECORDING_SELECTED);
    });
  },

  


  _onClearButtonClick: function() {
    Task.spawn(function*() {
      SnapshotsListView.empty();
      CallsListView.empty();

      $("#reload-notice").hidden = true;
      $("#empty-notice").hidden = true;
      $("#import-notice").hidden = true;

      if (yield gFront.isInitialized()) {
        $("#empty-notice").hidden = false;
      } else {
        $("#reload-notice").hidden = false;
      }

      $("#debugging-pane-contents").hidden = true;
      $("#screenshot-container").hidden = true;
      $("#snapshot-filmstrip").hidden = true;

      window.emit(EVENTS.SNAPSHOTS_LIST_CLEARED);
    });
  },

  


  _onRecordButtonClick: function() {
    Task.spawn(function*() {
      $("#record-snapshot").setAttribute("checked", "true");
      $("#record-snapshot").setAttribute("disabled", "true");

      
      
      
      
      let snapshotItem = this.addSnapshot();

      
      if (this.itemCount == 1) {
        $("#empty-notice").hidden = true;
        $("#import-notice").hidden = false;
      }

      yield DevToolsUtils.waitForTime(SNAPSHOT_START_RECORDING_DELAY);
      window.emit(EVENTS.SNAPSHOT_RECORDING_STARTED);

      let snapshotActor = yield gFront.recordAnimationFrame();
      let snapshotOverview = yield snapshotActor.getOverview();
      this.customizeSnapshot(snapshotItem, snapshotActor, snapshotOverview);

      $("#record-snapshot").removeAttribute("checked");
      $("#record-snapshot").removeAttribute("disabled");

      window.emit(EVENTS.SNAPSHOT_RECORDING_FINISHED);
    }.bind(this));
  },

  


  _onImportButtonClick: function() {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("snapshotsList.saveDialogTitle"), Ci.nsIFilePicker.modeOpen);
    fp.appendFilter(L10N.getStr("snapshotsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("snapshotsList.saveDialogAllFilter"), "*.*");

    if (fp.show() != Ci.nsIFilePicker.returnOK) {
      return;
    }

    let channel = NetUtil.newChannel2(fp.file,
                                      null,
                                      null,
                                      window.document,
                                      null, 
                                      null, 
                                      Ci.nsILoadInfo.SEC_NORMAL,
                                      Ci.nsIContentPolicy.TYPE_OTHER);
    channel.contentType = "text/plain";

    NetUtil.asyncFetch2(channel, (inputStream, status) => {
      if (!Components.isSuccessCode(status)) {
        console.error("Could not import recorded animation frame snapshot file.");
        return;
      }
      try {
        let string = NetUtil.readInputStreamToString(inputStream, inputStream.available());
        var data = JSON.parse(string);
      } catch (e) {
        console.error("Could not read animation frame snapshot file.");
        return;
      }
      if (data.fileType != CALLS_LIST_SERIALIZER_IDENTIFIER) {
        console.error("Unrecognized animation frame snapshot file.");
        return;
      }

      
      
      let snapshotItem = this.addSnapshot();
      snapshotItem.isLoadedFromDisk = true;
      data.calls.forEach(e => e.isLoadedFromDisk = true);

      this.customizeSnapshot(snapshotItem, data.calls, data);
    });
  },

  


  _onSaveButtonClick: function(e) {
    let snapshotItem = this.getItemForElement(e.target);

    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("snapshotsList.saveDialogTitle"), Ci.nsIFilePicker.modeSave);
    fp.appendFilter(L10N.getStr("snapshotsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("snapshotsList.saveDialogAllFilter"), "*.*");
    fp.defaultString = "snapshot.json";

    
    
    let serialized = Task.spawn(function*() {
      let data = {
        fileType: CALLS_LIST_SERIALIZER_IDENTIFIER,
        version: CALLS_LIST_SERIALIZER_VERSION,
        calls: [],
        thumbnails: [],
        screenshot: null
      };
      let functionCalls = snapshotItem.attachment.calls;
      let thumbnails = snapshotItem.attachment.thumbnails;
      let screenshot = snapshotItem.attachment.screenshot;

      
      yield DevToolsUtils.yieldingEach(functionCalls, (call, i) => {
        let { type, name, file, line, argsPreview, callerPreview } = call;
        return call.getDetails().then(({ stack }) => {
          data.calls[i] = {
            type: type,
            name: name,
            file: file,
            line: line,
            stack: stack,
            argsPreview: argsPreview,
            callerPreview: callerPreview
          };
        });
      });

      
      yield DevToolsUtils.yieldingEach(thumbnails, (thumbnail, i) => {
        let { index, width, height, flipped, pixels } = thumbnail;
        data.thumbnails.push({ index, width, height, flipped, pixels });
      });

      
      let { index, width, height, flipped, pixels } = screenshot;
      data.screenshot = { index, width, height, flipped, pixels };

      let string = JSON.stringify(data);
      let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
        createInstance(Ci.nsIScriptableUnicodeConverter);

      converter.charset = "UTF-8";
      return converter.convertToInputStream(string);
    });

    
    
    fp.open({ done: result => {
      if (result == Ci.nsIFilePicker.returnCancel) {
        return;
      }
      let footer = $(".snapshot-item-footer", snapshotItem.target);
      let save = $(".snapshot-item-save", snapshotItem.target);

      
      setNamedTimeout("call-list-save", CALLS_LIST_SLOW_SAVE_DELAY, () => {
        footer.classList.add("devtools-throbber");
        save.setAttribute("disabled", "true");
        save.setAttribute("value", L10N.getStr("snapshotsList.savingLabel"));
      });

      serialized.then(inputStream => {
        let outputStream = FileUtils.openSafeFileOutputStream(fp.file);

        NetUtil.asyncCopy(inputStream, outputStream, status => {
          if (!Components.isSuccessCode(status)) {
            console.error("Could not save recorded animation frame snapshot file.");
          }
          clearNamedTimeout("call-list-save");
          footer.classList.remove("devtools-throbber");
          save.removeAttribute("disabled");
          save.setAttribute("value", L10N.getStr("snapshotsList.saveLabel"));
        });
      });
    }});
  }
});





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

      return name.contains(lowerCaseSearchToken) ||
             file.contains(lowerCaseSearchToken) ||
             line.contains(lowerCaseSearchToken) ||
             args.contains(lowerCaseSearchToken);
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
        viewSourceInDebugger(file, line);
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
    viewSourceInDebugger(file, line);
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
    viewSourceInDebugger(file, line);
  },

  


  _onStepOut: function() {
    this.selectedIndex = this.itemCount - 1;
  }
});




let L10N = new ViewHelpers.L10N(STRINGS_URI);
let SHARED_L10N = new ViewHelpers.L10N(SHARED_STRINGS_URI);




EventEmitter.decorate(this);




function $(selector, target = document) target.querySelector(selector);
function $all(selector, target = document) target.querySelectorAll(selector);




function nsIURL(url, store = nsIURL.store) {
  if (store.has(url)) {
    return store.get(url);
  }
  let uri = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
  store.set(url, uri);
  return uri;
}


nsIURL.store = new Map();




function getFileName(url) {
  try {
    let { fileName } = nsIURL(url);
    return fileName || "/";
  } catch (e) {
    
    return "";
  }
}















function getImageDataStorage(ctx, w, h) {
  let storage = getImageDataStorage.cache;
  if (storage && storage.width == w && storage.height == h) {
    return storage;
  }
  return getImageDataStorage.cache = ctx.createImageData(w, h);
}


getImageDataStorage.cache = null;





















function drawImage(canvas, width, height, pixels, options = {}) {
  let ctx = canvas.getContext("2d");

  
  
  if (pixels.length <= 1) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    return;
  }

  let imageData = getImageDataStorage(ctx, width, height);
  imageData.data.set(pixels);

  if (options.centered) {
    let left = (canvas.width - width) / 2;
    let top = (canvas.height - height) / 2;
    ctx.putImageData(imageData, left, top);
  } else {
    ctx.putImageData(imageData, 0, 0);
  }
}














function drawBackground(id, width, height, pixels) {
  let canvas = document.createElementNS(HTML_NS, "canvas");
  canvas.width = width;
  canvas.height = height;

  drawImage(canvas, width, height, pixels);
  document.mozSetImageElement(id, canvas);

  
  if (window._onMozSetImageElement) {
    window._onMozSetImageElement(pixels);
  }
}




function getNextDrawCall(calls, call) {
  for (let i = calls.indexOf(call) + 1, len = calls.length; i < len; i++) {
    let nextCall = calls[i];
    let name = nextCall.attachment.actor.name;
    if (CanvasFront.DRAW_CALLS.has(name)) {
      return nextCall;
    }
  }
  return null;
}





function getScreenshotFromCallLoadedFromDisk(calls, call) {
  for (let i = calls.indexOf(call); i >= 0; i--) {
    let prevCall = calls[i];
    let screenshot = prevCall.screenshot;
    if (screenshot) {
      return screenshot;
    }
  }
  return CanvasFront.INVALID_SNAPSHOT_IMAGE;
}




function getThumbnailForCall(thumbnails, index) {
  for (let i = thumbnails.length - 1; i >= 0; i--) {
    let thumbnail = thumbnails[i];
    if (thumbnail.index <= index) {
      return thumbnail;
    }
  }
  return CanvasFront.INVALID_SNAPSHOT_IMAGE;
}





function viewSourceInDebugger(url, line) {
  let showSource = ({ DebuggerView }) => {
    let item = DebuggerView.Sources.getItemForAttachment(a => a.source.url === url);
    if (item) {
      DebuggerView.setEditorLocation(item.attachment.source.actor, line, { noDebug: true }).then(() => {
        window.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER);
      }, () => {
        window.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER);
      });
    }
  }

  
  
  
  let debuggerAlreadyOpen = gToolbox.getPanel("jsdebugger");
  gToolbox.selectTool("jsdebugger").then(({ panelWin: dbg }) => {
    if (debuggerAlreadyOpen) {
      showSource(dbg);
    } else {
      dbg.once(dbg.EVENTS.SOURCES_ADDED, () => showSource(dbg));
    }
  });
}
