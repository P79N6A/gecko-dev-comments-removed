










var count=0;
for (p in Number) count++;
if (count > 0){
  $ERROR('#1: count=0; for (p in Number) count++; count > 0. Actual: ' + (count));
}

