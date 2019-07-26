



"""New implementation of Visual Studio project generation for SCons."""

import os
import random

import gyp.common





try:
  import hashlib
  _new_md5 = hashlib.md5
except ImportError:
  import md5
  _new_md5 = md5.new



random.seed()


ENTRY_TYPE_GUIDS = {
    'project': '{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}',
    'folder': '{2150E333-8FDC-42A3-9474-1A3956D46DE8}',
}





def MakeGuid(name, seed='msvs_new'):
  """Returns a GUID for the specified target name.

  Args:
    name: Target name.
    seed: Seed for MD5 hash.
  Returns:
    A GUID-line string calculated from the name and seed.

  This generates something which looks like a GUID, but depends only on the
  name and seed.  This means the same name/seed will always generate the same
  GUID, so that projects and solutions which refer to each other can explicitly
  determine the GUID to refer to explicitly.  It also means that the GUID will
  not change when the project for a target is rebuilt.
  """
  
  d = _new_md5(str(seed) + str(name)).hexdigest().upper()
  
  guid = ('{' + d[:8] + '-' + d[8:12] + '-' + d[12:16] + '-' + d[16:20]
          + '-' + d[20:32] + '}')
  return guid




class MSVSSolutionEntry(object):
  def __cmp__(self, other):
    
    return cmp((self.name, self.get_guid()), (other.name, other.get_guid()))


class MSVSFolder(MSVSSolutionEntry):
  """Folder in a Visual Studio project or solution."""

  def __init__(self, path, name = None, entries = None,
               guid = None, items = None):
    """Initializes the folder.

    Args:
      path: Full path to the folder.
      name: Name of the folder.
      entries: List of folder entries to nest inside this folder.  May contain
          Folder or Project objects.  May be None, if the folder is empty.
      guid: GUID to use for folder, if not None.
      items: List of solution items to include in the folder project.  May be
          None, if the folder does not directly contain items.
    """
    if name:
      self.name = name
    else:
      
      self.name = os.path.basename(path)

    self.path = path
    self.guid = guid

    
    self.entries = sorted(list(entries or []))
    self.items = list(items or [])

    self.entry_type_guid = ENTRY_TYPE_GUIDS['folder']

  def get_guid(self):
    if self.guid is None:
      
      self.guid = MakeGuid(self.path, seed='msvs_folder')
    return self.guid





class MSVSProject(MSVSSolutionEntry):
  """Visual Studio project."""

  def __init__(self, path, name = None, dependencies = None, guid = None,
               spec = None, build_file = None, config_platform_overrides = None,
               fixpath_prefix = None):
    """Initializes the project.

    Args:
      path: Absolute path to the project file.
      name: Name of project.  If None, the name will be the same as the base
          name of the project file.
      dependencies: List of other Project objects this project is dependent
          upon, if not None.
      guid: GUID to use for project, if not None.
      spec: Dictionary specifying how to build this project.
      build_file: Filename of the .gyp file that the vcproj file comes from.
      config_platform_overrides: optional dict of configuration platforms to
          used in place of the default for this target.
      fixpath_prefix: the path used to adjust the behavior of _fixpath
    """
    self.path = path
    self.guid = guid
    self.spec = spec
    self.build_file = build_file
    
    self.name = name or os.path.splitext(os.path.basename(path))[0]

    
    self.dependencies = list(dependencies or [])

    self.entry_type_guid = ENTRY_TYPE_GUIDS['project']

    if config_platform_overrides:
      self.config_platform_overrides = config_platform_overrides
    else:
      self.config_platform_overrides = {}
    self.fixpath_prefix = fixpath_prefix
    self.msbuild_toolset = None

  def set_dependencies(self, dependencies):
    self.dependencies = list(dependencies or [])

  def get_guid(self):
    if self.guid is None:
      
      
      
      
      
      
      
      
      
      
      
      
      self.guid = MakeGuid(self.name)
    return self.guid

  def set_msbuild_toolset(self, msbuild_toolset):
    self.msbuild_toolset = msbuild_toolset




class MSVSSolution:
  """Visual Studio solution."""

  def __init__(self, path, version, entries=None, variants=None,
               websiteProperties=True):
    """Initializes the solution.

    Args:
      path: Path to solution file.
      version: Format version to emit.
      entries: List of entries in solution.  May contain Folder or Project
          objects.  May be None, if the folder is empty.
      variants: List of build variant strings.  If none, a default list will
          be used.
      websiteProperties: Flag to decide if the website properties section
          is generated.
    """
    self.path = path
    self.websiteProperties = websiteProperties
    self.version = version

    
    self.entries = list(entries or [])

    if variants:
      
      self.variants = variants[:]
    else:
      
      self.variants = ['Debug|Win32', 'Release|Win32']
    
    
    
    


    
    
    self.Write()


  def Write(self, writer=gyp.common.WriteOnDiff):
    """Writes the solution file to disk.

    Raises:
      IndexError: An entry appears multiple times.
    """
    
    all_entries = set()
    entries_to_check = self.entries[:]
    while entries_to_check:
      e = entries_to_check.pop(0)

      
      if e in all_entries:
        continue

      all_entries.add(e)

      
      if isinstance(e, MSVSFolder):
        entries_to_check += e.entries

    all_entries = sorted(all_entries)

    
    f = writer(self.path)
    f.write('Microsoft Visual Studio Solution File, '
            'Format Version %s\r\n' % self.version.SolutionVersion())
    f.write('# %s\r\n' % self.version.Description())

    
    sln_root = os.path.split(self.path)[0]
    for e in all_entries:
      relative_path = gyp.common.RelativePath(e.path, sln_root)
      
      
      folder_name = relative_path.replace('/', '\\') or '.'
      f.write('Project("%s") = "%s", "%s", "%s"\r\n' % (
          e.entry_type_guid,          
          e.name,                     
          folder_name,                
          e.get_guid(),               
      ))

      
      if self.websiteProperties:
        f.write('\tProjectSection(WebsiteProperties) = preProject\r\n'
                '\t\tDebug.AspNetCompiler.Debug = "True"\r\n'
                '\t\tRelease.AspNetCompiler.Debug = "False"\r\n'
                '\tEndProjectSection\r\n')

      if isinstance(e, MSVSFolder):
        if e.items:
          f.write('\tProjectSection(SolutionItems) = preProject\r\n')
          for i in e.items:
            f.write('\t\t%s = %s\r\n' % (i, i))
          f.write('\tEndProjectSection\r\n')

      if isinstance(e, MSVSProject):
        if e.dependencies:
          f.write('\tProjectSection(ProjectDependencies) = postProject\r\n')
          for d in e.dependencies:
            f.write('\t\t%s = %s\r\n' % (d.get_guid(), d.get_guid()))
          f.write('\tEndProjectSection\r\n')

      f.write('EndProject\r\n')

    
    f.write('Global\r\n')

    
    f.write('\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n')
    for v in self.variants:
      f.write('\t\t%s = %s\r\n' % (v, v))
    f.write('\tEndGlobalSection\r\n')

    
    config_guids = []
    config_guids_overrides = {}
    for e in all_entries:
      if isinstance(e, MSVSProject):
        config_guids.append(e.get_guid())
        config_guids_overrides[e.get_guid()] = e.config_platform_overrides
    config_guids.sort()

    f.write('\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n')
    for g in config_guids:
      for v in self.variants:
        nv = config_guids_overrides[g].get(v, v)
        
        
        f.write('\t\t%s.%s.ActiveCfg = %s\r\n' % (
            g,              
            v,              
            nv,             
        ))

        
        f.write('\t\t%s.%s.Build.0 = %s\r\n' % (
            g,              
            v,              
            nv,             
        ))
    f.write('\tEndGlobalSection\r\n')

    
    
    f.write('\tGlobalSection(SolutionProperties) = preSolution\r\n')
    f.write('\t\tHideSolutionNode = FALSE\r\n')
    f.write('\tEndGlobalSection\r\n')

    
    
    f.write('\tGlobalSection(NestedProjects) = preSolution\r\n')
    for e in all_entries:
      if not isinstance(e, MSVSFolder):
        continue        
      for subentry in e.entries:
        f.write('\t\t%s = %s\r\n' % (subentry.get_guid(), e.get_guid()))
    f.write('\tEndGlobalSection\r\n')

    f.write('EndGlobal\r\n')

    f.close()
