







































const gValidValues = [
  "10 10",
  "   10   10em  ",
  "1 2  ; 3,4",
  "1,2;3,4",
  "0 0",
  "0,0",
];

const gInvalidValues = [
  ";10 10",
  "10 10;",  
  "10 10;;",
  "1 2 3",
  "1 2 3 4",
  "1,2;3,4 ,",
  ",", " , ",
  ";", " ; ",
  "a", " a; ", ";a;",
  "", " ",
  "1,2;3,4,",
  "1,,2",
  ",1,2",
];

const gValidRotate = [
  "10",
  "20.1",
  "30.5deg",
  "0.5rad",
  "auto",
  "auto-reverse"
];

const gInvalidRotate = [
  " 10 ",
  "  10deg",
  "10 deg",
  "10deg ",
  "10 rad    ",
  "aaa",
  " 10.1 ",
];

const gValidToBy = [
 "0 0",
 "1em,2",
 "50.3em 0.2in",
 " 1,2",
 "1 2 "
];

const gInvalidToBy = [
 "0 0 0",
 "0 0,0",
 "0,0,0",
 "1emm 2",
 "1 2;",
 "1 2,",
 " 1,2 ,",
 "abc",
 ",",
 "",
 "1,,2",
 "1,2,"
];

const gValidPath = [
 "m0 0     L30 30",
 "M20,20L10    10",
 "M20,20 L30, 30h20",
 "m50 50", "M50 50",
 "m0 0", "M0, 0"
];


const gInvalidPath = [
 "M20in 20",
 "h30",
 "L50 50",
 "abc",
];



const gValidPathWithErrors = [
 "M20 20em",
 "m0 0 L30,,30",
 "M10 10 L50 50 abc",
];
