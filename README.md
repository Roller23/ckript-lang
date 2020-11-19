# Ckript language
An interpreter for my own programming language (Ckript)

If you have `g++` and `Make` installed simply running ``make`` should compile the whole project (you might need to change the Makefile a bit if you're using MinGW or clang, but it should work).
Ckript doesn't depend on any external libraries, OS-specific features, or compiler extensions, it should compile on any OS with any C++ compiler.

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
* bool
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
By default, functions do not capture outside variables. You can forward all outside variables by copy by using the `function>` syntax. Forwarding is done at invocation time, not declaration time.
```
alloc int var = 3;

func capture = function>(void) void {
  println(var); // prints 3
  var += 3; // var is a copy of a pointer, so this line will actually change the value
};

capture();
println(var); // prints 6
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

Reserving space in advance

```
// will reserve space for 10 elements
arr my_array = array() [10] int;
// works fine, but the value is undefined. Especially dangerous with 2D arrays or arrays of objects
int element = my_array[5];
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

Warning - class declarations are treated like variables. They'll not be visible inside inner functions unless captured.

## 'this' variable

If one of the object's members is a function, a special variable called `this` will be automatically pushed onto its stack.
`this` is a copy of the object that is holding the function effectively making the function a method sort of.

Remember that `this` is only a copy, hence reassigning its members will not cause the original object to be modified.
Exception to this rule is a situation when `this.member` is a copy of a pointer.

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
* print(any[, any]) void - accepts any number of arguments (at least one) of any type and prints them to stdout
* println(any) void - accepts any number of arguments, works like print() but prints a new line at the end for you
* flush(void) void - flushes stdout
* input(void) str - reads input from stdin and returns a string
* size(arr|str) int - returns the size of the given string or array
* to_str(any) str - returns the string representation of the given value
* to_int(int|str|float|bool) int - returns the number value of the given argument
* to_double(int|str|float|bool) double - returns the float value of the given argument
* exit(int) void - exits program with given status code
* timestamp(void) int - returns a UNIX timestamp
* file_exists(str) bool - returns a boolean value indicating whether the file exists or not
* file_remove(str) bool - removes the specified file, returns true on success, false on failure
* file_read(str) str - opens the given file path and returns the file contents
* file_write(str, str) bool - opens the given file path (arg1), writes arg2, and returns true on success, false on failure
* pow(double, double) double - returns the result of arg1 ^ arg2
* rand(int, int) int - returns a random number from range arg1 to arg2
* randf(double, double) double - returns a random float from range arg1 to arg2
* contains(str, str) bool - returns whether arg1 contains the substring arg2
* substr(str, int, int) str - returns a substring of arg1, starting from arg2 that is arg3 characters long
* split(str, str) arr - splits arg1 by any of the delims in arg2 and returns an array of strings
* to_bytes(str) arr - returns the bytes of arg1 as an array of ints
* from_bytes(arr) str - constructs a string from the array of bytes
* the following functions all accept a double|int value and return a double value which is the result of the mathematical operation
* sin/sinh/cos/cosh/tan/tanh/log/log10/ceil/floor/round/exp/sqrt/abs
```