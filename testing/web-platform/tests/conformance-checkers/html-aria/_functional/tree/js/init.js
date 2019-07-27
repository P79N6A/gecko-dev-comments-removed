function myLoad(){
	$$('[role="tree"]').each(function(elm){
		
		Aria.Trees.push(new Aria.Tree(elm));
	});
}
Event.observe(window, 'load', myLoad); 

