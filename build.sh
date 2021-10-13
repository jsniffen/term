mkdir -p bin

COMPILER='gcc'
COMPILER_FLAGS='-g -pthread'

$COMPILER $COMPILER_FLAGS src/main_linux.c -o bin/finn
