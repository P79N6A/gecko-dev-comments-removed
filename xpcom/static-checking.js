












var gClassMap = {};

function ClassType(name)
{
  this.name = name;
}

ClassType.prototype = {
  final: false,
  stack: false,
};

function process_class(c)
{
  get_class(c, true);
}










function get_class(c, allowIncomplete)
{
  var classattr, base, member, type, realtype, foundConstructor;
  var bases = [];

  if (c.isIncomplete) {
    if (allowIncomplete)
      return null;

    throw Error("Can't process incomplete type '" + c + "'.");
  }

  if (gClassMap.hasOwnProperty(c.name)) {
    return gClassMap[c.name];
  }

  for each (base in c.bases) {
      realtype = get_class(base, allowIncomplete);
      if (realtype == null) {
        error("Complete type " + c + " has incomplete base " + base);
        return null;
      }

      bases.push(realtype);
  }

  function hasAttribute(attrname)
  {
    var attr;

    if (c.attributes === undefined)
      return false;

    for each (attr in c.attributes) {
      if (attr.name == 'user' && attr.value[0] == attrname) {
        return true;
      }
    }

    return false;
  }

  classattr = new ClassType(c.name);
  gClassMap[c.name] = classattr;

  

  if (hasAttribute('NS_final')) {
    classattr.final = true;
  }

  

  if (hasAttribute('NS_stack')) {
    classattr.stack = true;
  }
  else {
    for each (base in bases) {
      if (base.stack) {
        classattr.stack = true;
        break;
      }
    }
  }

  if (!classattr.stack) {
    
    for each (member in c.members) {
      if (member.isFunction)
        continue;

      type = member.type;

      
      while (true) {
          if (type === undefined) {
              break;
          }
              
          if (type.isArray) {
              type = type.type;
              continue;
          }
          if (type.typedef) {
              type = type.typedef;
              continue;
          }
          break;
      }

      if (type === undefined) {
          warning("incomplete type  for member " + member + ".");
          continue;
      }

      if (type.isPointer || type.isReference) {
          continue;
      }

      if (!type.kind || (type.kind != 'class' && type.kind != 'struct')) {
          continue;
      }

      var membertype = get_class(type, false);
      if (membertype.stack) {
        classattr.stack = true;
        break;
      }
    }
  }

  

  for each (base in bases) {
    if (base.final) {
      error("class '" + c.name + "' inherits from final class '" + base.name + "'.");
    }
  }

  
  
  if (classattr.stack) {
    foundConstructor = false;
    for each (member in c.members) {
      if (member.isConstructor) {
        foundConstructor = true;
        break;
      }
    }

    if (!foundConstructor) {
      warning(c.loc + ": class " + c.name + " is marked stack-only but doesn't have a constructor. Static checking can't detect instantiations of this class properly.");
    }
  }

  return classattr;
}




function unwrapArray(t)
{
  while (t.isArray) {
    t = t.type;
  }
  return t;
}

function process_function(f, stmts)
{
  var stmt;
  function getLocation()
  {
    if (stmt.loc)
      return stmt.loc;

    return f.loc;
  }

  function processVar(v)
  {
    if (v.isConstructor &&
        v.fieldOf &&
        get_class(v.methodOf, false).stack &&
        v.fieldOf.type.isPointer) {
      error(getLocation() + ": constructed object of type '" +
            v.methodOf.name + "' not on the stack.");
    }
  }

  for each (stmt in stmts) {
    iter(processVar, stmt.statements);
  }
}
