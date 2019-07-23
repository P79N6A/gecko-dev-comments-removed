



































public abstract class Test4 extends Test9{
public String name_string;
public int name_int = 0;

public Test4(String s){
   System.out.println("name-string set to "+s);
   name_string = s;
}

private Test4(int i){
   System.out.println("name-int set to "+i);
   name_int = i;
}

protected Test4(){
   name_int = 10;
}

Test4(int i, String str){
   name_string = str;
   name_int  = i;
}

public int Ret_int(){
   return 10;
}

public abstract int Abs_public_abstract_int(int i);

protected abstract int Abs_protected_abstract_int(int i);

abstract int Abs_nomod_abstract_int(int i);


}
