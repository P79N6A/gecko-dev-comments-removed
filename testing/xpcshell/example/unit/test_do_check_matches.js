


function run_test() {
  do_check_matches({x:1}, {x:1});      
  todo_check_matches({x:1}, {});       
  todo_check_matches({x:1}, {x:2});    
  do_check_matches({x:1}, {x:1, y:2}); 

  
  do_check_matches({x:"foo", y:"bar"}, {y:"bar", x:"foo"});

  do_check_matches({x:undefined}, {x:1});
  do_check_matches({x:undefined}, {x:2});
  todo_check_matches({x:undefined}, {y:2});

  
  do_check_matches({a:1, b:{c:2,d:undefined}}, {a:1, b:{c:2,d:3}});

  
  do_check_matches([3,4,5], [3,4,5]);    
  todo_check_matches([3,4,5], [3,5,5]);  
  todo_check_matches([3,4,5], [3,4,5,6]);

  
  do_check_matches({foo:function (v) v.length == 2}, {foo:"hi"});
  todo_check_matches({foo:function (v) v.length == 2}, {bar:"hi"});
  todo_check_matches({foo:function (v) v.length == 2}, {foo:"hello"});

  
  
  
  
  do_check_matches({0:0, 1:1, length:2}, [0,1]); 
  do_check_matches({0:1}, [1,2]);                
  do_check_matches([0], {0:0, length:1});        
}
