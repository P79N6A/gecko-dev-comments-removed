



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var gSearchPane = {

  init: function ()
  {
    if (!Services.prefs.getBoolPref("browser.search.showOneOffButtons")) {
      document.getElementById("category-search").hidden = true;
      if (document.location.hash == "#search")
        document.location.hash = "";
      return;
    }

    let list = document.getElementById("defaultEngine");
    let currentEngine = Services.search.currentEngine.name;
    Services.search.getVisibleEngines().forEach(e => {
      let item = list.appendItem(e.name);
      item.setAttribute("class", "menuitem-iconic");
      if (e.iconURI)
        item.setAttribute("image", e.iconURI.spec);
      item.engine = e;
      if (e.name == currentEngine)
        list.selectedItem = item;
    });

    this.displayOneClickEnginesList();

    document.getElementById("oneClickProvidersList")
            .addEventListener("CheckboxStateChange", gSearchPane.saveOneClickEnginesList);
  },

  displayOneClickEnginesList: function () {
    let richlistbox = document.getElementById("oneClickProvidersList");
    let pref = document.getElementById("browser.search.hiddenOneOffs").value;
    let hiddenList = pref ? pref.split(",") : [];

    while (richlistbox.firstChild)
      richlistbox.firstChild.remove();

    let currentEngine = Services.search.currentEngine.name;
    Services.search.getVisibleEngines().forEach(e => {
      if (e.name == currentEngine)
        return;

      let item = document.createElement("richlistitem");
      item.setAttribute("label", e.name);
      if (hiddenList.indexOf(e.name) == -1)
        item.setAttribute("checked", "true");
      if (e.iconURI)
        item.setAttribute("src", e.iconURI.spec);
      richlistbox.appendChild(item);
    });
  },

  saveOneClickEnginesList: function () {
    let richlistbox = document.getElementById("oneClickProvidersList");
    let hiddenList = [];
    for (let child of richlistbox.childNodes) {
      if (!child.checked)
        hiddenList.push(child.getAttribute("label"));
    }
    document.getElementById("browser.search.hiddenOneOffs").value =
      hiddenList.join(",");
  },

  setDefaultEngine: function () {
    Services.search.currentEngine =
      document.getElementById("defaultEngine").selectedItem.engine;
    this.displayOneClickEnginesList();
  }
};
