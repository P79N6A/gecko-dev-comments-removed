





"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm", {});
const { PerformanceStats } = Cu.import("resource://gre/modules/PerformanceStats.jsm", {});




const MEASURES = [
  {key: "longestDuration", percentOfDeltaT: false, label: "Jank level"},
  {key: "totalUserTime", percentOfDeltaT: true, label: "User (%)"},
  {key: "totalSystemTime", percentOfDeltaT: true, label: "System (%)"},
  {key: "totalCPOWTime", percentOfDeltaT: true, label: "Cross-Process (%)"},
  {key: "ticks", percentOfDeltaT: false, label: "Activations"},
];

let State = {
  


  _processData: null,
  




  _componentsData: new Map(),

  


  _date: window.performance.now(),

  









  update: function() {
    let snapshot = PerformanceStats.getSnapshot();
    let newData = new Map();
    let deltas = [];
    for (let componentNew of snapshot.componentsData) {
      let {name, addonId, isSystem} = componentNew;
      let key = JSON.stringify({name, addonId, isSystem});
      let componentOld = State._componentsData.get(key);
      deltas.push(componentNew.substract(componentOld));
      newData.set(key, componentNew);
    }
    State._componentsData = newData;
    let now = window.performance.now();
    let result = {
      components: deltas.filter(x => x.ticks > 0),
      process: snapshot.processData.substract(State._processData),
      deltaT: now - State._date
    };
    result.components.sort((a, b) => {
      if (a.longestDuration < b.longestDuration) {
        return true;
      }
      if (a.longestDuration == b.longestDuration) {
        return a.totalUserTime <= b.totalUserTime
      }
      return false;
    });
    State._processData = snapshot.processData;
    State._date = now;
    return result;
  }
};


function update() {
  try {
    let dataElt = document.getElementById("data");
    dataElt.innerHTML = "";

    
    let headerElt = document.createElement("tr");
    dataElt.appendChild(headerElt);
    headerElt.classList.add("header");
    for (let column of [...MEASURES, {key: "name", name: ""}]) {
      let el = document.createElement("td");
      el.classList.add(column.key);
      el.textContent = column.label;
      headerElt.appendChild(el);
    }

    let deltas = State.update();

    for (let item of deltas.components) {
      let row = document.createElement("tr");
      if (item.addonId) {
        row.classList.add("addon");
      } else if (item.isSystem) {
        row.classList.add("platform");
      } else {
        row.classList.add("content");
      }
      dataElt.appendChild(row);

      
      for (let {key, percentOfDeltaT} of MEASURES) {
        let el = document.createElement("td");
        el.classList.add(key);
        el.classList.add("contents");
        row.appendChild(el);

        let value = percentOfDeltaT ? Math.round(item[key] / deltas.deltaT) : item[key];
        if (key == "longestDuration") {
          value += 1;
          el.classList.add("jank" + value);
        }
        el.textContent = value;
      }

      
      let el = document.createElement("td");
      let id = item.id;
      el.classList.add("contents");
      el.classList.add("name");
      row.appendChild(el);
      if (item.addonId) {
        let _el = el;
        let _item = item;
        AddonManager.getAddonByID(item.addonId, a => {
          _el.textContent = a?a.name:_item.name
        });
      } else {
        el.textContent = item.name;
      }
    }
  } catch (ex) {
    console.error(ex);
  }
}

function go() {
  
  
  State.update();

  window.setTimeout(() => {
    window.setInterval(update, 10000);
  }, 1000);
}
