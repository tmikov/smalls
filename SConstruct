# -*- coding: utf-8 -*-
import os

# Age of the file (in seconds) before using its cached signature (MD5) instead of calculating
# a new one. Negative values means 'never cache', 0 means 'always cache'
SetOption( 'max_drift', 60*60*4 )


# Extract CC and CXX from the environment, unless they are already specified on the command line
varsDict=ARGUMENTS
if os.environ.get('CC') and not varsDict.get('CC'):
  varsDict['CC'] = os.environ['CC']
if os.environ.get('CXX') and not varsDict.get('CXX'):
  varsDict['CXX'] = os.environ['CXX']

# The configuration vairables will be stored in '_vars.cache'. The default values may optionally
# be initialized from 'config.vars'
vars = Variables(['_vars.cache', 'config.vars'], varsDict)

# Declare configuration variables
vars.AddVariables(
  BoolVariable('inherit_env','Inherit system environment',False),
  PathVariable('out','Build directory','build',PathVariable.PathAccept),
  ('cross', 'Cross-compile prefix', ""),
  ('CC', 'The C compiler'),
  ('CXX', 'The C++ compiler'),
  ('sys_usr', 'System root usr dir', "/usr"),
  ('extra_flags', 'Additional compiler and linker flags', ""),
  BoolVariable('debug','',False),
  PackageVariable('boost','boost',False)
)

env = Environment( variables=vars )

Help(vars.GenerateHelpText(env))
vars.Save('_vars.cache',env)

env.Decider('MD5-timestamp')
env.CacheDir("_build-cache")

if env['inherit_env']:
  env['ENV']=os.environ

if env['cross']:
  env['CC']="$cross-gcc"
  env['CXX']="$cross-g++"
  env['LINK']="$cross-g++"
  env['AR']="$cross-ar"
  env['RANLIB']="$cross-ranlib"

if env['debug']:
  env['out'] += "/debug"
  env.AppendUnique(CPPDEFINES=['DEBUG'])
  env.AppendUnique(CCFLAGS=['-g'])
else:
  env['out'] += "/release"
  env.AppendUnique(CPPDEFINES=['NDEBUG'])
  env.Append(CCFLAGS=['-mtune=native','-march=native'])
  env.Append(CCFLAGS=['-O2'])
  #env.Append(CCFLAGS=['-fprofile-generate=prof'])
  #env.Append(CCFLAGS=['-fprofile-use=prof'])

env.Append(CCFLAGS=['-Wall'])

env.Append(LIBS=['gc'])
#env.Append(LIBS=['gcov'])

# A dummy object to avoid the deep copying of environments
class Modules:
  def __init__ ( self ):
    self.dict = {}
  
  def __getitem__ ( self, x ): return self.dict.__getitem__( x )
  def __setitem__ ( self, x, y ): self.dict.__setitem__( x, y )
  def __repr__ ( self ) : return self.dict.__repr__()


# Modules will store their object files here
env['module'] = Modules()
env['module']['utest'] = []

Export("env")

env.SConscript( dirs="cxxp1", variant_dir=env['out'], duplicate=False )

