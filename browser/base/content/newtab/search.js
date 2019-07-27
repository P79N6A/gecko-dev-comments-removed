#ifdef 0



#endif

let gSearch = {
  init: function () {
    document.getElementById("newtab-search-submit")
            .addEventListener("click", e => this._contentSearchController.search(e));
    let textbox = document.getElementById("newtab-search-text");
    this._contentSearchController =
      new ContentSearchUIController(textbox, textbox.parentNode, "newtab", "newtab");
  },
};
