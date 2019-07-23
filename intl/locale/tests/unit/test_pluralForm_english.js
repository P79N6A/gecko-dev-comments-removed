








































Components.utils.import("resource://gre/modules/PluralForm.jsm");

function run_test()
{
  
  do_check_eq(2, PluralForm.numForms());

  
  for (var num = 0; num <= 200; num++)
    do_check_eq(num == 1 ? "word" : "words", PluralForm.get(num, "word;words"));

  
  do_check_eq("word", PluralForm.get(2, "word"));

  
  do_check_eq("word", PluralForm.get(2, "word;"));
}
