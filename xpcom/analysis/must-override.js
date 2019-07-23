





















function get_must_overrides(cls)
{
  let mos = {};
  for each (let base in cls.bases)
    for each (let m in base.type.members)
      if (hasAttribute(m, 'NS_must_override')) 
       mos[m.shortName] = m;

  return mos;
}

function process_type(t)
{
  if (t.isIncomplete || (t.kind != 'class' && t.kind != 'struct'))
    return;

  let mos = get_must_overrides(t);
  for each (let m in t.members) {
    let mos_m = mos[m.shortName]
    if (mos_m && signaturesMatch(mos_m, m))
      delete mos[m.shortName];
  }

  for each (let u in mos)
    error(t.kind + " " + t.name + " must override " + u.name, t.loc);
}
