


function run_test() {
  do_check_matches({x:1}, {});         
  do_check_matches({x:1}, {x:2});      
  do_check_matches({x:undefined}, {});

  
  do_check_matches([3,4,5], [3,5,5]);  
  do_check_matches([3,4,5], [3,4,5,6]);
}
