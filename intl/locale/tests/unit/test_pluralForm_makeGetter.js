









































Components.utils.import("resource://gre/modules/PluralForm.jsm");

function run_test()
{
  
  let [get, numForms] = PluralForm.makeGetter(11);

  
  do_check_eq(5, numForms());

  
  let words = "is 1;is 2;is 3-6;is 7-10;everything else";

  let test = function(text, low, high) {
    for (let num = low; num <= high; num++)
      do_check_eq(text, get(num, words));
  };

  
  test("everything else", 0, 0);
  test("is 1", 1, 1);
  test("is 2", 2, 2);
  test("is 3-6", 3, 6);
  test("is 7-10", 7, 10);
  test("everything else", 11, 200);
}
