mkdir -p bin

COMPILER='gcc'
COMPILER_FLAGS='-g'

$COMPILER $COMPILER_FLAGS src/linux_main.c -o bin/finn
