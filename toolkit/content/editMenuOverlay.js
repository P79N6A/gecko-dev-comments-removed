






function goUpdateGlobalEditMenuItems()
{
  
  
  
  
  
  if (typeof gEditUIVisible != "undefined" && !gEditUIVisible)
    return;

  goUpdateCommand("cmd_undo");
  goUpdateCommand("cmd_redo");
  goUpdateCommand("cmd_cut");
  goUpdateCommand("cmd_copy");
  goUpdateCommand("cmd_paste");
  goUpdateCommand("cmd_selectAll");
  goUpdateCommand("cmd_delete");
  goUpdateCommand("cmd_switchTextDirection");
}


function goUpdateUndoEditMenuItems()
{
  goUpdateCommand("cmd_undo");
  goUpdateCommand("cmd_redo");
}


function goUpdatePasteMenuItems()
{
  goUpdateCommand("cmd_paste");
}
