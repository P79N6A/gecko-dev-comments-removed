


































var gDefaultFlexContainerSize = "200px";



var gRowPropertyMapping =
{
  "_main-size":               "width",
  "_min-main-size":           "min-width",
  "_max-main-size":           "max-width",
  "_border-main-start-width": "border-left-width",
  "_border-main-end-width":   "border-right-width",
  "_padding-main-start":      "padding-left",
  "_padding-main-end":        "padding-right",
  "_margin-main-start":       "margin-left",
  "_margin-main-end":         "margin-right"
};



var gRowReversePropertyMapping =
{
  "_main-size":               "width",
  "_min-main-size":           "min-width",
  "_max-main-size":           "max-width",
  "_border-main-start-width": "border-right-width",
  "_border-main-end-width":   "border-left-width",
  "_padding-main-start":      "padding-right",
  "_padding-main-end":        "padding-left",
  "_margin-main-start":       "margin-right",
  "_margin-main-end":         "margin-left"
};



var gColumnPropertyMapping =
{
  "_main-size":               "height",
  "_min-main-size":           "min-height",
  "_max-main-size":           "max-height",
  "_border-main-start-width": "border-top-width",
  "_border-main-end-width":   "border-bottom-width",
  "_padding-main-start":      "padding-top",
  "_padding-main-end":        "padding-bottom",
  "_margin-main-start":       "margin-top",
  "_margin-main-end":         "margin-bottom"
};



var gColumnReversePropertyMapping =
{
  "_main-size":               "height",
  "_min-main-size":           "min-height",
  "_max-main-size":           "max-height",
  "_border-main-start-width": "border-bottom-width",
  "_border-main-end-width":   "border-top-width",
  "_padding-main-start":      "padding-bottom",
  "_padding-main-end":        "padding-top",
  "_margin-main-start":       "margin-bottom",
  "_margin-main-end":         "margin-top"
};


var gFlexboxTestcases =
[
 
 {
   items:
     [
       { "_main-size":       [ "40px", "40px" ] },
       { "_main-size":       [ "65px", "65px" ] },
     ]
 },
 
 {
   items:
     [
       { "-moz-flex-basis": "50px",
         "_main-size":       [ null,  "50px" ]
       },
       {
         "-moz-flex-basis": "20px",
         "_main-size":       [ null, "20px" ]
       },
     ]
 },
 
 
 {
   items:
     [
       {
         "-moz-flex": "0 0 150px",
         "_main-size":       [ null, "150px" ]
       },
       {
         "-moz-flex": "0 0 90px",
         "_main-size":       [ null, "90px" ]
       },
     ]
 },
 
 
 {
   items:
     [
       {
         "-moz-flex": "0 0 250px",
         "_main-size":       [ null, "250px" ]
       },
       {
         "-moz-flex": "0 0 400px",
         "_main-size":       [ null, "400px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex-basis": "30%",
         "_main-size":       [ null, "60px" ]
       },
       {
         "-moz-flex-basis": "45%",
         "_main-size":       [ null, "90px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex-basis": "-moz-calc(20%)",
         "_main-size":       [ null, "40px" ]
       },
       {
         "-moz-flex-basis": "-moz-calc(80%)",
         "_main-size":       [ null, "160px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex-basis": "-moz-calc(10px + 20%)",
         "_main-size":       [ null, "50px" ]
       },
       {
         "-moz-flex-basis": "-moz-calc(60% - 1px)",
         "_main-size":       [ null, "119px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex": "1",
         "_main-size":       [ null,  "60px" ]
       },
       {
         "-moz-flex": "2",
         "_main-size":       [ null, "120px" ]
       },
       {
         "-moz-flex": "0 20px",
         "_main-size":       [ null, "20px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex": "100000",
         "_main-size": [ null,  "60px" ]
       },
       {
         "-moz-flex": "200000",
         "_main-size": [ null, "120px" ]
       },
       {
         "-moz-flex": "0.000001 20px",
         "_main-size": [ null,  "20px" ]
       }
     ]
 },
 
 
 {
   items:
     [
       {
         "-moz-flex": "none",
         "_main-size": [ "20px", "20px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,   "60px" ]
       },
       {
         "-moz-flex": "2",
         "_main-size": [ null,  "120px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "-moz-flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "200px" ]
       },
     ]
 },
 {
   items:
     [
       {
         "-moz-flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "-moz-flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "-moz-flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "-moz-flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
     ]
 },
 {
   items:
     [
       {
         "-moz-flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "-moz-flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "-moz-flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "-moz-flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
     ]
 },

 
 

 
 
 
 {
   container_properties:
   {
     "_main-size": "9000000px"
   },
   items:
     [
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "9000000px" ]
       },
     ]
 },
 
 {
   container_properties:
   {
     "_main-size": "9000000px"
   },
   items:
     [
       {
         "-moz-flex": "2",
         "_main-size": [ null,  "6000000px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "3000000px" ]
       },
     ]
 },

 
 
 
 
 
 
 
 
 
 
 
 
 
 {
   container_properties:
   {
     "_main-size": "9000000px"
   },
   items:
     [
       {
         "-moz-flex": "3000000",
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "1px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "1px" ]
       },
       {
         "-moz-flex": "2999999",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "2999998",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "1px" ]
       },
     ]
 },
 
 
 {
   container_properties:
   {
     "_main-size": "9000000px"
   },
   items:
     [
       {
         "-moz-flex": "3000000",
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "2999999",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "2999998",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "1px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "1px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "1px" ]
       },
     ]
 },
 
 
 {
   container_properties:
   {
     "_main-size": "9000000px"
   },
   items:
     [
       {
         "-moz-flex": "1",
         
         
         "_main-size": [ null,  "0.966667px" ]
       },
       {
         "-moz-flex": "1",
         
         
         "_main-size": [ null,  "0.983333px" ]
       },
       {
         "-moz-flex": "1",
         
         
         "_main-size": [ null,  "0.983333px" ]
       },
       {
         "-moz-flex": "3000000",
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "2999999",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "-moz-flex": "2999998",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
     ]
 },

 
 {
   items:
     [
       {
         "-moz-flex": "auto",
         "_main-size": [ null, "45px" ]
       },
       {
         "-moz-flex": "2",
         "_main-size": [ null, "90px" ]
       },
       {
         "-moz-flex": "20px 1 0",
         "_main-size": [ null, "65px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex": "20px",
         "_main-size": [ null, "65px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null, "45px" ]
       },
       {
         "-moz-flex": "2",
         "_main-size": [ null, "90px" ]
       }
     ]
 },
 {
   items:
     [
       {
         "-moz-flex": "2",
         "_main-size": [ null,  "100px" ],
         "border": "0px dashed",
         "_border-main-start-width":  [ "5px",  "5px" ],
         "_border-main-end-width": [ "15px", "15px" ],
         "_margin-main-start": [ "22px", "22px" ],
         "_margin-main-end": [ "8px", "8px" ]
       },
       {
         "-moz-flex": "1",
         "_main-size": [ null,  "50px" ],
         "_margin-main-start":   [ "auto", "0px" ],
         "_padding-main-end": [ "auto", "0px" ],
       }
     ]
 },
 

 
 
 {
   items:
     [
       { "_main-size": [ "400px",  "200px" ] },
     ],
 },
 
 {
   items:
     [
       {
         "-moz-flex": "4 2 250px",
         "_main-size": [ null,  "200px" ]
       },
     ],
 },
 
 
 {
   items:
     [
       { "_main-size": [ "80px",   "40px" ] },
       { "_main-size": [ "40px",   "20px" ] },
       { "_main-size": [ "30px",   "15px" ] },
       { "_main-size": [ "250px", "125px" ] },
     ]
 },
 
 
 
 {
   items:
     [
       {
         "-moz-flex": "4 3 100px",
         "_main-size": [ null,  "80px" ]
       },
       {
         "-moz-flex": "5 3 50px",
         "_main-size": [ null,  "40px" ]
       },
       {
         "-moz-flex": "0 3 100px",
         "_main-size": [ null, "80px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex": "4 2 50px",
         "_main-size": [ null,  "30px" ]
       },
       {
         "-moz-flex": "5 3 50px",
         "_main-size": [ null,  "20px" ]
       },
       {
         "-moz-flex": "0 0 150px",
         "_main-size": [ null, "150px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex": "4 20000000 50px",
         "_main-size": [ null,  "30px" ]
       },
       {
         "-moz-flex": "5 30000000 50px",
         "_main-size": [ null,  "20px" ]
       },
       {
         "-moz-flex": "0 0.0000001 150px",
         "_main-size": [ null, "150px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "-moz-flex": "4 2 115px",
         "_main-size": [ null,  "69px" ]
       },
       {
         "-moz-flex": "5 1 150px",
         "_main-size": [ null,  "120px" ]
       },
       {
         "-moz-flex": "1 4 30px",
         "_main-size": [ null,  "6px" ]
       },
       {
         "-moz-flex": "1 0 5px",
         "_main-size": [ null, "5px" ]
       },
     ]
 },

 
 {
   items:
     [
       {
         "-moz-flex": "4 5 75px",
         "_min-main-size": "50px",
         "_main-size": [ null,  "50px" ],
       },
       {
         "-moz-flex": "5 5 100px",
         "_main-size": [ null,  "62.5px" ]
       },
       {
         "-moz-flex": "0 4 125px",
         "_main-size": [ null, "87.5px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "-moz-flex": "auto",
         "_min-main-size": "110px",
         "_main-size": [ "50px",  "125px" ]
       },
       {
         "-moz-flex": "auto",
         "_main-size": [ null, "75px" ]
       }
     ]
 },

 
 
 
 {
   items:
     [
       {
         "-moz-flex": "auto",
         "_min-main-size": "150px",
         "_main-size": [ "50px",  "150px" ]
       },
       {
         "-moz-flex": "auto",
         "_main-size": [ null, "50px" ]
       }
     ]
 },

 
 {
   items:
     [
       {
         "-moz-flex": "auto",
         "_min-main-size": "20px",
         "_main-size": [ null,  "20px" ]
       },
       {
         "-moz-flex": "9 auto",
         "_min-main-size": "150px",
         "_main-size": [ "50px",  "180px" ]
       },
     ]
 },
 {
   items:
     [
       {
         "-moz-flex": "1 1 0px",
         "_min-main-size": "90px",
         "_main-size": [ null, "90px" ]
       },
       {
         "-moz-flex": "1 1 0px",
         "_min-main-size": "80px",
         "_main-size": [ null, "80px" ]
       },
       {
         "-moz-flex": "1 1 40px",
         "_main-size": [ null, "30px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "-moz-flex": "1 2 100px",
         "_min-main-size": "90px",
         "_main-size": [ null, "90px" ]
       },
       {
         "-moz-flex": "1 1 100px",
         "_min-main-size": "70px",
         "_main-size": [ null, "70px" ]
       },
       {
         "-moz-flex": "1 1 100px",
         "_main-size": [ null, "40px" ]
       }
     ]
 },

 
 

 
 
 
 
 
 
 
 
 
 
{
   items:
     [
       {
         "-moz-flex": "auto",
         "_min-main-size": "130px",
         "_main-size": [ null, "150px" ]
       },
       {
         "-moz-flex": "auto",
         "_max-main-size": "50px",
         "_main-size": [ null,  "50px" ]
       },
     ]
 },

 
 
 
 
 
 
 
 
 
 {
   items:
     [
       {
         "-moz-flex": "auto",
         "_min-main-size": "130px",
         "_main-size": [ null, "130px" ]
       },
       {
         "-moz-flex": "auto",
         "_max-main-size": "80px",
         "_main-size": [ null,  "70px" ]
       },
     ]
 },

 
 
 
 
 
 
 
 
 {
   items:
     [
       {
         "-moz-flex": "auto",
         "_max-main-size": "80px",
         "_main-size": [ null,  "80px" ]
       },
       {
         "-moz-flex": "auto",
         "_min-main-size": "120px",
         "_main-size": [ null, "120px" ]
       },
     ]
 },
];
