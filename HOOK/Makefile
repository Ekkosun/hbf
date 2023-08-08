hook_debug:
	$CC -O3 -DDEBUG -fPIC -shared -Wall -g -o hook.so hook.c -ldl

hook:
	$CC -O3 -fPIC -shared -Wall -o hook.so hook.c -ldl

test_hook:
	$CC -O3 -DDEBUG -fPIC -shared -Wall -g -o hook.so test_hook.c -ldl

