func fib = function(int n) int {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
};

print("How many fibonacci numbers? ");
int n = to_int(input());

int start = timestamp();
int i = 0;
for (; i <= n; i += 1)
  println("Fib number no. " + i + " = " + fib(i));

println("It took " + (timestamp() - start) + "ms to calculate " + n + " fibonacci numbers");