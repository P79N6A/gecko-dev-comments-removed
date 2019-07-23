



















function testSelect(aID, aNames, aRoles, aStates, aUndesiredStates)
{
  
  var acc = getAccessible(aID);
  if (!acc) {
    return;
  }

  testThis(aID, acc, aNames, aRoles, aStates, aUndesiredStates, 0);
}
















function testThis(aID, aAcc, aNames, aRoles, aStates, aUndesiredStates, aIndex)
{
  if (aIndex >= aNames.length)
    return;  
  else if (!aAcc) {
    ok(false, "No accessible for " + aID + " at index " + aIndex + "!");
    return;
  }

  is(aAcc.name, aNames[aIndex],
     "wrong name for " + aID + " at index " + aIndex + "!");
  var role = getRole(aAcc);
  is(role, aRoles[aIndex],
     "Wrong role for " + aID + " at index " + aIndex + "!");
  testStates(aID, aAcc, aStates, aUndesiredStates, aIndex);
  switch(role) {
    case ROLE_COMBOBOX:
    case ROLE_COMBOBOX_LIST:
    case ROLE_LABEL:
    case ROLE_LIST:
      
      
      var acc = null;
      try {
        acc = aAcc.firstChild;
      } catch(e) {}
      testThis(aID, acc, aNames, aRoles, aStates, aUndesiredStates, ++aIndex);
      break;
    case ROLE_COMBOBOX_OPTION:
    case ROLE_OPTION:
    case ROLE_TEXT_LEAF:
      
      var acc = null;
      try {
        acc = aAcc.nextSibling;
      } catch(e) {}
      testThis(aID, acc, aNames, aRoles, aStates, aUndesiredStates, ++aIndex);
      break;
    default:
      break;
  }
}












function testStates(aID, aAcc, aStates, aUndesiredStates, aIndex)
{
  var state = {}, extraState = {};
  aAcc.getState(state, extraState);
  if (aStates[aIndex] != 0)
    is(state.value & aStates[aIndex], aStates[aIndex],
       "Wrong state bits for " + aID + " at index " + aIndex + "!");
  if (aUndesiredStates[aIndex] != 0)
    is(state.value & aUndesiredStates[aIndex], 0,
       "Wrong undesired state bits for " + aID + " at index " + aIndex + "!");
}
