Import('env')
env=env.Clone()

# main() files have a different extension (.cxx) to prevent them from being
# included in wildcard patterns. Later on we will put them in separate directories.

objs = env.Object( env.Glob("*.cpp") )

tests = env.Object( env.Glob("tests/*.cpp") )

bench_utf8 = env.Program( "bench-utf8", env.Object("bench-utf8.cxx") + objs )
env.Alias('bench-utf8', bench_utf8)

tests = env.Program( "test-smalls", [objs, tests], LIBS=env['LIBS'] + ['cppunit'] )
env.Alias('tests',tests)

env.Alias('all', [bench_utf8,tests])


