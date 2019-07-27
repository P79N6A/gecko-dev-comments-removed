



"use strict";






const EventEmitter = require("devtools/toolkit/event-emitter");
const { Cu } = require("chrome");
const { ViewHelpers } = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});
const STRINGS_URI = "chrome://browser/locale/devtools/filterwidget.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);

const DEFAULT_FILTER_TYPE = "length";
const UNIT_MAPPING = {
  percentage: "%",
  length: "px",
  angle: "deg",
  string: ""
};

const FAST_VALUE_MULTIPLIER = 10;
const SLOW_VALUE_MULTIPLIER = 0.1;
const DEFAULT_VALUE_MULTIPLIER = 1;

const LIST_PADDING = 7;
const LIST_ITEM_HEIGHT = 32;

const filterList = [
  {
    "name": "blur",
    "range": [0, Infinity],
    "type": "length"
  },
  {
    "name": "brightness",
    "range": [0, Infinity],
    "type": "percentage"
  },
  {
    "name": "contrast",
    "range": [0, Infinity],
    "type": "percentage"
  },
  {
    "name": "drop-shadow",
    "placeholder": L10N.getStr("dropShadowPlaceholder"),
    "type": "string"
  },
  {
    "name": "grayscale",
    "range": [0, 100],
    "type": "percentage"
  },
  {
    "name": "hue-rotate",
    "range": [0, 360],
    "type": "angle"
  },
  {
    "name": "invert",
    "range": [0, 100],
    "type": "percentage"
  },
  {
    "name": "opacity",
    "range": [0, 100],
    "type": "percentage"
  },
  {
    "name": "saturate",
    "range": [0, Infinity],
    "type": "percentage"
  },
  {
    "name": "sepia",
    "range": [0, 100],
    "type": "percentage"
  },
  {
    "name": "url",
    "placeholder": "example.svg#c1",
    "type": "string"
  }
];
















function CSSFilterEditorWidget(el, value = "") {
  this.doc = el.ownerDocument;
  this.win = this.doc.ownerGlobal;
  this.el = el;

  this._addButtonClick = this._addButtonClick.bind(this);
  this._removeButtonClick = this._removeButtonClick.bind(this);
  this._mouseMove = this._mouseMove.bind(this);
  this._mouseUp = this._mouseUp.bind(this);
  this._mouseDown = this._mouseDown.bind(this);
  this._input = this._input.bind(this);

  this._initMarkup();
  this._buildFilterItemMarkup();
  this._addEventListeners();

  EventEmitter.decorate(this);

  this.filters = [];
  this.setCssValue(value);
}

exports.CSSFilterEditorWidget = CSSFilterEditorWidget;

CSSFilterEditorWidget.prototype = {
  _initMarkup: function() {
    const list = this.el.querySelector(".filters");
    this.el.appendChild(list);
    this.el.insertBefore(list, this.el.firstChild);
    this.container = this.el;
    this.list = list;

    this.filterSelect = this.el.querySelector("select");
    this._populateFilterSelect();
  },

  



  _populateFilterSelect: function() {
    let select = this.filterSelect;
    filterList.forEach(filter => {
      let option = this.doc.createElement("option");
      option.innerHTML = option.value = filter.name;
      select.appendChild(option);
    });
  },

  


  _buildFilterItemMarkup: function() {
    let base = this.doc.createElement("div");
    base.className = "filter";

    let name = this.doc.createElement("div");
    name.className = "filter-name";

    let value = this.doc.createElement("div");
    value.className = "filter-value";

    let drag = this.doc.createElement("i");
    drag.title = L10N.getStr("dragHandleTooltipText");

    let label = this.doc.createElement("label");

    name.appendChild(drag);
    name.appendChild(label);

    let unitPreview = this.doc.createElement("span");
    let input = this.doc.createElement("input");
    input.classList.add("devtools-textinput");

    value.appendChild(input);
    value.appendChild(unitPreview);

    let removeButton = this.doc.createElement("button");
    removeButton.className = "remove-button";
    value.appendChild(removeButton);

    base.appendChild(name);
    base.appendChild(value);

    this._filterItemMarkup = base;
  },

  _addEventListeners: function() {
    this.addButton = this.el.querySelector("#add-filter");
    this.addButton.addEventListener("click", this._addButtonClick);
    this.list.addEventListener("click", this._removeButtonClick);
    this.list.addEventListener("mousedown", this._mouseDown);

    
    
    this.win.addEventListener("mousemove", this._mouseMove);
    this.win.addEventListener("mouseup", this._mouseUp);

    
    this.list.addEventListener("input", this._input);
  },

  _input: function(e) {
    let filterEl = e.target.closest(".filter"),
        index = [...this.list.children].indexOf(filterEl),
        filter = this.filters[index],
        def = this._definition(filter.name);

    if (def.type !== "string") {
      e.target.value = fixFloat(e.target.value);
    }
    this.updateValueAt(index, e.target.value);
  },

  _mouseDown: function(e) {
    let filterEl = e.target.closest(".filter");

    
    if (e.target.tagName.toLowerCase() === "i") {
      this.isReorderingFilter = true;
      filterEl.startingY = e.pageY;
      filterEl.classList.add("dragging");

      this.container.classList.add("dragging");
    
    } else if (e.target.classList.contains("devtools-draglabel")) {
      let label = e.target,
          input = filterEl.querySelector("input"),
          index = [...this.list.children].indexOf(filterEl);

      this._dragging = {
        index, label, input,
        startX: e.pageX
      };

      this.isDraggingLabel = true;
    }
  },

  _addButtonClick: function() {
    const select = this.filterSelect;
    if (!select.value) {
      return;
    }

    const key = select.value;
    const def = this._definition(key);
    
    
    const unitLabel = typeof UNIT_MAPPING[def.type] === "undefined" ?
                             UNIT_MAPPING[DEFAULT_FILTER_TYPE] :
                             UNIT_MAPPING[def.type];

    
    if (!unitLabel) {
      this.add(key);
    } else {
      this.add(key, def.range[0] + unitLabel);
    }

    this.render();
  },

  _removeButtonClick: function(e) {
    const isRemoveButton = e.target.classList.contains("remove-button");
    if (!isRemoveButton) {
      return;
    }

    let filterEl = e.target.closest(".filter");
    let index = [...this.list.children].indexOf(filterEl);
    this.removeAt(index);
  },

  _mouseMove: function(e) {
    if (this.isReorderingFilter) {
      this._dragFilterElement(e);
    } else if (this.isDraggingLabel) {
      this._dragLabel(e);
    }
  },

  _dragFilterElement: function(e) {
    const rect = this.list.getBoundingClientRect(),
          top = e.pageY - LIST_PADDING,
          bottom = e.pageY + LIST_PADDING;
    
    if (top < rect.top || bottom > rect.bottom) {
      return;
    }

    const filterEl = this.list.querySelector(".dragging");

    const delta = e.pageY - filterEl.startingY;
    filterEl.style.top = delta + "px";

    
    
    let change = delta / LIST_ITEM_HEIGHT;
    change = change > 0 ? Math.floor(change) :
             change < 0 ? Math.ceil(change) : change;

    const children = this.list.children;
    const index = [...children].indexOf(filterEl);
    const destination = index + change;

    
    if (destination >= children.length || destination < 0 || change === 0) {
      return;
    }

    
    swapArrayIndices(this.filters, index, destination);

    
    const target = change > 0 ? children[destination + 1]
                              : children[destination];
    if (target) {
      this.list.insertBefore(filterEl, target);
    } else {
      this.list.appendChild(filterEl);
    }

    filterEl.removeAttribute("style");

    const currentPosition = change * LIST_ITEM_HEIGHT;
    filterEl.startingY = e.pageY + currentPosition - delta;
  },

  _dragLabel: function(e) {
    let dragging = this._dragging;

    let input = dragging.input;

    let multiplier = DEFAULT_VALUE_MULTIPLIER;

    if (e.altKey) {
      multiplier = SLOW_VALUE_MULTIPLIER;
    } else if (e.shiftKey) {
      multiplier = FAST_VALUE_MULTIPLIER;
    }

    dragging.lastX = e.pageX;
    const delta = e.pageX - dragging.startX;
    const startValue = parseFloat(input.value);
    let value = startValue + delta * multiplier;

    const filter = this.filters[dragging.index];
    const [min, max] = this._definition(filter.name).range;
    value = value < min ? min :
            value > max ? max : value;

    input.value = fixFloat(value);

    dragging.startX = e.pageX;

    this.updateValueAt(dragging.index, value);
  },

  _mouseUp: function() {
    
    this._dragging = null;
    this.isDraggingLabel = false;

    
    if (!this.isReorderingFilter) {
      return;
    }
    let filterEl = this.list.querySelector(".dragging");

    this.isReorderingFilter = false;
    filterEl.classList.remove("dragging");
    this.container.classList.remove("dragging");
    filterEl.removeAttribute("style");

    this.emit("updated", this.getCssValue());
    this.render();
  },

  



  render: function() {
    if (!this.filters.length) {
      this.list.innerHTML = `<p> ${L10N.getStr("emptyFilterList")} <br />
                                 ${L10N.getStr("addUsingList")} </p>`;
      this.emit("render");
      return;
    }

    this.list.innerHTML = "";

    let base = this._filterItemMarkup;

    for (let filter of this.filters) {
      const def = this._definition(filter.name);

      let el = base.cloneNode(true);

      let [name, value] = el.children,
          label = name.children[1],
          [input, unitPreview] = value.children;

      let min, max;
      if (def.range) {
        [min, max] = def.range;
      }

      label.textContent = filter.name;
      input.value = filter.value;

      switch (def.type) {
        case "percentage":
        case "angle":
        case "length":
          input.type = "number";
          input.min = min;
          if (max !== Infinity) {
            input.max = max;
          }
          input.step = "0.1";
        break;
        case "string":
          input.type = "text";
          input.placeholder = def.placeholder;
        break;
      }

      
      
      if (def.type !== "string") {
        unitPreview.textContent = filter.unit;

        label.classList.add("devtools-draglabel");
        label.title = L10N.getStr("labelDragTooltipText");
      } else {
        
        unitPreview.remove();
      }

      this.list.appendChild(el);
    }

    let el = this.list.querySelector(`.filter:last-of-type input`);
    if (el) {
      el.focus();
      
      el.setSelectionRange(el.value.length, el.value.length);
    }

    this.emit("render");
  },

  







  _definition: function(name) {
    return filterList.find(a => a.name === name);
  },

  





  setCssValue: function(cssValue) {
    if (!cssValue) {
      throw new Error("Missing CSS filter value in setCssValue");
    }

    this.filters = [];

    if (cssValue === "none") {
      this.emit("updated", this.getCssValue());
      this.render();
      return;
    }

    
    
    
    let tmp = this.doc.createElement("i");
    tmp.style.filter = cssValue;
    const computedValue = this.win.getComputedStyle(tmp).filter;

    for (let {name, value} of tokenizeComputedFilter(computedValue)) {
      this.add(name, value);
    }

    this.emit("updated", this.getCssValue());
    this.render();
  },

  









  add: function(name, value = "") {
    const def = this._definition(name);
    if (!def) {
      return false;
    }

    let unit = def.type === "string"
               ? ""
               : (/[a-zA-Z%]+/.exec(value) || [])[0];

    if (def.type !== "string") {
      value = parseFloat(value);

      
      if (def.type === "percentage" && !unit) {
        value = value * 100;
        unit = "%";
      }

      const [min, max] = def.range;
      if (value < min) {
        value = min;
      } else if (value > max) {
        value = max;
      }
    }

    const index = this.filters.push({value, unit, name: def.name}) - 1;
    this.emit("updated", this.getCssValue());

    return index;
  },

  







  getValueAt: function(index) {
    let filter = this.filters[index];
    if (!filter) {
      return null;
    }

    const {value, unit} = filter;

    return value + unit;
  },

  removeAt: function(index) {
    if (!this.filters[index]) {
      return null;
    }

    this.filters.splice(index, 1);
    this.emit("updated", this.getCssValue());
    this.render();
  },

  





  getCssValue: function() {
    return this.filters.map((filter, i) => {
      return `${filter.name}(${this.getValueAt(i)})`;
    }).join(" ") || "none";
  },

  








  updateValueAt: function(index, value) {
    let filter = this.filters[index];
    if (!filter) {
      return;
    }

    const def = this._definition(filter.name);

    if (def.type !== "string") {
      const [min, max] = def.range;
      if (value < min) {
        value = min;
      } else if (value > max) {
        value = max;
      }
    }

    filter.value = filter.unit ? fixFloat(value, true) : value;

    this.emit("updated", this.getCssValue());
  },

  _removeEventListeners: function() {
    this.addButton.removeEventListener("click", this._addButtonClick);
    this.list.removeEventListener("click", this._removeButtonClick);
    this.list.removeEventListener("mousedown", this._mouseDown);

    
    this.win.removeEventListener("mousemove", this._mouseMove);
    this.win.removeEventListener("mouseup", this._mouseUp);

    
    this.list.removeEventListener("input", this._input);
  },

  _destroyMarkup: function() {
    this._filterItemMarkup.remove();
    this.el.remove();
    this.el = this.list = this.container = this._filterItemMarkup = null;
  },

  destroy: function() {
    this._removeEventListeners();
    this._destroyMarkup();
  }
};


function fixFloat(a, number) {
  let fixed = parseFloat(a).toFixed(1);
  return number ? parseFloat(fixed) : fixed;
}












function swapArrayIndices(array, a, b) {
  array[a] = array.splice(b, 1, array[a])[0];
}














function tokenizeComputedFilter(css) {
  let filters = [];
  let current = "";
  let depth = 0;

  if (css === "none") {
    return filters;
  }

  while (css.length) {
    const char = css[0];

    switch (char) {
      case "(":
        depth++;
        if (depth === 1) {
          filters.push({name: current.trim()});
          current = "";
        } else {
          current += char;
        }
      break;
      case ")":
        depth--;
        if (depth === 0) {
          filters[filters.length - 1].value = current.trim();
          current = "";
        } else {
          current += char;
        }
      break;
      default:
        current += char;
      break;
    }

    css = css.slice(1);
  }

  return filters;
}
