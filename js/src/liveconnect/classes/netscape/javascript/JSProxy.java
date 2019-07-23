











































package netscape.javascript;
import java.applet.Applet;

public interface JSProxy {
    Object  getMember(JSObject jso, String name);
    Object  getSlot(JSObject jso, int index);
    void    setMember(JSObject jso, String name, Object value);
    void    setSlot(JSObject jso, int index, Object value);
    void    removeMember(JSObject jso, String name);
    Object  call(JSObject jso, String methodName, Object args[]);
    Object  eval(JSObject jso, String s);
    String      toString(JSObject jso);
    JSObject    getWindow(Applet applet);
}
