





































function getMetricsService() {
  return Components.classes["@mozilla.org/metrics/service;1"]
    .getService(Components.interfaces.nsIMetricsService);
}

function createEventItem(ns, name) {
  return getMetricsService().createEventItem(ns, name);
}

const EventNS = "http://www.mozilla.org/metrics";

function buildItemChildren(item) {
  var items = [];
  var child;
  for (var i = 0; i < 10; ++i) {
    child = createEventItem(EventNS, "child" + i);
    items.push(child);
    item.appendChild(child);
  }
  return items;
}

function compareItemChildren(item, children) {
  do_check_eq(item.childCount, children.length);
  for (var i = 0; i < children.length; ++i) {
    do_check_eq(item.childAt(i), children[i]);
  }
}
