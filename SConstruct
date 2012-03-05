env = Environment()

env.Append(CCFLAGS=['-g'])
env.Append(LIBS=['gc'])

objs = env.Object( env.Glob("*.cpp") )

tests = env.Object( env.Glob("tests/*.cpp") )

bin = env.Program( "smalls", env.Object("main.cxx") + objs )
env.Alias('bin', bin)

tests = env.Program( "test-smalls", [objs, tests], LIBS=env['LIBS'] + ['cppunit'] )
env.Alias('tests',tests)

env.Alias('all', [bin,tests])


