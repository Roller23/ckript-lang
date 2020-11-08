# Ckript language
An interpreter for my own programming language (Ckript)

Cheatsheet:

Built in types

* int
* str
* double
* arr
* obj
* func

Declaring variables

```
int number = 5;
```

Declaring functions

```
func name = function(args...) return_type { function_body };
```

Example:

```
func square = function(int x) int {
  return x * x;
};
```

All functions are lambdas (values of type func)

```
int four = (function(int x) {
  return x * x;
})(2); // immediate invocation
```



