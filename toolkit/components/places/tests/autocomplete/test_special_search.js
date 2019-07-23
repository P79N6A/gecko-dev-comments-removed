












































let kURIs = [
  "http://url/",
  "http://url/2",
  "http://foo.bar/",
  "http://foo.bar/2",
  "http://url/star",
  "http://url/star/2",
  "http://foo.bar/star",
  "http://foo.bar/star/2",
  "http://url/tag",
  "http://url/tag/2",
  "http://foo.bar/tag",
  "http://foo.bar/tag/2",
];
let kTitles = [
  "title",
  "foo.bar",
];


addPageBook(0, 0); 
addPageBook(1, 1); 
addPageBook(2, 0); 
addPageBook(3, 1); 


addPageBook(4, 0, 0); 
addPageBook(5, 1, 1); 
addPageBook(6, 0, 0); 
addPageBook(7, 1, 1); 


addPageBook(8, 0, 0, [1]); 
addPageBook(9, 1, 1, [1]); 
addPageBook(10, 0, 0, [1]); 
addPageBook(11, 1, 1, [1]); 



removePages([4,6,7,8,9,11]);

markTyped([0,3,10]);



let gTests = [
  
  ["0: History restrict",
   "^", [0,1,2,3,5,10], ignoreTags],
  ["1: Star restrict",
   "*", [4,5,6,7,8,9,10,11]],
  ["2: Tag restrict",
   "+", [8,9,10,11]],

  
  ["3: Special as first word",
   "^ foo bar", [1,2,3,5,10], ignoreTags],
  ["4: Special as middle word",
   "foo ^ bar", [1,2,3,5,10], ignoreTags],
  ["5: Special as last word",
   "foo bar ^", [1,2,3,5,10], ignoreTags],

  
  ["6.1: foo ^ -> history",
   "foo ^", [1,2,3,5,10], ignoreTags],
  ["6.2: foo | -> history (change pref)",
   "foo |", [1,2,3,5,10], function() {ignoreTags(); changeRestrict("history", "|"); }],
  ["7.1: foo * -> is star",
   "foo *", [5,6,7,8,9,10,11], function() resetRestrict("history")],
  ["7.2: foo | -> is star (change pref)",
   "foo |", [5,6,7,8,9,10,11], function() changeRestrict("bookmark", "|")],
  ["8.1: foo # -> in title",
   "foo #", [1,3,5,7,8,9,10,11], function() resetRestrict("bookmark")],
  ["8.2: foo | -> in title (change pref)",
   "foo |", [1,3,5,7,8,9,10,11], function() changeRestrict("title", "|")],
  ["9.1: foo @ -> in url",
   "foo @", [2,3,6,7,10,11], function() resetRestrict("title")],
  ["9.2: foo | -> in url (change pref)",
   "foo |", [2,3,6,7,10,11], function() changeRestrict("url", "|")],
  ["10: foo + -> is tag",
   "foo +", [8,9,10,11], function() resetRestrict("url")],
  ["10.2: foo | -> is tag (change pref)",
   "foo |", [8,9,10,11], function() changeRestrict("tag", "|")],
  ["10.3: foo ~ -> is typed",
   "foo ~", [3,10], function() resetRestrict("tag")],
  ["10.4: foo | -> is typed (change pref)",
   "foo |", [3,10], function() changeRestrict("typed", "|")],

  
  ["11: foo ^ * -> history, is star",
   "foo ^ *", [5,10], function() resetRestrict("typed")],
  ["12: foo ^ # -> history, in title",
   "foo ^ #", [1,3,5,10], ignoreTags],
  ["13: foo ^ @ -> history, in url",
   "foo ^ @", [2,3,10], ignoreTags],
  ["14: foo ^ + -> history, is tag",
   "foo ^ +", [10]],
  ["14.1: foo ^ ~ -> history, is typed",
   "foo ^ ~", [3,10], ignoreTags],
  ["15: foo * # -> is star, in title",
   "foo * #", [5,7,8,9,10,11]],
  ["16: foo * @ -> is star, in url",
   "foo * @", [6,7,10,11]],
  ["17: foo * + -> same as +",
   "foo * +", [8,9,10,11]],
  ["17.1: foo * ~ -> is star, is typed",
   "foo * ~", [10]],
  ["18: foo # @ -> in title, in url",
   "foo # @", [3,7,10,11]],
  ["19: foo # + -> in title, is tag",
   "foo # +", [8,9,10,11]],
  ["19.1: foo # ~ -> in title, is typed",
   "foo # ~", [3,10]],
  ["20: foo @ + -> in url, is tag",
   "foo @ +", [10,11]],
  ["20.1: foo @ ~ -> in url, is typed",
   "foo @ ~", [3,10]],
  ["20.2: foo + ~ -> is tag, is typed",
   "foo + ~", [10]],

  
  ["21: foo -> default history",
   "foo", [1,2,3,5,10], function() makeDefault(1)],
  ["22: foo -> default history, is star",
   "foo", [5,10], function() makeDefault(3)],
  ["22.1: foo -> default history, is star, is typed",
   "foo", [10], function() makeDefault(35)],
  ["23: foo -> default history, is star, in url",
   "foo", [10], function() makeDefault(19)],

  
  ["24: foo -> default is star, in url",
   "foo", [6,7,10,11], function() makeDefault(18)],
  ["25: foo -> default in url",
   "foo", [2,3,6,7,10,11], function() makeDefault(16)],
];

function makeDefault(aDefault) {
  
  if ((aDefault & 1) && !((aDefault & 2) || (aDefault & 4)))
    ignoreTags();

  prefs.setIntPref("browser.urlbar.default.behavior", aDefault);
}

function changeRestrict(aType, aChar)
{
  let branch = "browser.urlbar.";
  
  if (aType == "title" || aType == "url")
    branch += "match.";
  else
    branch += "restrict.";

  print("changing restrict for " + aType + " to '" + aChar + "'");
  prefs.setCharPref(branch + aType, aChar);
}

function resetRestrict(aType)
{
  let branch = "browser.urlbar.";
  
  if (aType == "title" || aType == "url")
    branch += "match.";
  else
    branch += "restrict.";

  if (prefs.prefHasUserValue(branch + aType))
    prefs.clearUserPref(branch + aType);
}
