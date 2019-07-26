








var formatter = new Intl.NumberFormat();
  
if (formatter.format(1) === formatter.format(-1)) {
    $ERROR('Intl.NumberFormat is formatting 1 and -1 the same way.');
}

if (formatter.format(-0) !== formatter.format(0)) {
    $ERROR('Intl.NumberFormat is formatting signed zeros differently.');
}

