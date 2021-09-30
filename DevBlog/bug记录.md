/usr/bin/ld: cannot open output file a.out: Permission denied
collect2: error: ld returned 1 exit status


>> Building dependency file for src/logger/logger.c <<<
>>> Building dependency file for src/container/container.c <<<
>>> Building dependency file for src/rules/general.c <<<
>>> Building dependency file for src/rules/c_cpp.c <<<
>>> Building dependency file for src/argtable/argtable3.c <<<
>>> Building dependency file for src/parser.c <<<
>>> Building dependency file for src/util.c <<<
>>> Building dependency file for src/main.c <<<
>>> Compiling src/main.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/main.o src/main.c -lpthread -lseccomp
>>> Compiling src/util.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/util.o src/util.c -lpthread -lseccomp
>>> Compiling src/parser.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/parser.o src/parser.c -lpthread -lseccomp
>>> Compiling src/argtable/argtable3.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/argtable3.o src/argtable/argtable3.c -lpthread -lseccomp
>>> Compiling src/rules/c_cpp.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/c_cpp.o src/rules/c_cpp.c -lpthread -lseccomp
>>> Compiling src/rules/general.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/general.o src/rules/general.c -lpthread -lseccomp
>>> Compiling src/container/container.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/container.o src/container/container.c -lpthread -lseccomp
>>> Compiling src/logger/logger.c <<<
gcc -g -Wall -Werror -O3 -std=c99 -pie -fPIC -c -o build/objects/logger.o src/logger/logger.c -lpthread -lseccomp
>>> Linking build/bin/sandbox <<<
gcc  build/objects/main.o build/objects/util.o build/objects/parser.o build/objects/argtable3.o build/objects/c_cpp.o build/objects/general.o build/objects/container.o build/objects/logger.o -o build/bin/sandbox -lpthread -lseccomp
