



"use strict";

let { Cu } = require("chrome");
let EventEmitter = require("devtools/shared/event-emitter");

Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const {
  PROFILE_IDLE,
  PROFILE_COMPLETED,
  PROFILE_RUNNING,
  L10N_BUNDLE
} = require("devtools/profiler/consts");

loader.lazyGetter(this, "L10N", () => new ViewHelpers.L10N(L10N_BUNDLE));

function Sidebar(el) {
  EventEmitter.decorate(this);

  this.document = el.ownerDocument;
  this.widget = new SideMenuWidget(el);
  this.widget.notice = L10N.getStr("profiler.sidebarNotice");

  this.widget.addEventListener("select", (ev) => {
    if (!ev.detail)
      return;

    this.emit("select", parseInt(ev.detail.value, 10));
  });
}

Sidebar.prototype = Heritage.extend(WidgetMethods, {
  












  addProfile: function (profile) {
    let doc  = this.document;
    let vbox = doc.createElement("vbox");
    let hbox = doc.createElement("hbox");
    let h3   = doc.createElement("h3");
    let span = doc.createElement("span");
    let save = doc.createElement("a");

    vbox.id = "profile-" + profile.uid;
    vbox.className = "profiler-sidebar-item";

    h3.textContent = profile.name;
    span.setAttribute("flex", 1);
    span.textContent = L10N.getStr("profiler.stateIdle");

    save.textContent = L10N.getStr("profiler.save");
    save.addEventListener("click", (ev) => {
      ev.preventDefault();
      this.emit("save", profile.uid);
    });

    hbox.appendChild(span);
    hbox.appendChild(save);

    vbox.appendChild(h3);
    vbox.appendChild(hbox);

    this.push([vbox, profile.uid], {
      attachment: {
        name:  profile.name,
        state: PROFILE_IDLE
      }
    });
  },

  getElementByProfile: function (profile) {
    return this.document.querySelector("#profile-" + profile.uid);
  },

  getItemByProfile: function (profile) {
    return this.getItemByValue(profile.uid.toString());
  },

  setProfileState: function (profile, state) {
    let item = this.getItemByProfile(profile);
    let label = item.target.querySelector(".profiler-sidebar-item > hbox > span");

    switch (state) {
      case PROFILE_IDLE:
        item.target.setAttribute("state", "idle");
        label.textContent = L10N.getStr("profiler.stateIdle");
        break;
      case PROFILE_RUNNING:
        item.target.setAttribute("state", "running");
        label.textContent = L10N.getStr("profiler.stateRunning");
        break;
      case PROFILE_COMPLETED:
        item.target.setAttribute("state", "completed");
        label.textContent = L10N.getStr("profiler.stateCompleted");
        break;
      default: 
        return;
    }

    item.attachment.state = state;
    this.emit("stateChanged", item);
  }
});

module.exports = Sidebar;