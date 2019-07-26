











try{
	eval("FOR1 : for(var i=1;i<2;i++){ LABEL1 : do {var x =1;break\u000AFOR1;var y=2;} while(0);}");
	if (i!==2) {
		$ERROR('#1: Since LineTerminator(U-000A) between break and Identifier not allowed break evaluates without label');
	}
} catch(e){
	$ERROR('#1.1: eval("FOR1 : for(var i=1;i<2;i++){ LABEL1 : do {var x =1;break\\u000AFOR1;var y=2;} while(0);}") does not lead to throwing exception');
}





try{
	eval("FOR2 : for(var i=1;i<2;i++){ LABEL2 : do {var x =1;break\u000DFOR2;var y=2;} while(0);}");
	if (i!==2) {
		$ERROR('#2: Since LineTerminator(U-000D) between break and Identifier not allowed break evaluates without label');
	}
} catch(e){
	$ERROR('#2.1: eval("FOR2 : for(var i=1;i<2;i++){ LABEL2 : do {var x =1;break\\u000DFOR2;var y=2;} while(0);}") does not lead to throwing exception');
}





try{
	eval("FOR3 : for(var i=1;i<2;i++){ LABEL3 : do {var x =1;break\u2028FOR3;var y=2;} while(0);}");
	if (i!==2) {
		$ERROR('#3: Since LineTerminator(U-2028) between break and Identifier not allowed break evaluates without label');
	}
} catch(e){
	$ERROR('#3.1: eval("FOR3 : for(var i=1;i<2;i++){ LABEL3 : do {var x =1;break\\u2028FOR3;var y=2;} while(0);}") does not lead to throwing exception');
}





try{
	eval("FOR4 : for(var i=1;i<2;i++){ LABEL4 : do {var x =1;break\u2029FOR4;var y=2;} while(0);}");
	if (i!==2) {
		$ERROR('#4: Since LineTerminator(U-2029) between break and Identifier not allowed break evaluates without label');
	}
} catch(e){
	$ERROR('#4.1: eval("FOR4 : for(var i=1;i<2;i++){ LABEL4 : do {var x =1;break\\u2029FOR4;var y=2;} while(0);}") does not lead to throwing exception');
}





