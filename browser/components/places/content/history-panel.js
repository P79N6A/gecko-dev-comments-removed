




Components.utils.import("resource://gre/modules/TelemetryStopwatch.jsm");

var gHistoryTree;
var gSearchBox;
var gHistoryGrouping = "";
var gSearching = false;

function HistorySidebarInit()
{
  gHistoryTree = document.getElementById("historyTree");
  gSearchBox = document.getElementById("search-box");

  gHistoryGrouping = document.getElementById("viewButton").
                              getAttribute("selectedsort");

  if (gHistoryGrouping == "site")
    document.getElementById("bysite").setAttribute("checked", "true");
  else if (gHistoryGrouping == "visited") 
    document.getElementById("byvisited").setAttribute("checked", "true");
  else if (gHistoryGrouping == "lastvisited")
    document.getElementById("bylastvisited").setAttribute("checked", "true");
  else if (gHistoryGrouping == "dayandsite")
    document.getElementById("bydayandsite").setAttribute("checked", "true");
  else
    document.getElementById("byday").setAttribute("checked", "true");
  
  searchHistory("");
}

function GroupBy(groupingType)
{
  gHistoryGrouping = groupingType;
  searchHistory(gSearchBox.value);
}

function searchHistory(aInput)
{
  var query = PlacesUtils.history.getNewQuery();
  var options = PlacesUtils.history.getNewQueryOptions();

  const NHQO = Ci.nsINavHistoryQueryOptions;
  var sortingMode;
  var resultType;

  switch (gHistoryGrouping) {
    case "visited":
      resultType = NHQO.RESULTS_AS_URI;
      sortingMode = NHQO.SORT_BY_VISITCOUNT_DESCENDING;
      break;
    case "lastvisited":
      resultType = NHQO.RESULTS_AS_URI;
      sortingMode = NHQO.SORT_BY_DATE_DESCENDING;
      break;
    case "dayandsite":
      resultType = NHQO.RESULTS_AS_DATE_SITE_QUERY;
      break;
    case "site":
      resultType = NHQO.RESULTS_AS_SITE_QUERY;
      sortingMode = NHQO.SORT_BY_TITLE_ASCENDING;
      break;
    case "day":
    default:
      resultType = NHQO.RESULTS_AS_DATE_QUERY;
      break;
  }

  if (aInput) {
    query.searchTerms = aInput;
    if (gHistoryGrouping != "visited" && gHistoryGrouping != "lastvisited") {
      sortingMode = NHQO.SORT_BY_FRECENCY_DESCENDING;
      resultType = NHQO.RESULTS_AS_URI;
    }
  }

  options.sortingMode = sortingMode;
  options.resultType = resultType;
  options.includeHidden = !!aInput;

  if (gHistoryGrouping == "lastvisited")
    this.TelemetryStopwatch.start("HISTORY_LASTVISITED_TREE_QUERY_TIME_MS");

  
  
  
  gHistoryTree.load([query], options);

  if (gHistoryGrouping == "lastvisited")
    this.TelemetryStopwatch.finish("HISTORY_LASTVISITED_TREE_QUERY_TIME_MS");
}

window.addEventListener("SidebarFocused",
                        function()
                          gSearchBox.focus(),
                        false);
