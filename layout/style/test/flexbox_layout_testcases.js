







































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
       { "_main-size": [ "40px", "40px" ] },
       { "_main-size": [ "65px", "65px" ] },
     ]
 },
 
 {
   items:
     [
       { "flex-basis": "50px",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex-basis": "20px",
         "_main-size": [ null, "20px" ]
       },
     ]
 },
 
 
 {
   items:
     [
       {
         "flex": "0 0 150px",
         "_main-size": [ null, "150px" ]
       },
       {
         "flex": "0 0 90px",
         "_main-size": [ null, "90px" ]
       },
     ]
 },
 
 
 {
   items:
     [
       {
         "flex": "0 0 250px",
         "_main-size": [ null, "250px" ]
       },
       {
         "flex": "0 0 400px",
         "_main-size": [ null, "400px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "flex-basis": "30%",
         "_main-size": [ null, "60px" ]
       },
       {
         "flex-basis": "45%",
         "_main-size": [ null, "90px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "flex-basis": "calc(20%)",
         "_main-size": [ null, "40px" ]
       },
       {
         "flex-basis": "calc(80%)",
         "_main-size": [ null, "160px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "flex-basis": "calc(10px + 20%)",
         "_main-size": [ null, "50px" ]
       },
       {
         "flex-basis": "calc(60% - 1px)",
         "_main-size": [ null, "119px" ]
       },
     ]
 },
 
 {
   items:
     [
       {
         "flex": "1",
         "_main-size": [ null,  "60px" ]
       },
       {
         "flex": "2",
         "_main-size": [ null, "120px" ]
       },
       {
         "flex": "0 20px",
         "_main-size": [ null, "20px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "flex": "100000",
         "_main-size": [ null,  "60px" ]
       },
       {
         "flex": "200000",
         "_main-size": [ null, "120px" ]
       },
       {
         "flex": "0.000001 20px",
         "_main-size": [ null,  "20px" ]
       }
     ]
 },
 
 
 {
   items:
     [
       {
         "flex": "none",
         "_main-size": [ "20px", "20px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null,   "60px" ]
       },
       {
         "flex": "2",
         "_main-size": [ null,  "120px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "200px" ]
       },
     ]
 },
 {
   items:
     [
       {
         "flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex": "9999999999999999999999999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
     ]
 },
 {
   items:
     [
       {
         "flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex": "99999999999999999999999999999999999",
         "_main-size": [ null,  "50px" ]
       },
       {
         "flex": "99999999999999999999999999999999999",
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
         "flex": "1",
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
         "flex": "2",
         "_main-size": [ null,  "6000000px" ]
       },
       {
         "flex": "1",
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
         "flex": "3000000",
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "1px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "1px" ]
       },
       {
         "flex": "2999999",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "2999998",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "1px", 0.2 ]
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
         "flex": "3000000",
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "2999999",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "2999998",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "1px", 0.2 ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "1px", 0.2 ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "1px", 0.2 ]
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
         "flex": "1",
         
         
         "_main-size": [ null,  "1px", 0.2 ]
       },
       {
         "flex": "1",
         
         
         "_main-size": [ null,  "1px", 0.2 ]
       },
       {
         "flex": "1",
         
         
         "_main-size": [ null,  "1px", 0.2 ]
       },
       {
         "flex": "3000000",
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "2999999",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
       {
         "flex": "2999998",
         
         
         "_main-size": [ null,  "3000000px" ]
       },
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "auto",
         "_main-size": [ null, "45px" ]
       },
       {
         "flex": "2",
         "_main-size": [ null, "90px" ]
       },
       {
         "flex": "20px 1 0",
         "_main-size": [ null, "65px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "flex": "20px",
         "_main-size": [ null, "65px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null, "45px" ]
       },
       {
         "flex": "2",
         "_main-size": [ null, "90px" ]
       }
     ]
 },
 {
   items:
     [
       {
         "flex": "2",
         "_main-size": [ null,  "100px" ],
         "border": "0px dashed",
         "_border-main-start-width": [ "5px",  "5px" ],
         "_border-main-end-width": [ "15px", "15px" ],
         "_margin-main-start": [ "22px", "22px" ],
         "_margin-main-end": [ "8px", "8px" ]
       },
       {
         "flex": "1",
         "_main-size": [ null,  "50px" ],
         "_margin-main-start": [ "auto", "0px" ],
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
         "flex": "4 2 250px",
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
         "flex": "4 3 100px",
         "_main-size": [ null,  "80px" ]
       },
       {
         "flex": "5 3 50px",
         "_main-size": [ null,  "40px" ]
       },
       {
         "flex": "0 3 100px",
         "_main-size": [ null, "80px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "flex": "4 2 50px",
         "_main-size": [ null,  "30px" ]
       },
       {
         "flex": "5 3 50px",
         "_main-size": [ null,  "20px" ]
       },
       {
         "flex": "0 0 150px",
         "_main-size": [ null, "150px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "flex": "4 20000000 50px",
         "_main-size": [ null,  "30px" ]
       },
       {
         "flex": "5 30000000 50px",
         "_main-size": [ null,  "20px" ]
       },
       {
         "flex": "0 0.0000001 150px",
         "_main-size": [ null, "150px" ]
       }
     ]
 },
 
 {
   items:
     [
       {
         "flex": "4 2 115px",
         "_main-size": [ null,  "69px" ]
       },
       {
         "flex": "5 1 150px",
         "_main-size": [ null,  "120px" ]
       },
       {
         "flex": "1 4 30px",
         "_main-size": [ null,  "6px" ]
       },
       {
         "flex": "1 0 5px",
         "_main-size": [ null, "5px" ]
       },
     ]
 },

 
 {
   items:
     [
       {
         "flex": "4 5 75px",
         "_min-main-size": "50px",
         "_main-size": [ null,  "50px" ],
       },
       {
         "flex": "5 5 100px",
         "_main-size": [ null,  "62.5px" ]
       },
       {
         "flex": "0 4 125px",
         "_main-size": [ null, "87.5px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "auto",
         "_min-main-size": "110px",
         "_main-size": [ "50px",  "125px" ]
       },
       {
         "flex": "auto",
         "_main-size": [ null, "75px" ]
       }
     ]
 },

 
 
 
 {
   items:
     [
       {
         "flex": "auto",
         "_min-main-size": "150px",
         "_main-size": [ "50px",  "150px" ]
       },
       {
         "flex": "auto",
         "_main-size": [ null, "50px" ]
       }
     ]
 },

 
 {
   items:
     [
       {
         "flex": "auto",
         "_min-main-size": "20px",
         "_main-size": [ null,  "20px" ]
       },
       {
         "flex": "9 main-size",
         "_min-main-size": "150px",
         "_main-size": [ "50px",  "180px" ]
       },
     ]
 },
 {
   items:
     [
       {
         "flex": "1 1 0px",
         "_min-main-size": "90px",
         "_main-size": [ null, "90px" ]
       },
       {
         "flex": "1 1 0px",
         "_min-main-size": "80px",
         "_main-size": [ null, "80px" ]
       },
       {
         "flex": "1 1 40px",
         "_main-size": [ null, "30px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "1 2 100px",
         "_min-main-size": "90px",
         "_main-size": [ null, "90px" ]
       },
       {
         "flex": "1 1 100px",
         "_min-main-size": "70px",
         "_main-size": [ null, "70px" ]
       },
       {
         "flex": "1 1 100px",
         "_main-size": [ null, "40px" ]
       }
     ]
 },

 
 

 
 
 
 
 
 
 
 
 
 
 {
   items:
     [
       {
         "flex": "auto",
         "_min-main-size": "130px",
         "_main-size": [ null, "150px" ]
       },
       {
         "flex": "auto",
         "_max-main-size": "50px",
         "_main-size": [ null,  "50px" ]
       },
     ]
 },

 
 
 
 
 
 
 
 
 
 {
   items:
     [
       {
         "flex": "auto",
         "_min-main-size": "130px",
         "_main-size": [ null, "130px" ]
       },
       {
         "flex": "auto",
         "_max-main-size": "80px",
         "_main-size": [ null,  "70px" ]
       },
     ]
 },

 
 
 
 
 
 
 
 
 {
   items:
     [
       {
         "flex": "auto",
         "_max-main-size": "80px",
         "_main-size": [ null,  "80px" ]
       },
       {
         "flex": "auto",
         "_min-main-size": "120px",
         "_main-size": [ null, "120px" ]
       },
     ]
 },

 
 
 
 
 

 
 {
   items:
     [
       {
         "flex": "0.1 100px",
         "_main-size": [ null, "110px" ] 
       },
     ]
 },
 {
   items:
     [
       {
         "flex": "0.8 0px",
         "_main-size": [ null, "160px" ] 
       },
     ]
 },

 
 {
   items:
     [
       {
         "flex": "0.4 70px",
         "_main-size": [ null, "110px" ] 
       },
       {
         "flex": "0.2 30px",
         "_main-size": [ null,  "50px" ] 
       },
     ]
 },

 
 {
   items:
     [
       {
         "flex": "0.4 70px",
         "_main-size": [ null, "110px" ] 
       },
       {
         "flex": "0.2 30px",
         "_max-main-size": "35px",
         "_main-size": [ null,  "35px" ] 
       },
     ]
 },
 
 
 
 {
   items:
     [
       {
         "flex": "0.4 70px",
         "_main-size": [ null, "118px" ] 
       },
       {
         "flex": "0.2 30px",
         "_max-main-size": "10px",
         "_main-size": [ null,  "10px" ] 
       },
     ]
 },
 
 
 
 
 {
   items:
     [
       {
         "flex": "0.4 70px",
         "_main-size": [ null, "118px" ] 
                                         
       },
       {
         "flex": "0.2 150px",
         "_max-main-size": "10px",
         "_main-size": [ null,  "10px" ] 
       },
     ]
 },

 
 {
   items:
     [
       {
         "flex": "0.4 70px",
         "_main-size": [ null, "110px" ] 
       },
       {
         "flex": "0.2 30px",
         "_min-main-size": "70px",
         "_main-size": [ null,  "70px" ] 
       },
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "0.4 70px",
         "_main-size": [ null, "80px" ] 
                                        
                                        
       },
       {
         "flex": "0.2 30px",
         "_min-main-size": "120px",
         "_main-size": [ null,  "120px" ] 
       },
     ]
 },
 
 
 {
   items:
     [
       {
         "flex": "0.3 30px",
         "_main-size": [ null,  "20px" ] 
       },
       {
         "flex": "0.2 20px",
         "_min-main-size": "180px",
         "_main-size": [ null,  "180px" ] 
       },
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "0.3 30px",
         "_main-size": [ null,  "75px" ]
         
         
         
       },
       {
         "flex": "0.2 20px",
         "_max-main-size": "30px",
         "_main-size": [ null,  "30px" ]
         
         
       },
       {
         "flex": "4.5 0px",
         "_max-main-size": "20px",
         "_main-size": [ null,  "20px" ]
         
       },
     ]
 },

 
 
 
 {
   
   
   
   
   
   items:
     [
       {
         "flex": "0.5 100px",
         "_main-size": [ null,  "130px" ]
       },
       {
         "flex": "1 98px",
         "_max-main-size": "40px",
         "_main-size": [ null,  "40px" ]
       },
     ]
 },
 {
   
   
   
   items:
     [
       {
         "flex": "0.5 100px",
         "_main-size": [ null,  "130px" ]
       },
       {
         "flex": "1 101px",
         "_max-main-size": "40px",
         "_main-size": [ null,  "40px" ]
       },
     ]
 },

 {
   
   
   
   
   
   items:
     [
       {
         "flex": "0.4 50px",
         "_main-size": [ null,  "90px" ]
       },
       {
         "flex": "0.5 50px",
         "_main-size": [ null,  "100px" ]
       },
       {
         "flex": "0 90px",
         "_max-main-size": "0px",
         "_main-size": [ null,  "0px" ]
       },
     ]
 },
 {
   
   
   
   
   items:
     [
       {
         "flex": "0.45 50px",
         "_main-size": [ null,  "95px" ]
       },
       {
         "flex": "0.55 50px",
         "_main-size": [ null,  "105px" ]
       },
       {
         "flex": "0 90px",
         "_max-main-size": "0px",
         "_main-size": [ null,  "0px" ]
       },
     ]
 },

 
 
 
 
 
 
 
 
 
 
 
 

 
 {
   items:
     [
       {
         "flex": "0 0.1 300px",
         "_main-size": [ null,  "290px" ] 
       },
     ]
 },
 {
   items:
     [
       {
         "flex": "0 0.8 400px",
         "_main-size": [ null,  "240px" ] 
       },
     ]
 },

 
 {
   items:
     [
       {
         "flex": "0 0.4 150px",
         "_main-size": [ null,  "110px" ] 
       },
       {
         "flex": "0 0.2 150px",
         "_main-size": [ null,  "130px" ] 
       },
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "76px" ]
       },
       {
         "flex": "0 0.1 200px",
         "_main-size": [ null,  "184px" ]
       }
     ]
     
     
     
     
     
     
     
     
     
     
     
     
     
     
 },

 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "70px" ]
       },
       {
         "flex": "0 0.1 200px",
         "_min-main-size": "190px",
         "_main-size": [ null,  "190px" ]
       }
     ]
     
     
     
     
     
     
 },

 
 
 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "55px" ] 
       },
       {
         "flex": "0 0.1 200px",
         "_min-main-size": "250px",
         "_main-size": [ null,  "250px" ] 
       }
     ]
     
     
 },

 
 
 
 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "70px" ]
       },
       {
         "flex": "0 0.1 50px",
         "_min-main-size": "200px",
         "_main-size": [ null,  "200px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "70px" ]
       },
       {
         "flex": "0 0.1 200px",
         "_max-main-size": "150px",
         "_main-size": [ null,  "150px" ]
       }
     ]
     
     
     
     
     
     
 },

 
 
 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "90px" ]
       },
       {
         "flex": "0 0.1 200px",
         "_max-main-size": "110px",
         "_main-size": [ null,  "110px" ]
       }
     ]
     
     
     
     
     
     
     
 },

 
 
 {
   items:
     [
       {
         "flex": "1 0.3 100px",
         "_main-size": [ null,  "120px" ]
       },
       {
         "flex": "1 0.1 200px",
         "_max-main-size": "80px",
         "_main-size": [ null,  "80px" ]
       }
     ]
 },

 
 
 {
   items:
     [
       {
         "flex": "0 0.3 100px",
         "_main-size": [ null,  "76px" ]
       },
       {
         "flex": "0 0.1 150px",
         "_main-size": [ null,  "138px" ]
       },
       {
         "flex": "0 0.8 10px",
         "_min-main-size": "40px",
         "_main-size": [ null,  "40px" ]
       }
     ]
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
 },

 
 
 
 
 
 {
   items:
     [
       {
         "flex": "0 .5 300px",
         "_main-size": [ null,  "200px" ]
       },
       {
         "flex": "0 5 0px",
         "_main-size": [ null,  "0px" ]
       }
     ]
 },

 
 
 
 
 
 
 {
   items:
     [
       {
         "flex": "0 .5 300px",
         "_main-size": [ null,  "200px" ]
       },
       {
         "flex": "0 1 0.01px",
         "_main-size": [ null,  "0px" ]
       }
     ]
 },
 
 
 
 
 
 
 
 {
   items:
     [
       {
         "flex": "0 .5 300px",
         "_main-size": [ null,  "250px" ]
       },
       {
         "flex": "0 5 0.01px",
         "_main-size": [ null,  "0px" ]
       }
     ]
 },
];
