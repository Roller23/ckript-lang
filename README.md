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
./skript
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
const double number2 = 5.2132; // cannot reassign
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

All functions are lambdas (values of type func)

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
  // do something with result...
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
alloc int a = 5;
ref int a_ptr = a;

del a_ptr;

// 'a_ptr' is no longer a pointer
// 'a' points still points to a location on the heap and using it may cause undefined behavior
```

Assigning an allocated variable to a variable that is not a reference will copy the actual value

```
alloc double pi = 3.14;
double pi_copy = pi;

del pi_copy; // will fail
```

Function arguments can also take references

## Arrays

```
arr my_array1 = array() int; // empty array that holds ints
arr my_array2 = array("hello", "world") str; // array of strings with some initial values

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
$Mark.age = 31; // the $ is required
```

Objects can have member objects, object members can be references
