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

## if statement

```
if (condition) {
  // statements
} else {
  // else branch
}
```

## while and for statements

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
