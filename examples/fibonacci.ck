func fib = function(int n) int {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
};

print("How many fibonacci numbers? ");
int n = to_int(input());

int start = timestamp();
int i = 0;
for (; i <= n; i += 1)
  println('Fib number no. @1 = @2'(i, fib(i)));

println('It took @1ms to calculate @2 fibonacci numbers'(timestamp() - start, n));