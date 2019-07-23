



































public class Test7 extends Test4
{

  public int name_int = 0;
  private String str;
  public Test7(String s, String s1){
      super(s); str = s1;

  }
 
  public void Set_int(int i){
     name_int = i;
  }

  int Abs_nomod_abstract_int(int i){
      return i;
  }

  protected int Abs_protected_abstract_int(int i){
      return i;
  }

  public int Abs_public_abstract_int(int i){
      return i;
  }

  int Test9_abs_nomod_int(int i){
      return i;
  }

  protected int Test9_abs_protected_int(int i){
      return i;
  }

  public int Test9_abs_public_int(int i){
      return i;
  }


}
