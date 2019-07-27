


"use strict";




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
    this._onRecordSuccess = this._onRecordSuccess.bind(this);
    this._onRecordFailure = this._onRecordFailure.bind(this);
    this._stopRecordingAnimation = this._stopRecordingAnimation.bind(this);

    window.on(EVENTS.SNAPSHOT_RECORDING_FINISHED, this._enableRecordButton);
    this.emptyText = L10N.getStr("noSnapshotsText");
    this.widget.addEventListener("select", this._onSelect, false);
  },

  


  destroy: function() {
    clearNamedTimeout("canvas-actor-recording");
    window.off(EVENTS.SNAPSHOT_RECORDING_FINISHED, this._enableRecordButton);
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

  


  removeLastSnapshot: function () {
    this.removeAt(this.itemCount - 1);
    
    if (this.itemCount === 0) {
      $("#empty-notice").hidden = false;
      $("#waiting-notice").hidden = true;
    }
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
    $("#waiting-notice").hidden = false;

    $("#debugging-pane-contents").hidden = true;
    $("#screenshot-container").hidden = true;
    $("#snapshot-filmstrip").hidden = true;

    Task.spawn(function*() {
      
      
      

      yield DevToolsUtils.waitForTime(SNAPSHOT_DATA_DISPLAY_DELAY);
      CallsListView.showCalls(calls);
      $("#debugging-pane-contents").hidden = false;
      $("#waiting-notice").hidden = true;

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
      $("#waiting-notice").hidden = true;

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

  


  _onRecordButtonClick: function () {
    this._disableRecordButton();

    if (this._recording) {
      this._stopRecordingAnimation();
      return;
    }

    
    
    
    
    this.addSnapshot();

    
    if (this.itemCount == 1) {
      $("#empty-notice").hidden = true;
      $("#waiting-notice").hidden = false;
    }

    this._recordAnimation();
  },

  


  _enableRecordButton: function () {
    $("#record-snapshot").removeAttribute("disabled");
  },

  


  _disableRecordButton: function () {
    $("#record-snapshot").setAttribute("disabled", true);
  },

  


  _recordAnimation: Task.async(function *() {
    if (this._recording) {
      return;
    }
    this._recording = true;
    $("#record-snapshot").setAttribute("checked", "true");

    setNamedTimeout("canvas-actor-recording", CANVAS_ACTOR_RECORDING_ATTEMPT, this._stopRecordingAnimation);

    yield DevToolsUtils.waitForTime(SNAPSHOT_START_RECORDING_DELAY);
    window.emit(EVENTS.SNAPSHOT_RECORDING_STARTED);

    gFront.recordAnimationFrame().then(snapshot => {
      if (snapshot) {
        this._onRecordSuccess(snapshot);
      } else {
        this._onRecordFailure();
      }
    });

    
    
    yield DevToolsUtils.waitForTime(SNAPSHOT_START_RECORDING_DELAY);
    this._enableRecordButton();
  }),

  



  _stopRecordingAnimation: Task.async(function *() {
    clearNamedTimeout("canvas-actor-recording");
    let actorCanStop = yield gTarget.actorHasMethod("canvas", "stopRecordingAnimationFrame");

    if (actorCanStop) {
      yield gFront.stopRecordingAnimationFrame();
    }
    
    
    
    
    else {
      this._onRecordFailure();
    }

    this._recording = false;
    $("#record-snapshot").removeAttribute("checked");
    this._enableRecordButton();
  }),

  


  _onRecordSuccess: Task.async(function *(snapshotActor) {
    
    clearNamedTimeout("canvas-actor-recording");
    let snapshotItem = this.getItemAtIndex(this.itemCount - 1);
    let snapshotOverview = yield snapshotActor.getOverview();
    this.customizeSnapshot(snapshotItem, snapshotActor, snapshotOverview);

    this._recording = false;
    $("#record-snapshot").removeAttribute("checked");

    window.emit(EVENTS.SNAPSHOT_RECORDING_COMPLETED);
    window.emit(EVENTS.SNAPSHOT_RECORDING_FINISHED);
  }),

  


  _onRecordFailure: function () {
    clearNamedTimeout("canvas-actor-recording");
    showNotification(gToolbox, "canvas-debugger-timeout", L10N.getStr("recordingTimeoutFailure"));
    window.emit(EVENTS.SNAPSHOT_RECORDING_CANCELLED);
    window.emit(EVENTS.SNAPSHOT_RECORDING_FINISHED);
    this.removeLastSnapshot();
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

function showNotification (toolbox, name, message) {
  let notificationBox = toolbox.getNotificationBox();
  let notification = notificationBox.getNotificationWithValue(name);
  if (!notification) {
    notificationBox.appendNotification(message, name, "", notificationBox.PRIORITY_WARNING_HIGH);
  }
}
