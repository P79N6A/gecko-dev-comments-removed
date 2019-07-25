







































const NO_STUDIES_IMG = "chrome://testpilot/skin/testPilot_200x200.png";
const PROPOSE_STUDY_URL =
  "https://wiki.mozilla.org/Labs/Test_Pilot#For_researchers";

var TestPilotXulWindow = {
  _stringBundle : null,

  onSubmitButton: function(experimentId) {
    Components.utils.import("resource://testpilot/modules/setup.js");
    let task = TestPilotSetup.getTaskById(experimentId);
    let button = document.getElementById("submit-button-" + task.id);

    
    let parent = button.parentNode;
    while (parent.firstChild) {
      parent.removeChild(parent.firstChild);
    }
    
    this.addLabel(
      parent,
      this._stringBundle.getString("testpilot.studiesWindow.uploading"));
    let self = this;

    task.upload( function(success) {
      while (parent.firstChild) {
        parent.removeChild(parent.firstChild);
      }
      if (success) {
        self.addThanksMessage(parent);
        
      } else {
        
        self.addLabel(
          parent,
          self._stringBundle.getString(
            "testpilot.studiesWindow.unableToReachServer"));
      }
    });

  },

  addThanksMessage: function(container) {
    
    let hbox = document.createElement("hbox");
    container.appendChild(this.makeSpacer());
    container.appendChild(hbox);
    this.addLabel(
      container,
      this._stringBundle.getString(
        "testpilot.studiesWindow.thanksForContributing"));
    container.appendChild(this.makeSpacer());
    hbox.appendChild(this.makeSpacer());
    this.addImg(hbox, "study-submitted");
    hbox.appendChild(this.makeSpacer());
  },

  addXulLink: function (container, text, url, openInTab) {
    let linkContainer = document.createElement("hbox");
    let link = document.createElement("label");
    let spacer = document.createElement("spacer");
    link.setAttribute("value", text);
    link.setAttribute("class", "text-link");
    if (openInTab) {
      link.setAttribute(
        "onclick",
        "if (event.button==0) { " +
        "TestPilotWindowUtils.openInTab('" + url + "'); }");
    } else {
      link.setAttribute(
        "onclick",
        "if (event.button==0) { " +
        "TestPilotWindowUtils.openChromeless('" + url + "'); }");
    }
    linkContainer.appendChild(link);
    spacer.setAttribute("flex", "1");
    linkContainer.appendChild(spacer);
    container.appendChild(linkContainer);
  },

  addLabel: function(container, text, styleClass) {
    let label = document.createElement("label");
    label.setAttribute("value", text);
    if (styleClass) {
      label.setAttribute("class", styleClass);
    }
    container.appendChild(label);
  },

  addImg: function(container, iconClass) {
    let newImg = document.createElement("image");
    newImg.setAttribute("class", iconClass);
    container.appendChild(newImg);
  },

  makeSpacer: function() {
    let spacer = document.createElement("spacer");
    spacer.setAttribute("flex", "1");
    return spacer;
  },

  addThumbnail: function(container, imgUrl) {
    let boundingBox = document.createElement("vbox");
    boundingBox.setAttribute("class", "results-thumbnail");
    let bBox2 = document.createElement("hbox");

    boundingBox.appendChild(this.makeSpacer());
    boundingBox.appendChild(bBox2);
    boundingBox.appendChild(this.makeSpacer());

    bBox2.appendChild(this.makeSpacer());
    let newImg = document.createElement("image");
    newImg.setAttribute("src", imgUrl);
    newImg.setAttribute("class", "results-thumbnail");
    bBox2.appendChild(newImg);
    bBox2.appendChild(this.makeSpacer());

    container.appendChild(boundingBox);
  },

  addProgressBar: function(container, percent) {
    let progBar = document.createElement("progressmeter");
    progBar.setAttribute("mode", "determined");
    progBar.setAttribute("value", Math.ceil(percent).toString());
    container.appendChild(progBar);
  },

  addDescription: function(container, title, paragraph) {
    let desc = document.createElement("description");
    desc.setAttribute("class", "study-title");
    let txtNode = document.createTextNode(title);
    desc.appendChild(txtNode);
    container.appendChild(desc);

    desc = document.createElement("description");
    desc.setAttribute("class", "study-description");
    desc.setAttribute("crop", "none");
    txtNode = document.createTextNode(paragraph);
    desc.appendChild(txtNode);
    container.appendChild(desc);
  },

  addButton: function(container, label, id, onClickHandler) {
    let button = document.createElement("button");
    button.setAttribute("label", label);
    button.setAttribute("id", id);
    button.setAttribute("oncommand", onClickHandler);
    container.appendChild(button);
  },

  _sortNewestFirst: function(experiments) {
    experiments.sort(
      function sortFunc(a, b) {
        if (a.endDate && b.endDate) {
          return b.endDate - a.endDate;
        }
        if (a.publishDate && b.publishDate) {
          if (isNaN(a.publishDate) || isNaN(b.publishDate)) {
            return 0;
          }
          return b.publishDate - a.publishDate;
        }
        return 0;
      });
    return experiments;
  },

  onLoad: function () {
    Components.utils.import("resource://testpilot/modules/Observers.js");
    Components.utils.import("resource://testpilot/modules/setup.js");
    Components.utils.import("resource://testpilot/modules/tasks.js");

    this._stringBundle = document.getElementById("testpilot-stringbundle");
    this.sizeWindow();
    this._init(false);
    Observers.add("testpilot:task:changed", this._onTaskStatusChanged, this);
  },

  onUnload: function() {
    Observers.remove("testpilot:task:changed", this._onTaskStatusChanged, this);
  },

  _onTaskStatusChanged : function() {
    this._init(true);
  },

  onTakeSurveyButton: function(taskId) {
    let task = TestPilotSetup.getTaskById(taskId);
    TestPilotWindowUtils.openChromeless(task.defaultUrl);
    task.onDetailPageOpened();
  },

  _init: function(aReload) {
    let experiments;
    let ready = false;

    
    if (TestPilotSetup.startupComplete) {
      experiments = TestPilotSetup.getAllTasks();
      if (experiments.length > 0 ) {
        ready = true;
      }
    }

    if (!ready) {
      
      
      window.setTimeout(
        function() { TestPilotXulWindow._init(aReload); }, 2000);
      return;
    }

    let numFinishedStudies = 0;
    let numCurrentStudies = 0;

    
    let msg = window.document.getElementById("still-loading-msg");
    msg.setAttribute("hidden", "true");

    if (aReload) {
      

      let listboxIds =
        ["current-studies-listbox", "finished-studies-listbox",
         "study-results-listbox"];
      for (let i = 0; i < listboxIds.length; i++) {
        let listbox = document.getElementById(listboxIds[i]);

        while (listbox.lastChild) {
          listbox.removeChild(listbox.lastChild);
        }
      }
    }

    experiments = this._sortNewestFirst(experiments);

    for (let i = 0; i < experiments.length; i++) {
      let task = experiments[i];
      let newRow = document.createElement("richlistitem");
      newRow.setAttribute("class", "tp-study-list");

      this.addThumbnail(newRow, task.thumbnail);

      let textVbox = document.createElement("vbox");
      newRow.appendChild(textVbox);

      let openInTab = (task.taskType == TaskConstants.TYPE_LEGACY);

      this.addDescription(textVbox, task.title, task.summary);
      this.addXulLink(
        textVbox, this._stringBundle.getString("testpilot.moreInfo"),
        task.defaultUrl, openInTab);

      
      let statusVbox = document.createElement("vbox");
      if (task.status == TaskConstants.STATUS_FINISHED) {
        this.addLabel(
          statusVbox,
          this._stringBundle.getFormattedString(
            "testpilot.studiesWindow.finishedOn",
            [(new Date(task.endDate)).toLocaleDateString()]));
        this.addButton(statusVbox,
          this._stringBundle.getString("testpilot.submit"),
          "submit-button-" + task.id,
          "TestPilotXulWindow.onSubmitButton(" + task.id + ");");
      }
      if (task.status == TaskConstants.STATUS_CANCELLED) {
        let hbox = document.createElement("hbox");
        newRow.setAttribute("class", "tp-opted-out");
        statusVbox.appendChild(this.makeSpacer());
        statusVbox.appendChild(hbox);
        this.addLabel(
          statusVbox,
          this._stringBundle.getString("testpilot.studiesWindow.canceledStudy"));
        statusVbox.appendChild(this.makeSpacer());
        hbox.appendChild(this.makeSpacer());
        this.addImg(hbox, "study-canceled");
        hbox.appendChild(this.makeSpacer());
      }
      if (task.status == TaskConstants.STATUS_NEW ||
          task.status == TaskConstants.STATUS_PENDING ) {
        newRow.setAttribute("class", "tp-new-results");

        if (task.taskType == TaskConstants.TYPE_SURVEY) {
          this.addButton(
            statusVbox,
            this._stringBundle.getString("testpilot.takeSurvey"),
            "survey-button",
            "TestPilotXulWindow.onTakeSurveyButton('" + task.id + "');");
        } else if (task.taskType == TaskConstants.TYPE_EXPERIMENT) {
          if (task.startDate) {
            this.addLabel(
              statusVbox,
              this._stringBundle.getFormattedString(
                "testpilot.studiesWindow.willStart",
                [(new Date(task.startDate)).toLocaleDateString()]));
          }
        }
      }
      if (task.status == TaskConstants.STATUS_IN_PROGRESS ||
          task.status == TaskConstants.STATUS_STARTING) {

        if (task.taskType == TaskConstants.TYPE_SURVEY) {
          this.addButton(
            statusVbox,
            this._stringBundle.getString("testpilot.takeSurvey"),
            "survey-button",
            "TestPilotXulWindow.onTakeSurveyButton('" + task.id + "');");
        } else if (task.taskType == TaskConstants.TYPE_EXPERIMENT) {
          this.addLabel(
            statusVbox,
            this._stringBundle.getString(
             "testpilot.studiesWindow.gatheringData"));
             let now = (new Date()).getTime();
          let progress =
            100 * (now - task.startDate) / (task.endDate - task.startDate);
          this.addProgressBar(statusVbox, progress);
          this.addLabel(
            statusVbox,
            this._stringBundle.getFormattedString(
              "testpilot.studiesWindow.willFinish",
              [(new Date(task.endDate)).toLocaleDateString()]));
        }
      }
      if (task.status >= TaskConstants.STATUS_SUBMITTED) {
        if (task.taskType == TaskConstants.TYPE_RESULTS) {
          let maintask = TestPilotSetup.getTaskById(task.relatedStudyId);
          if (maintask && maintask.status >= TaskConstants.STATUS_SUBMITTED) {
            this.addThanksMessage(statusVbox);
          }
        } else {
          if (task.status == TaskConstants.STATUS_MISSED) {
            
            let hbox = document.createElement("hbox");
            newRow.setAttribute("class", "tp-opted-out");
            statusVbox.appendChild(this.makeSpacer());
            statusVbox.appendChild(hbox);
            this.addLabel(
              statusVbox,
              this._stringBundle.getString("testpilot.studiesWindow.missedStudy"));
            statusVbox.appendChild(this.makeSpacer());
            hbox.appendChild(this.makeSpacer());
            this.addImg(hbox, "study-missed");
            hbox.appendChild(this.makeSpacer());
          } else {
            this.addThanksMessage(statusVbox);
            numFinishedStudies ++;
          }
        }
      }
      let spacer = document.createElement("spacer");
      spacer.setAttribute("flex", "1");
      newRow.appendChild(spacer);
      newRow.appendChild(statusVbox);

      
      let rowset;
      if (task.taskType == TaskConstants.TYPE_RESULTS) {
        rowset = document.getElementById("study-results-listbox");
      } else if (task.status > TaskConstants.STATUS_FINISHED) {
        rowset = document.getElementById("finished-studies-listbox");
      } else {
        rowset = document.getElementById("current-studies-listbox");
        numCurrentStudies++;
      }

      
      rowset.appendChild(newRow);
    }

    
    
    if (numCurrentStudies == 0) {
      let newRow = document.createElement("richlistitem");
      newRow.setAttribute("class", "tp-study-list");
      this.addThumbnail(newRow, NO_STUDIES_IMG);
      let textVbox = document.createElement("vbox");
      textVbox.setAttribute("class", "pilot-largetext");
      newRow.appendChild(textVbox);
      this.addDescription(
        textVbox, "",
        this._stringBundle.getString("testpilot.studiesWindow.noStudies"));
      this.addXulLink(
        textVbox,
        this._stringBundle.getString("testpilot.studiesWindow.proposeStudy"),
        PROPOSE_STUDY_URL, true);
      document.getElementById("current-studies-listbox").appendChild(newRow);
    }

    
    document.getElementById("num-finished-badge").setAttribute(
      "value", numFinishedStudies);
  },

  sizeWindow: function() {
    
    
    let currList = document.getElementById("current-studies-listbox");
    let finList = document.getElementById("finished-studies-listbox");
    let resultsList = document.getElementById("study-results-listbox");

    let screenWidth = window.screen.availWidth;
    let screenHeight = window.screen.availHeight;
    let width = screenWidth >= 800 ? 700 : screenWidth - 100;
    let height = screenHeight >= 800 ? 700 : screenHeight - 100;

    height -= 130; 

    currList.width = width;
    currList.height = height;
    finList.width = width;
    finList.height = height;
    resultsList.width = width;
    resultsList.height = height;
    window.sizeToContent();
  },

  focusPane: function(paneIndex) {
    document.getElementById("tp-xulwindow-deck").selectedIndex = paneIndex;

    
    
    
    if (paneIndex == 2) {
      Components.utils.import("resource://testpilot/modules/setup.js");
      Components.utils.import("resource://testpilot/modules/tasks.js");

      let experiments = TestPilotSetup.getAllTasks();
      for each (let experiment in experiments) {
        if (experiment.taskType == TaskConstants.TYPE_RESULTS) {
          if (experiment.status == TaskConstants.STATUS_NEW) {
            experiment.changeStatus(TaskConstants.STATUS_ARCHIVED, true);
          }
        }
      }
    }
  }
};
