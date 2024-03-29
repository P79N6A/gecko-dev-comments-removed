




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const NET_STRINGS_URI = "chrome://browser/locale/devtools/netmonitor.properties";
const SVG_NS = "http://www.w3.org/2000/svg";
const PI = Math.PI;
const TAU = PI * 2;
const EPSILON = 0.0000001;
const NAMED_SLICE_MIN_ANGLE = TAU / 8;
const NAMED_SLICE_TEXT_DISTANCE_RATIO = 1.9;
const HOVERED_SLICE_TRANSLATE_DISTANCE_RATIO = 20;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");

this.EXPORTED_SYMBOLS = ["Chart"];




let L10N = new ViewHelpers.L10N(NET_STRINGS_URI);





let Chart = {
  Pie: createPieChart,
  Table: createTableChart,
  PieTable: createPieTableChart
};









function PieChart(node) {
  this.node = node;
  this.slices = new WeakMap();
  EventEmitter.decorate(this);
}









function TableChart(node) {
  this.node = node;
  this.rows = new WeakMap();
  EventEmitter.decorate(this);
}











function PieTableChart(node, pie, table) {
  this.node = node;
  this.pie = pie;
  this.table = table;
  EventEmitter.decorate(this);
}























function createPieTableChart(document, { title, diameter, data, strings, totals, sorted }) {
  if (data && sorted) {
    data = data.slice().sort((a, b) => +(a.size < b.size));
  }

  let pie = Chart.Pie(document, {
    width: diameter,
    data: data
  });

  let table = Chart.Table(document, {
    title: title,
    data: data,
    strings: strings,
    totals: totals
  });

  let container = document.createElement("hbox");
  container.className = "pie-table-chart-container";
  container.appendChild(pie.node);
  container.appendChild(table.node);

  let proxy = new PieTableChart(container, pie, table);

  pie.on("click", (event, item) => {
    proxy.emit(event, item)
  });

  table.on("click", (event, item) => {
    proxy.emit(event, item)
  });

  pie.on("mouseover", (event, item) => {
    proxy.emit(event, item);
    if (table.rows.has(item)) {
      table.rows.get(item).setAttribute("focused", "");
    }
  });

  pie.on("mouseout", (event, item) => {
    proxy.emit(event, item);
    if (table.rows.has(item)) {
      table.rows.get(item).removeAttribute("focused");
    }
  });

  table.on("mouseover", (event, item) => {
    proxy.emit(event, item);
    if (pie.slices.has(item)) {
      pie.slices.get(item).setAttribute("focused", "");
    }
  });

  table.on("mouseout", (event, item) => {
    proxy.emit(event, item);
    if (pie.slices.has(item)) {
      pie.slices.get(item).removeAttribute("focused");
    }
  });

  return proxy;
}




























function createPieChart(document, { data, width, height, centerX, centerY, radius }) {
  height = height || width;
  centerX = centerX || width / 2;
  centerY = centerY || height / 2;
  radius = radius || (width + height) / 4;
  let isPlaceholder = false;

  
  data = data ? data.filter(e => e.size > EPSILON) : null;

  
  if (!data) {
    data = loadingPieChartData;
    isPlaceholder = true;
  }
  if (!data.length) {
    data = emptyPieChartData;
    isPlaceholder = true;
  }

  let container = document.createElementNS(SVG_NS, "svg");
  container.setAttribute("class", "generic-chart-container pie-chart-container");
  container.setAttribute("pack", "center");
  container.setAttribute("flex", "1");
  container.setAttribute("width", width);
  container.setAttribute("height", height);
  container.setAttribute("viewBox", "0 0 " + width + " " + height);
  container.setAttribute("slices", data.length);
  container.setAttribute("placeholder", isPlaceholder);

  let proxy = new PieChart(container);

  let total = data.reduce((acc, e) => acc + e.size, 0);
  let angles = data.map(e => e.size / total * (TAU - EPSILON));
  let largest = data.reduce((a, b) => a.size > b.size ? a : b);
  let smallest = data.reduce((a, b) => a.size < b.size ? a : b);

  let textDistance = radius / NAMED_SLICE_TEXT_DISTANCE_RATIO;
  let translateDistance = radius / HOVERED_SLICE_TRANSLATE_DISTANCE_RATIO;
  let startAngle = TAU;
  let endAngle = 0;
  let midAngle = 0;
  radius -= translateDistance;

  for (let i = data.length - 1; i >= 0; i--) {
    let sliceInfo = data[i];
    let sliceAngle = angles[i];
    if (!sliceInfo.size || sliceAngle < EPSILON) {
      continue;
    }

    endAngle = startAngle - sliceAngle;
    midAngle = (startAngle + endAngle) / 2;

    let x1 = centerX + radius * Math.sin(startAngle);
    let y1 = centerY - radius * Math.cos(startAngle);
    let x2 = centerX + radius * Math.sin(endAngle);
    let y2 = centerY - radius * Math.cos(endAngle);
    let largeArcFlag = Math.abs(startAngle - endAngle) > PI ? 1 : 0;

    let pathNode = document.createElementNS(SVG_NS, "path");
    pathNode.setAttribute("class", "pie-chart-slice chart-colored-blob");
    pathNode.setAttribute("name", sliceInfo.label);
    pathNode.setAttribute("d",
      " M " + centerX + "," + centerY +
      " L " + x2 + "," + y2 +
      " A " + radius + "," + radius +
      " 0 " + largeArcFlag +
      " 1 " + x1 + "," + y1 +
      " Z");

    if (sliceInfo == largest) {
      pathNode.setAttribute("largest", "");
    }
    if (sliceInfo == smallest) {
      pathNode.setAttribute("smallest", "");
    }

    let hoverX = translateDistance * Math.sin(midAngle);
    let hoverY = -translateDistance * Math.cos(midAngle);
    let hoverTransform = "transform: translate(" + hoverX + "px, " + hoverY + "px)";
    pathNode.setAttribute("style", data.length > 1 ? hoverTransform : "");

    proxy.slices.set(sliceInfo, pathNode);
    delegate(proxy, ["click", "mouseover", "mouseout"], pathNode, sliceInfo);
    container.appendChild(pathNode);

    if (sliceInfo.label && sliceAngle > NAMED_SLICE_MIN_ANGLE) {
      let textX = centerX + textDistance * Math.sin(midAngle);
      let textY = centerY - textDistance * Math.cos(midAngle);
      let label = document.createElementNS(SVG_NS, "text");
      label.appendChild(document.createTextNode(sliceInfo.label));
      label.setAttribute("class", "pie-chart-label");
      label.setAttribute("style", data.length > 1 ? hoverTransform : "");
      label.setAttribute("x", data.length > 1 ? textX : centerX);
      label.setAttribute("y", data.length > 1 ? textY : centerY);
      container.appendChild(label);
    }

    startAngle = endAngle;
  }

  return proxy;
}







































function createTableChart(document, { title, data, strings, totals }) {
  strings = strings || {};
  totals = totals || {};
  let isPlaceholder = false;

  
  if (!data) {
    data = loadingTableChartData;
    isPlaceholder = true;
  }
  if (!data.length) {
    data = emptyTableChartData;
    isPlaceholder = true;
  }

  let container = document.createElement("vbox");
  container.className = "generic-chart-container table-chart-container";
  container.setAttribute("pack", "center");
  container.setAttribute("flex", "1");
  container.setAttribute("rows", data.length);
  container.setAttribute("placeholder", isPlaceholder);

  let proxy = new TableChart(container);

  let titleNode = document.createElement("label");
  titleNode.className = "plain table-chart-title";
  titleNode.setAttribute("value", title);
  container.appendChild(titleNode);

  let tableNode = document.createElement("vbox");
  tableNode.className = "plain table-chart-grid";
  container.appendChild(tableNode);

  for (let rowInfo of data) {
    let rowNode = document.createElement("hbox");
    rowNode.className = "table-chart-row";
    rowNode.setAttribute("align", "center");

    let boxNode = document.createElement("hbox");
    boxNode.className = "table-chart-row-box chart-colored-blob";
    boxNode.setAttribute("name", rowInfo.label);
    rowNode.appendChild(boxNode);

    for (let [key, value] in Iterator(rowInfo)) {
      let index = data.indexOf(rowInfo);
      let stringified = strings[key] ? strings[key](value, index) : value;
      let labelNode = document.createElement("label");
      labelNode.className = "plain table-chart-row-label";
      labelNode.setAttribute("name", key);
      labelNode.setAttribute("value", stringified);
      rowNode.appendChild(labelNode);
    }

    proxy.rows.set(rowInfo, rowNode);
    delegate(proxy, ["click", "mouseover", "mouseout"], rowNode, rowInfo);
    tableNode.appendChild(rowNode);
  }

  let totalsNode = document.createElement("vbox");
  totalsNode.className = "table-chart-totals";

  for (let [key, value] in Iterator(totals)) {
    let total = data.reduce((acc, e) => acc + e[key], 0);
    let stringified = totals[key] ? totals[key](total || 0) : total;
    let labelNode = document.createElement("label");
    labelNode.className = "plain table-chart-summary-label";
    labelNode.setAttribute("name", key);
    labelNode.setAttribute("value", stringified);
    totalsNode.appendChild(labelNode);
  }

  container.appendChild(totalsNode);

  return proxy;
}

XPCOMUtils.defineLazyGetter(this, "loadingPieChartData", () => {
  return [{ size: 1, label: L10N.getStr("pieChart.loading") }];
});

XPCOMUtils.defineLazyGetter(this, "emptyPieChartData", () => {
  return [{ size: 1, label: L10N.getStr("pieChart.unavailable") }];
});

XPCOMUtils.defineLazyGetter(this, "loadingTableChartData", () => {
  return [{ size: "", label: L10N.getStr("tableChart.loading") }];
});

XPCOMUtils.defineLazyGetter(this, "emptyTableChartData", () => {
  return [{ size: "", label: L10N.getStr("tableChart.unavailable") }];
});













function delegate(emitter, events, node, args) {
  for (let event of events) {
    node.addEventListener(event, emitter.emit.bind(emitter, event, args));
  }
}
