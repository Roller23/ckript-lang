func fib = function(int n) int {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
};