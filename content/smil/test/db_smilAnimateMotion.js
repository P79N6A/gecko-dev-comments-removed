









































var gMotionAttr = new AdditiveAttribute(SMILUtil.getMotionFakeAttributeName(),
                                        "XML", "rect");


var _reusedCTMLists = {
  pacedBasic:     { ctm0:   [100, 200, 0],
                    ctm1_6: [105, 205, 0],
                    ctm1_3: [110, 210, 0],
                    ctm2_3: [120, 220, 0],
                    ctm1:   [130, 210, 0]
  },
  pacedR60:       { ctm0:   [100, 200, Math.PI/3],
                    ctm1_6: [105, 205, Math.PI/3],
                    ctm1_3: [110, 210, Math.PI/3],
                    ctm2_3: [120, 220, Math.PI/3],
                    ctm1:   [130, 210, Math.PI/3]
  },
  pacedRAuto:     { ctm0:   [100, 200, Math.PI/4],
                    ctm1_6: [105, 205, Math.PI/4],
                    ctm1_3: [110, 210, Math.PI/4],
                    ctm2_3: [120, 220, Math.PI/4],
                    ctm1:   [130, 210, -Math.PI/4]
  },
  pacedRAutoReverse : { ctm0:   [100, 200, 5*Math.PI/4],
                        ctm1_6: [105, 205, 5*Math.PI/4],
                        ctm1_3: [110, 210, 5*Math.PI/4],
                        ctm2_3: [120, 220, 5*Math.PI/4],
                        ctm1:   [130, 210, 3*Math.PI/4]
  },
  
  discreteBasic : { ctm0:   [100, 200, 0],
                    ctm1_6: [100, 200, 0],
                    ctm1_3: [120, 220, 0],
                    ctm2_3: [130, 210, 0],
                    ctm1:   [130, 210, 0]
  },
  discreteRAuto : { ctm0:   [100, 200, Math.PI/4],
                    ctm1_6: [100, 200, Math.PI/4],
                    ctm1_3: [120, 220, Math.PI/4],
                    ctm2_3: [130, 210, -Math.PI/4],
                    ctm1:   [130, 210, -Math.PI/4]
  },
  justMoveBasic : { ctm0:   [40, 80, 0],
                    ctm1_6: [40, 80, 0],
                    ctm1_3: [40, 80, 0],
                    ctm2_3: [40, 80, 0],
                    ctm1:   [40, 80, 0]
  },
  justMoveR60 :   { ctm0:   [40, 80, Math.PI/3],
                    ctm1_6: [40, 80, Math.PI/3],
                    ctm1_3: [40, 80, Math.PI/3],
                    ctm2_3: [40, 80, Math.PI/3],
                    ctm1:   [40, 80, Math.PI/3]
  },
  justMoveRAuto : { ctm0:   [40, 80, Math.atan(2)],
                    ctm1_6: [40, 80, Math.atan(2)],
                    ctm1_3: [40, 80, Math.atan(2)],
                    ctm2_3: [40, 80, Math.atan(2)],
                    ctm1:   [40, 80, Math.atan(2)]
  },
  justMoveRAutoReverse : { ctm0:   [40, 80, Math.PI + Math.atan(2)],
                           ctm1_6: [40, 80, Math.PI + Math.atan(2)],
                           ctm1_3: [40, 80, Math.PI + Math.atan(2)],
                           ctm2_3: [40, 80, Math.PI + Math.atan(2)],
                           ctm1:   [40, 80, Math.PI + Math.atan(2)]
  },
  nullMoveBasic : { ctm0:   [0, 0, 0],
                    ctm1_6: [0, 0, 0],
                    ctm1_3: [0, 0, 0],
                    ctm2_3: [0, 0, 0],
                    ctm1:   [0, 0, 0]
  },
  nullMoveRAutoReverse : { ctm0:   [0, 0, Math.PI],
                           ctm1_6: [0, 0, Math.PI],
                           ctm1_3: [0, 0, Math.PI],
                           ctm2_3: [0, 0, Math.PI],
                           ctm1:   [0, 0, Math.PI]
  },
};

var gMotionBundles =
[
  
  new TestcaseBundle(gMotionAttr, [
    
    new AnimMotionTestcase({ "values": "100, 200; 120, 220; 130, 210" },
                           _reusedCTMLists.pacedBasic),
    new AnimMotionTestcase({ "path":  "M100 200 L120 220 L130 210" },
                           _reusedCTMLists.pacedBasic),
    new AnimMotionTestcase({ "mpath": "M100 200 L120 220 L130 210" },
                           _reusedCTMLists.pacedBasic),

    
    new AnimMotionTestcase({ "values": "100,200; 120,220; 130, 210",
                             "rotate": "60" },
                           _reusedCTMLists.pacedR60),
    new AnimMotionTestcase({ "path": "M100 200 L120 220 L130 210",
                             "rotate": "60" },
                           _reusedCTMLists.pacedR60),
    new AnimMotionTestcase({ "mpath": "M100 200 L120 220 L130 210",
                             "rotate": "60" },
                           _reusedCTMLists.pacedR60),

    
    new AnimMotionTestcase({ "path": "M100 200 L120 220 L130 210",
                             "rotate": "1.0471975512rad" }, 
                           _reusedCTMLists.pacedR60),

    
    new AnimMotionTestcase({ "values": "100,200; 120,220; 130, 210",
                             "rotate": "auto" },
                           _reusedCTMLists.pacedRAuto),
    new AnimMotionTestcase({ "path": "M100 200 L120 220 L130 210",
                             "rotate": "auto" },
                           _reusedCTMLists.pacedRAuto),
    new AnimMotionTestcase({ "mpath": "M100 200 L120 220 L130 210",
                             "rotate": "auto" },
                           _reusedCTMLists.pacedRAuto),

    
    new AnimMotionTestcase({ "values": "100,200; 120,220; 130, 210",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.pacedRAutoReverse),
    new AnimMotionTestcase({ "path": "M100 200 L120 220 L130 210",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.pacedRAutoReverse),
    new AnimMotionTestcase({ "mpath": "M100 200 L120 220 L130 210",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.pacedRAutoReverse),

  ]),

  
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "100, 200; 120, 220; 130, 210",
                             "calcMode": "discrete" },
                           _reusedCTMLists.discreteBasic),
    new AnimMotionTestcase({ "path": "M100 200 L120 220 L130 210",
                             "calcMode": "discrete" },
                           _reusedCTMLists.discreteBasic),
    new AnimMotionTestcase({ "mpath": "M100 200 L120 220 L130 210",
                             "calcMode": "discrete" },
                           _reusedCTMLists.discreteBasic),
    
    new AnimMotionTestcase({ "values": "100, 200; 120, 220; 130, 210",
                             "calcMode": "discrete",
                             "rotate": "auto" },
                           _reusedCTMLists.discreteRAuto),
    new AnimMotionTestcase({ "path": "M100 200 L120 220 L130 210",
                             "calcMode": "discrete",
                             "rotate": "auto" },
                           _reusedCTMLists.discreteRAuto),
    new AnimMotionTestcase({ "mpath": "M100 200 L120 220 L130 210",
                             "calcMode": "discrete",
                             "rotate": "auto" },
                           _reusedCTMLists.discreteRAuto),
  ]),

  
  new TestcaseBundle(gMotionAttr, [
    
    new AnimMotionTestcase({ "from": "10, 10",
                             "by":   "30, 60" },
                           { ctm0:   [10, 10, 0],
                             ctm1_6: [15, 20, 0],
                             ctm1_3: [20, 30, 0],
                             ctm2_3: [30, 50, 0],
                             ctm1:   [40, 70, 0]
                           }),
    
    new AnimMotionTestcase({ "from": "1em, 10",
                             "by":   "30, 6em" },
                           { ctm0:   [10, 10, 0],
                             ctm1_6: [15, 20, 0],
                             ctm1_3: [20, 30, 0],
                             ctm2_3: [30, 50, 0],
                             ctm1:   [40, 70, 0]
                           }),
  ]),

  
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "40, 80" },
                           _reusedCTMLists.justMoveBasic),
    new AnimMotionTestcase({ "path":  "M40 80" },
                           _reusedCTMLists.justMoveBasic),
    new AnimMotionTestcase({ "mpath": "m40 80" },
                           _reusedCTMLists.justMoveBasic),
  ]),
  
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "40, 80",
                             "rotate": "60" },
                           _reusedCTMLists.justMoveR60),
    new AnimMotionTestcase({ "path":  "M40 80",
                             "rotate": "60" },
                           _reusedCTMLists.justMoveR60),
    new AnimMotionTestcase({ "mpath": "m40 80",
                             "rotate": "60" },
                           _reusedCTMLists.justMoveR60),
  ]),
  
  
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "40, 80",
                             "rotate": "auto" },
                           _reusedCTMLists.justMoveRAuto),
    new AnimMotionTestcase({ "path":  "M40 80",
                             "rotate": "auto" },
                           _reusedCTMLists.justMoveRAuto),
    new AnimMotionTestcase({ "mpath": "m40 80",
                             "rotate": "auto" },
                           _reusedCTMLists.justMoveRAuto),
  ]),
  
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "40, 80",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.justMoveRAutoReverse),
    new AnimMotionTestcase({ "path":  "M40 80",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.justMoveRAutoReverse),
    new AnimMotionTestcase({ "mpath": "m40 80",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.justMoveRAutoReverse),
  ]),
  
  
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "0, 0",
                             "rotate": "auto" },
                           _reusedCTMLists.nullMoveBasic),
  ]),
  new TestcaseBundle(gMotionAttr, [
    new AnimMotionTestcase({ "values": "0, 0",
                             "rotate": "auto-reverse" },
                           _reusedCTMLists.nullMoveRAutoReverse),
  ]),
];





