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

Memory allocation

Used ``alloc`` to allocate something on the heap, ``del`` to delete from the heap, ``ref`` to get the pointer of that value

```
alloc int a = 5;
ref int a_ptr = a;

del a_ptr;

// both a and a_ptr point to nothing now

```

Assigning an allocated variable to a variable that is not a reference will copy the actual value

```
alloc double pi = 3.14;
double pi_copy = pi;

del pi_copy; // will fail
```

Function arguments can also take references

Arrays

```
arr my_array1 = array() int; // empty array that holds ints
arr my_array2 = array("hello", "world") str; // array of strings with some initial values

my_array2 += "!"; // appending to the array
my_array2 = "Test" + my_array2; // prepending to the array

my_array1 ^= array(1, 2, 3) int; // array concatenation

int element = array[1]; // getting the value
#element[0] = 42; // setting a new value (# is required)

```

Objects

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

Objects can have member objects, member objects can be references
