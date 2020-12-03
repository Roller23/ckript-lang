include "fib.ck"

int nums = to_int(10);
int i = 0;
int start = timestamp();
for (; i <= nums; i += 1) {
  println(i, '=', fib(i));
}

println('executed in', timestamp() - start, 'ms');