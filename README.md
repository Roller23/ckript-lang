# Ckript language
An interpreter for my own programming language (Ckript)

Compile with `g++` (`clang++` won't compile it). `MingGW` and `MSVC` have not been tested yet.
If you have `g++` and `Make` installed simply running ``make`` should compile the whole project.

Usage:

```
./ckript <input file>
```
or
```
./ckript
```
(will launch the Ckript shell)

Cheatsheet:

Built in types

* int
* str
* double
* arr
* obj
* func

## Declaring variables

```
int number = 5;
const double number2 = 5.2132; // constant variable
```

## Declaring functions

```
func name = function(type args...) return_type { function_body };
```

Example:

```
func square = function(int x) int {
  return x * x;
};
```

All functions are lambdas and their type is `func`

```
int four = (function(int x) int {
  return x * x;
})(2); // immediate invocation
```

Functions can also be passed as parameters

```
func square = function(int x, func callback) void {
  int result = x * x;
  callback(result);
};

square(5, function(int result) void {
  // do something with the result...
});

```

## Strings

```
str string = "something";
str string2 = string + " new"; // concatenation
str pi_str = "this is pi " + 3.14; // will stringify the number
```

## Memory allocation

Use ``alloc`` to allocate something on the heap, ``del`` to delete from the heap, ``ref`` to get the pointer of that value

```
alloc int a = 5; // 5 is stored on the heap and 'a' is a pointer to that location
ref int a_ptr = a; // both 'a_ptr' and 'a' point to the same location on the heap

del a_ptr;

// 'a_ptr' is no longer a pointer
// 'a' still points to a location on the heap and using it may cause undefined behavior
```

Assigning an allocated variable to a variable that is not a reference will copy the actual value

```
alloc double pi = 3.14;
double pi_copy = pi; // 'pi_copy' is not a pointer, it holds the value 3.14

del pi_copy; // will fail
```

Function arguments can also be references

```
func change_me = function(ref int arg) void {
  arg += 3;
};

alloc int a = 3;

change_me(a);

int b = a + 4; // 'b' is 10

```

## Arrays

```
arr my_array1 = array() int; // an empty array that holds ints
arr my_array2 = array("hello", "world") str; // an array of strings with some initial values

my_array2 += "!"; // appending to the array
my_array2 = "Test" + my_array2; // prepending to the array

my_array1 ^= array(1, 2, 3) int; // array concatenation

int element = array[1]; // getting the value
#element[0] = 42; // setting a new value (# is required)

```

## Objects and classes

Define a class like so

```
class Person(str name, int age);
```

Create a new instance

```
obj Mark = Person("Mark", 24);
```

Access a member

```
int MarksAge = Mark.age;
```

Assign a new value to a member

```
$Mark.age = 31; // ($ is required)
```

Objects can have member objects, object members can be references

```
class Food(str name);
class Animal(ref str type, obj food);

alloc str type = "monkey";
obj monkey = Animal(type, Food("banana"));

monkey.food.name; // banana
```

## 'this' variable

If one of the object's members is a function, a special variable called `this` will be automatically pushed onto its stack.
`this` is a copy of the object that is holding the function effectively making the function a method sort of.

Remember that `this` is only a copy, hence reassigning its members will not cause the original object to be modified.
Exception to this rule is a situation when `this` is a copy of a pointer.

Example:

```
class Null(int null);
class PtrTest(ref obj ptr, obj object, func change);

alloc obj null_ptr = Null(0);
obj null = Null(0);

obj test = PtrTest(null_ptr, null, function(void) void {
  $this.ptr.null = 123;
  $this.object.null = 123;
});

test.change();

test.ptr.null; // is 123
test.object.null; // is still 0

```

## Command line arguments

Ckript supports command line arguments.

Each program has a default variable called `arguments` which is an array of strings representing passed arguments

Invoking a program like so

```
./ckript your_code.ck one two three
```

Will produce a 4 element array of strings `array("your_code.ck", "one", "two", "three") str`
These strings can be later accessed like any other array

## Variable scope

All variables are local to the function they were declared in.
Even though Ckript allows declaring functions inside functions, inner functions' variable scope is only limited to their own scope

Functions are also able to see a copy of themselves, allowing for recursion

Example:

```
func outer = function(void) void {
  int a = 5;
  func inner = function(int arg) int {
    // a is not visible here
    return arg * 2;
  };
  int res = inner(a); // the only way to pass a variable local to 'outer' to 'inner'
};

func fib = function(int n) int {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2); // the function fib is visible in this scope
};

```

Warning: class declarations are also local to functions. Treat class declarations like variable declarations.

## If statement

```
if (condition) {
  // statements
} else {
  // else branch
}
```

## While and for statements

```
while (condition) {
  // do stuff
  break; // break out of the loop
  continue; // skip one iteration
}

for (expression; condition; increment) {
  // do stuff
}
```
## Standard library

Ckript includes a small, simple standard library for most common tasks.

All functions are available globally.

```
* print(str) void - prints the string to stdout
* flush(void) void - flushes stdout
* input(void) str - reads input from stdin and returns a string
* size(arr|str) int - returns the size of the given string or array
* to_str(any) str - returns the string representation of the given value
* to_int(int|str|float|bool) int - returns the number value of the given argument
* to_double(int|str|float|bool) double - returns the float value of the given argument
* exit(int) void - exits program with given status code
* timestamp(void) int - returns a UNIX timestamp
* file_read(str) str - opens the given file path and returns the file contents
* file_write(str, str) bool - opens the given file path (arg1), writes arg2, and returns true on success, false on failure
* pow(double, double) double - returns the result of arg1 ^ arg2
* rand(int, int) int - returns a random number from range arg1 to arg2
* randf(double, double) double - returns a random float from range arg1 to arg2
* contains(str, str) bool - returns whether arg1 contains the substring arg2
* substr(str, int, int) str - returns a substring of arg1, starting from arg2 that is arg3 characters long
* the following functions all accept a double value and return a double value which is the result of the mathematical operation
* sin/sinh/cos/cosh/tan/tanh/log/log10/ceil/floor/round/exp/sqrt/abs
```