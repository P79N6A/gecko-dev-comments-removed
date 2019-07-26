









function FACTORY(){
   this.id = 0;
   
   this.func = function (){
      return 5;
   }
   
   this.id = this.func();
     
}


try {
	var obj = new FACTORY();
} catch (e) {
	$ERROR('#1: var obj = new FACTORY() does not lead to throwing exception. Actual: Exception is '+e);
}





if (obj.id !== 5) {
	$ERROR('#2: obj.id === 5. Actual: obj.id ==='+obj.id);
}



