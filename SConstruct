env = Environment()

#env.Append(CCFLAGS=['-g'])
env.Append(CCFLAGS=['-mtune=native','-march=native'])
env.Append(CCFLAGS=['-O2'])
#env.Append(CCFLAGS=['-fprofile-generate=prof'])
#env.Append(CCFLAGS=['-fprofile-use=prof'])
env.Append(LIBS=['gc'])
#env.Append(LIBS=['gcov'])

env.SConscript("SConscript.py", exports='env', variant_dir='out', duplicate=False)
