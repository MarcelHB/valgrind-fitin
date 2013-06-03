# Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
tests="dup tri trump cflow timer"
crafts="hand auto"
olevels="O3 O2 Os O0"

for t in $tests; do
  for c in $crafts; do
    for o in $olevels; do
      echo build/basic-$o.o
      echo build/$t-$c-$o.o
    done
    echo build/$t-$c.log
  done
done | sort -u | xargs redo-ifchange
for o in $olevels; do
  echo "extern int basic_${o}(int);"
done

for t in $tests; do
  for c in $crafts; do
    for o in $olevels; do
      echo "extern int ${t}_${c}_${o}(int);"
    done
  done
done

echo
echo "void all_tests() {"
for o in $olevels; do
  echo "  run_test(start, basic_${o}, \"${o}\");"
done
echo "  summarize(\"basic\", 1, iterations, true, false);" 

for t in $tests; do
  for o in $olevels; do
    for c in $crafts; do
      echo "  run_test(start, ${t}_${c}_${o}, \"${c} ${o}\");"
    done
  done
  echo "  summarize(\"${t}\", 1, iterations, true, false);" 
  echo
done
echo "}"

