


"use strict";




let RecordingsListView = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    this.widget = new SideMenuWidget($("#recordings-list"));

    this._onSelect = this._onSelect.bind(this);
    this._onClearButtonClick = this._onClearButtonClick.bind(this);
    this._onRecordButtonClick = this._onRecordButtonClick.bind(this);
    this._onImportButtonClick = this._onImportButtonClick.bind(this);
    this._onSaveButtonClick = this._onSaveButtonClick.bind(this);

    this.emptyText = L10N.getStr("noRecordingsText");
    this.widget.addEventListener("select", this._onSelect, false);
  },

  


  destroy: function() {
    this.widget.removeEventListener("select", this._onSelect, false);
  },

  





  addEmptyRecording: function(profileLabel) {
    let titleNode = document.createElement("label");
    titleNode.className = "plain recording-item-title";
    titleNode.setAttribute("value", profileLabel ||
      L10N.getFormatStr("recordingsList.itemLabel", this.itemCount + 1));

    let durationNode = document.createElement("label");
    durationNode.className = "plain recording-item-duration";
    durationNode.setAttribute("value",
      L10N.getStr("recordingsList.recordingLabel"));

    let saveNode = document.createElement("label");
    saveNode.className = "plain recording-item-save";
    saveNode.addEventListener("click", this._onSaveButtonClick);

    let hspacer = document.createElement("spacer");
    hspacer.setAttribute("flex", "1");

    let footerNode = document.createElement("hbox");
    footerNode.className = "recording-item-footer";
    footerNode.appendChild(durationNode);
    footerNode.appendChild(hspacer);
    footerNode.appendChild(saveNode);

    let vspacer = document.createElement("spacer");
    vspacer.setAttribute("flex", "1");

    let contentsNode = document.createElement("vbox");
    contentsNode.className = "recording-item";
    contentsNode.setAttribute("flex", "1");
    contentsNode.appendChild(titleNode);
    contentsNode.appendChild(vspacer);
    contentsNode.appendChild(footerNode);

    
    return this.push([contentsNode], {
      attachment: {
        
        
        profilerData: { profileLabel },
        ticksData: null
      }
    });
  },

  





  handleRecordingStarted: function(profileLabel) {
    
    let recordingItem;

    
    
    
    if (profileLabel) {
      recordingItem = this.getItemForAttachment(e =>
        e.profilerData.profileLabel == profileLabel);
    }
    
    if (!recordingItem) {
      recordingItem = this.addEmptyRecording(profileLabel);
    }

    
    recordingItem.isRecording = true;

    
    if (this.itemCount == 1) {
      this.selectedItem = recordingItem;
    }

    window.emit(EVENTS.RECORDING_STARTED, profileLabel);
  },

  





  handleRecordingEnded: function(recordingData) {
    let profileLabel = recordingData.profilerData.profileLabel;
    let recordingItem;

    
    
    
    if (profileLabel) {
      recordingItem = this.getItemForAttachment(e =>
        e.profilerData.profileLabel == profileLabel);
    }
    
    if (!recordingItem) {
      recordingItem = this.getItemForPredicate(e => e.isRecording);
    }

    
    recordingItem.isRecording = false;

    
    this.customizeRecording(recordingItem, recordingData);
    this.forceSelect(recordingItem);

    window.emit(EVENTS.RECORDING_ENDED, recordingData);
  },

  



  handleRecordingCancelled: Task.async(function*() {
    if ($("#record-button").hasAttribute("checked")) {
      $("#record-button").removeAttribute("checked");
      yield gFront.cancelRecording();
    }
    ProfileView.showEmptyNotice();

    window.emit(EVENTS.RECORDING_LOST);
  }),

  







  customizeRecording: function(recordingItem, recordingData) {
    recordingItem.attachment = recordingData;

    let saveNode = $(".recording-item-save", recordingItem.target);
    saveNode.setAttribute("value",
      L10N.getStr("recordingsList.saveLabel"));

    let durationMillis = recordingData.recordingDuration;
    let durationNode = $(".recording-item-duration", recordingItem.target);
    durationNode.setAttribute("value",
      L10N.getFormatStr("recordingsList.durationLabel", durationMillis));
  },

  


  _onSelect: Task.async(function*({ detail: recordingItem }) {
    if (!recordingItem) {
      ProfileView.showEmptyNotice();
      return;
    }
    if (recordingItem.isRecording) {
      ProfileView.showRecordingNotice();
      return;
    }

    ProfileView.showLoadingNotice();
    ProfileView.removeAllTabs();

    let recordingData = recordingItem.attachment;
    let durationMillis = recordingData.recordingDuration;
    yield ProfileView.addTabAndPopulate(recordingData, 0, durationMillis);
    ProfileView.showTabbedBrowser();

    $("#record-button").removeAttribute("checked");
    $("#record-button").removeAttribute("locked");

    window.emit(EVENTS.RECORDING_DISPLAYED);
  }),

  


  _onClearButtonClick: Task.async(function*() {
    this.empty();
    yield this.handleRecordingCancelled();
  }),

  


  _onRecordButtonClick: Task.async(function*() {
    if (!$("#record-button").hasAttribute("checked")) {
      $("#record-button").setAttribute("checked", "true");
      yield gFront.startRecording();
      this.handleRecordingStarted();
    } else {
      $("#record-button").setAttribute("locked", "");
      let recordingData = yield gFront.stopRecording();
      this.handleRecordingEnded(recordingData);
    }
  }),

  


  _onImportButtonClick: Task.async(function*() {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("recordingsList.saveDialogTitle"), Ci.nsIFilePicker.modeOpen);
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogAllFilter"), "*.*");

    if (fp.show() == Ci.nsIFilePicker.returnOK) {
      loadRecordingFromFile(fp.file);
    }
  }),

  


  _onSaveButtonClick: function(e) {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("recordingsList.saveDialogTitle"), Ci.nsIFilePicker.modeSave);
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogAllFilter"), "*.*");
    fp.defaultString = "profile.json";

    fp.open({ done: result => {
      if (result == Ci.nsIFilePicker.returnCancel) {
        return;
      }
      let recordingItem = this.getItemForElement(e.target);
      saveRecordingToFile(recordingItem, fp.file);
    }});
  }
});





function getUnicodeConverter() {
  let className = "@mozilla.org/intl/scriptableunicodeconverter";
  let converter = Cc[className].createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  return converter;
}













function saveRecordingToFile(recordingItem, file) {
  let deferred = promise.defer();

  let recordingData = recordingItem.attachment;
  recordingData.fileType = PROFILE_SERIALIZER_IDENTIFIER;
  recordingData.version = PROFILE_SERIALIZER_VERSION;

  let string = JSON.stringify(recordingData);
  let inputStream = getUnicodeConverter().convertToInputStream(string);
  let outputStream = FileUtils.openSafeFileOutputStream(file);

  NetUtil.asyncCopy(inputStream, outputStream, status => {
    if (!Components.isSuccessCode(status)) {
      deferred.reject(new Error("Could not save recording data file."));
    }
    deferred.resolve();
  });

  return deferred.promise;
}










function loadRecordingFromFile(file) {
  let deferred = promise.defer();

  let channel = NetUtil.newChannel(file);
  channel.contentType = "text/plain";

  NetUtil.asyncFetch(channel, (inputStream, status) => {
    if (!Components.isSuccessCode(status)) {
      deferred.reject(new Error("Could not import recording data file."));
      return;
    }
    try {
      let string = NetUtil.readInputStreamToString(inputStream, inputStream.available());
      var recordingData = JSON.parse(string);
    } catch (e) {
      deferred.reject(new Error("Could not read recording data file."));
      return;
    }
    if (recordingData.fileType != PROFILE_SERIALIZER_IDENTIFIER) {
      deferred.reject(new Error("Unrecognized recording data file."));
      return;
    }

    let profileLabel = recordingData.profilerData.profileLabel;
    let recordingItem = RecordingsListView.addEmptyRecording(profileLabel);
    RecordingsListView.customizeRecording(recordingItem, recordingData);

    
    if (RecordingsListView.itemCount == 1) {
      RecordingsListView.selectedItem = recordingItem;
    }

    deferred.resolve();
  });

  return deferred.promise;
}
