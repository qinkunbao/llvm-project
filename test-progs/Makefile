hello: hello.c Makefile
	../ra/bin/clang hello.c -ffunction-sections -Wl,--gc-sections -Wl,--module-symbol,foo -fuse-ld=lld -pie -g -fPIE -o hello
