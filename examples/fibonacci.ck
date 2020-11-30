func fib = function(int n) int {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
};

int nums = to_int(input());
int i = 0;
int start = timestamp();
for (; i <= nums; i += 1) {
  println(i, '=', fib(i));
}

println('executed in', timestamp() - start, 'ms');