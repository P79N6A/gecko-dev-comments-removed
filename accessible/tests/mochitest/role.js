






function getRole(aAccOrElmOrID)
{
  var acc = getAccessible(aAccOrElmOrID);
  if (!acc)
    return -1;

  var role = -1;
  try {
    role = acc.finalRole;
  } catch(e) {
    ok(false, "Role for " + aAccOrElmOrID + " could not be retrieved!");
  }

  return role;
}







function testRole(aAccOrElmOrID, aRole)
{
  var role = getRole(aAccOrElmOrID);
  is(role, aRole, "Wrong role for " + aAccOrElmOrID + "!");
}
