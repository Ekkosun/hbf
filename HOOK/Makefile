hook_debug:
	clang -O3 -DDEBUG -fPIC -shared -Wall -g -o hook.so hook.c -ldl

hook:
	clang -O3 -fPIC -shared -Wall -o hook.so hook.c -ldl

test_hook:
	clang -O3 -DDEBUG -fPIC -shared -Wall -g -o hook.so test_hook.c -ldl

