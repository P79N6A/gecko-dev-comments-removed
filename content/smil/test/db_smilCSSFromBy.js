










































var _fromByTestLists =
{
  color: [
    new AnimTestcaseFromBy("rgb(10, 20, 30)", "currentColor",
                           { midComp: "rgb(35, 45, 55)",
                             toComp:  "rgb(60, 70, 80)"}),
    new AnimTestcaseFromBy("currentColor", "rgb(30, 20, 10)",
                           { fromComp: "rgb(50, 50, 50)",
                             midComp:  "rgb(65, 60, 55)",
                             toComp:   "rgb(80, 70, 60)"}),
  ],
  lengthPx: [
    new AnimTestcaseFromBy("1px", "10px", { midComp: "6px", toComp: "11px"}),
  ],
  opacity: [
    new AnimTestcaseFromBy("1", "-1", { midComp: "0.5", toComp: "0"}),
    new AnimTestcaseFromBy("0.4", "-0.6", { midComp: "0.1", toComp: "0"}),
    new AnimTestcaseFromBy("0.8", "-1.4", { midComp: "0.1", toComp: "0"},
                           "opacities with abs val >1 get clamped too early"),
    new AnimTestcaseFromBy("1.2", "-0.6", { midComp: "0.9", toComp: "0.6"},
                           "opacities with abs val >1 get clamped too early"),
  ],
  paint: [
    
    
    
    
    
    new AnimTestcaseFromBy("none", "none",  { noEffect: 1 }),
    new AnimTestcaseFromBy("url(#gradA)", "url(#gradB)", { noEffect: 1 }),
    new AnimTestcaseFromBy("url(#gradA)", "url(#gradB) red", { noEffect: 1 }),
    new AnimTestcaseFromBy("url(#gradA)", "none", { noEffect: 1 }),
    new AnimTestcaseFromBy("red", "url(#gradA)", { noEffect: 1 }),
  ]
};


var gFromByBundles =
[
  new TestcaseBundle(gPropList.fill, [].concat(_fromByTestLists.color,
                                               _fromByTestLists.paint)),
  new TestcaseBundle(gPropList.font_size,      _fromByTestLists.lengthPx),
  new TestcaseBundle(gPropList.font_size_adjust, [
    
    
    new AnimTestcaseFromBy("0.5", "0.1"),
    new AnimTestcaseFromBy("none", "0.1"),
    new AnimTestcaseFromBy("0.1", "none")
  ]),
  new TestcaseBundle(gPropList.lighting_color, _fromByTestLists.color),
  new TestcaseBundle(gPropList.opacity,        _fromByTestLists.opacity),
  new TestcaseBundle(gPropList.stroke_miterlimit, [
    new AnimTestcaseFromBy("1", "1", { midComp: "1.5", toComp: "2" }),
    new AnimTestcaseFromBy("20.1", "-10", { midComp: "15.1", toComp: "10.1" }),
  ]),
  new TestcaseBundle(gPropList.stroke_dasharray, [
    
    
    new AnimTestcaseFromBy("none", "5"),
    new AnimTestcaseFromBy("10", "5"),
    new AnimTestcaseFromBy("1", "2, 3"),
  ]),
  new TestcaseBundle(gPropList.stroke_width,   _fromByTestLists.lengthPx),
];
