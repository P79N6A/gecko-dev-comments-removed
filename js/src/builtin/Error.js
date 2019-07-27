




function ErrorToString()
{
  
  var obj = this;
  if (!IsObject(obj))
    ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "Error", "toString", "value");

  
  var name = obj.name;
  name = (name === undefined) ? "Error" : ToString(name);

  
  var msg = obj.message;
  msg = (msg === undefined) ? "" : ToString(msg);

  
  if (name === "")
    return msg;

  
  if (msg === "")
    return name;

  
  return name + ": " + msg;
}
