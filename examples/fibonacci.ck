include "fib.ck"

int i = 0;
int start = timestamp();
int nums = 25;
for (; i <= nums; i += 1) {
  println(i, '=', fib(i));
}

println('executed in', timestamp() - start, 'ms');