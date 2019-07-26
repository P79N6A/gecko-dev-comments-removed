









function FACTORY(){
   this.id = 0;
      
   this.id = func();
   
   function func(){
      return "id_string";
   }
     
}


try {
	var obj = new FACTORY();
} catch (e) {
	$ERROR('#1: var obj = new FACTORY() does not lead to throwing exception. Actual: Exception is '+e);
}





if (obj.id !== "id_string") {
	$ERROR('#2: obj.id === "id_string". Actual: obj.id ==='+obj.id);
}



