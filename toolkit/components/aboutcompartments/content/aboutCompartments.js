





"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm", {});




const MEASURES = [
  {key: "totalUserTime", label: "Total user (µs)"},
  {key: "totalSystemTime", label: "Total system (µs)"},
  {key: "ownUserTime", label: "Own user (µs)"},
  {key: "ownSystemTime", label: "Own system (µs)"},
  {key: "cpowTime", label: "CPOW (µs)"},
  {key: "visits", label: "Activations"},
];








function Owner(id, kind, name) {
  for (let {key} of MEASURES) {
    this[key] = 0;
  }
  this.compartments = [];
  this.id = id;
  this.kind = kind;
  this.name = name;
}
Owner.prototype = {
  add: function(compartment) {
    this.compartments.push(compartment);
    for (let {key} of MEASURES) {
      this[key] += compartment[key];
    }
  },
  promiseName: function() {
    if (this.kind != "<addon>") {
      return Promise.resolve(this.name);
    }
    return new Promise(resolve =>
      AddonManager.getAddonByID(this.id, a =>
        resolve(a?a.name:null)
      )
    );
  }
};

function getStatistics() {
  let compartmentInfo = Cc["@mozilla.org/compartment-info;1"]
          .getService(Ci.nsICompartmentInfo);
  let compartments = compartmentInfo.getCompartments();
  let count = compartments.length;
  let data = {};
  for (let i = 0; i < count; i++) {
    let compartment = compartments.queryElementAt(i, Ci.nsICompartment);
    let kind, id, name;
    if (!compartment.isSystem) {
      name = id = compartment.compartmentName;
      kind = "<page>";
    } else if (compartment.addonId == "<non-addon>") {
      id = kind = name = "<built-in>";
    } else {
      name = id = compartment.addonId;
      kind = "<addon>";
    }
    let key = kind + ":" + id;
    let owner = data[key];
    if (!owner) {
      owner = data[key] = new Owner(id, kind, name);
    }
    owner.add(compartment);
  }
  return [data[k] for (k of Object.keys(data))].sort((a, b) => a.totalUserTime <= b.totalUserTime);
}

function update() {
  try {
    console.log("Updating");

    
    Cu.stopwatchMonitoring = true;

    let dataElt = document.getElementById("data");
    dataElt.innerHTML = "";

    
    let headerElt = document.createElement("tr");
    dataElt.appendChild(headerElt);
    for (let column of [...MEASURES, {key:"compartments", name: "Compartments"}, {key: "name", name: ""}]) {
      let el = document.createElement("td");
      el.classList.add(column.key);
      el.classList.add("header");
      el.textContent = column.label;
      headerElt.appendChild(el);
    }

    
    let data = getStatistics();
    console.log("Data", data);
    for (let item of data) {
      
      
      let show = false;
      for (let column of MEASURES) {
        if (item[column.key]) {
          show = true;
        }
      }
      if (!show) {
        continue;
      }

      let row = document.createElement("tr");
      row.classList.add(item.kind);
      dataElt.appendChild(row);

      
      for (let column of MEASURES) {
        let el = document.createElement("td");
        el.classList.add(column.key);
        el.classList.add("contents");
        el.textContent = item[column.key];
        row.appendChild(el);
      }

      
      let el = document.createElement("td");
      el.classList.add("contents");
      el.classList.add("compartments");
      el.textContent = item.compartments.length;
      row.appendChild(el);

      
      el = document.createElement("td");
      el.classList.add("contents");
      el.classList.add("name");
      row.appendChild(el);
      item.promiseName().then(name => {
        name ? el.textContent = name : item.id;
      });
    }
  } catch (ex) {
    console.error(ex);
  }
}

function stop() {
  Cu.stopwatchMonitoring = false;
}

function go() {
  update();
  window.setInterval(update, 5000);
  window.addEventListener("beforeunload", stop);
}
