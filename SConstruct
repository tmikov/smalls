env = Environment()

#env.Append(CCFLAGS=['-g'])
env.Append(CCFLAGS=['-mtune=native','-march=native'])
env.Append(CCFLAGS=['-O2'])
#env.Append(CCFLAGS=['-fprofile-generate=prof'])
#env.Append(CCFLAGS=['-fprofile-use=prof'])
env.Append(LIBS=['gc'])
#env.Append(LIBS=['gcov'])

objs = env.Object( env.Glob("*.cpp") )

tests = env.Object( env.Glob("tests/*.cpp") )

bin = env.Program( "smalls", env.Object("main.cxx") + objs )
env.Alias('bin', bin)

tests = env.Program( "test-smalls", [objs, tests], LIBS=env['LIBS'] + ['cppunit'] )
env.Alias('tests',tests)

env.Alias('all', [bin,tests])


