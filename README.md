# Pebble

Pebble is an open source interpreted scripting language designed to explore how syntax and programming primitives can be reimagined in a more familiar, human friendly way. Our mission is to create a language that is easy to pick up, straightforward to learn, and expressive enough to be impactful. Achieving these goals involves finding innovative solutions to reduce syntax constraints and designing novel ways to think about coding primitives.

## Motivation

As we move forward into this modern era, we are touched by technology in ways that are profound and unnoticed. Code governs our everyday. But we often take that code for granted, and we treat it as a black box.

Technology has the power to help solve the world's toughest and oldest problems. But to harness its creative power first requires an broad knowledge of how it works. And this is a responsibility which rests on everyone's shoulders. It should not be enough to let the "experts" do the work and simply trust their results; that technology is so powerful and personal means that it *must* be held accountable. It means that each citizen must understand enough about it to know when it isn't working for society. And while citizens need not be *authors* themselves, they must be *literate*.

We're developing Pebble because we believe that knowledge of coding is vital to be informed citizens and well-rounded individuals. Our goal is to create a language that anyone can pick up, understand, and master. It is our hope that in the process, newcomers will develop a stronger understanding of how code works and a more nuanced understanding of how coding can impact our society. 


## Using Pebble
Pebble is an interpreted language built over C++. To get started with Pebble, you'll need a `make` utility (either the standard Linux `make` or [mingw32-make](https://sourceforge.net/p/mingw-w64/wiki2/Make/) on Windows). Additionally, you will need a C++ compiler. We use `g++` as our standard.

Steps to install:
 * Clone the Pebble repo and run your `make` command in the repo root directory. This should produce a new executable `pebble.exe`
   ```
   $ make
   ```
 * In this same directory create a file titled `program.pebl` and write any Pebble code you would like to run in this file.
   ```python
   # program.pebl
   print "hello world"
   ```
 * Run your program by opening a terminal and running the command `./pebble.exe`
   ```
   $ ./pebble.exe
   ```

## Running the unit tests
Follow the following steps after you have installed Pebble to create the test build of the Pebble interpreter and run the unit tests
```
$ make TestBuilder
$ make pebble_testbuild.exe
$ ./pebble_testbuild.exe --byte-code --ignore-custom
```


# Language Specification
Currently it uses an experimental syntax (codenamed Boulder). The syntax is still in early beta and is subject to rapid change. These changes may not be reflected in this documentation. We will update this section as often as possible.

## Overview of Ideas
Pebble uses three core primitives which can be translated into different narratives/coding-paradigms. These three concepts are **Calls**, **Scopes** and **Sections**

### Calls
A call is the name given to a variable. For instance, in the statement `X = 4`, `X` is a call for `4`, whatever that is. Currently calls are untyped and Pebble is a dynamically typed language, but whether this is a central design feature is still under discussion. 

By legacy convention, calls must begin with an uppercase letter and must only consist of alphanumeric characters and underscores.

The same “thing” can have multiple calls. 
```python
X = 4
Y = 4
```
Whatever `4` is has the call `X` and the call `Y`. 

### Scope
Conceptually speaking, Scope is used in roughly the same was as in C++ and other traditional languages. It represents all calls which are defined at a given point in the code. Specifically, scope is determined by the indent level of a statement.
```python
if True:
    X = 4
print X
```
In the example above, `Nothing` will be printed because `X` is defined in the scope of the `if` statement (i.e. it is defined on a higher indent level) and thus not visible in the scope where `print X` is called

Similar to other langauges, "Methods" also have their own local scope which is separate from the scope where a Method is called. When statements are defined at indent level 0, however, this is considered to be **program scope** and all calls defined in this scope are accessible everywhere.

### Sections
A section is a block of imperative instructions which are executable. These are the actual lines of code written in Pebble, and are grouped by indent level. For example
```python
X = 4
print X
    Y = 4
    print Y
```
contains two sections of code. 

## Call Binding (under construction)
Calls can be associated with a scope, a section, or both. The act of association is defined as call binding. (Type binding is currently in the design phase and will be introduced fully to effect a type system) This can be done by the following operators

### Transfer binding operator
The `=` symbol denotes the transfer binding operator. It has the usage `<Call1> = <Call2>` and transfers whatever bindings are associated with `<Call2>` to `<Call1>`. 

### Section binding operator
The `:` symbol binds a section to call. It has the usage `<Call>:` and it binds the next section to `<Call>`. Note that `<Call>` may be an anonymous call

### Scope binding operator
The `()` symbol binds an new scope with optional parameters. It has the usage `<Call>()`, `<Call>(args)` where `(args)`. Note that `<Call>` may be an anonymous call. 

### Type binding operator
The `a` symbol defines a new type. It has the usage `a <Call>` and it binds the type with name `<Call>` to the scope bound to `<Call>`

## Narratives
Using these three core primitives, different narratives can be constructed that support either imperative, object oriented, or functional (WIP) paradigms. Understanding how to construct narratives first requires a general understanding of call binding and its three associated operators, the `a` (typebinding), `:` (sectionbinding), and `()` (scopebinding).

```python
# evaluates the bound section of Call
Call()

# does not evaluate the bound section of Call because rebinding is concurrently specified
# instead rebinds the section of Call to next section (indented portion)
Call():
    print "Called"

```

### Object Oriented Pebble
Classes can be considered to be typed scopes bound to a section

```python
a Rock:
    Color
    Weight

```

Instantiating an instance of a class is equivalent to evaluating the bound section of a typed scope
```python
MyRock = Rock()
```

Methods are scopes bound to a section. They can be invoked by the `()` (sectioneval) operator
```python
# binding a scope and section, section is not evaluated here
Throw():
    print "thrown"

# the scope is bound and the originally bound section evaluated
Throw()
```

The `.` (dot) operator functions as expected. An additional feature is that it preserves an additional relationship between the caller and the called "thing". This is illustrated in the following example
```python
a Rock:
    Color = "Grey"
    Weight

    Info():
        Name = "Info"
        print self.Info + ":" + caller.Color

MyRock = Rock()
MyRock.Info()
# Info:Grey
```
In this example, the keyword `caller` is a call to the "Object" to the left of the dot, which is whatever scope is calling the "Method". The `self` keyword refers to the current scope, which, inside a "Method" is the method's own scope. 

The last operator is the `here` operator. It is aliased to `inherits` to bootstrap inheritance
```python
a Rock:
    Color = "Grey"

a Pebble:
    # same as here Rock()
    inherits Rock()

MyPebble = Pebble()
print MyPebble.Color
# Grey
```
In this example, the `inherits` operator specifies that the section bound to `Rock` should be called in the current scope. Thus `Pebble` inherits the section bound to `Rock` which is exactly the section of code which defines its attributes.

## Standard operators
Pebble supports the standard suite of programming language operations with a few exceptions that have not been implemented yet (mod, bitwise operations). These also have single and multi token aliases to improve readability. The up to date list is located in `./assets/grammar.txt` under the preprocessor rules section. All documentation on that file is self contained. Reproduced below is roughly an image of that file last updated `7/24/20`. 

```
# Format:
# @ <operator>
# <token in alias1> <token in alias1> ...
# <token in alias2> ...

@ <=
is leq to
is leq
leq 
leq to
is less than or equal to

@ >=
is geq to
is geq
geq
geq to
is greater than or equal to

@ ==
equals
is equal
is equal to
is

@ !=
does not equal
not equal
not equal to
not equals
is not

@ !
not

@ a
an

@ &&
and

@ ||
or

@ here
inherits

# system call to write to std::cout
@ say
print

@ .
's

@ else
otherwise
ow
```

Additionally, to query for user input, use the `ask` keyword.
```python
ask "what day is today?"
# what day is today?
# <wait for user input>
```

## Special operators
Some operators are unique to Pebble. These are documented below

### Is
The `is` operator is an overloaded operator which exhibits conditional functionality and is similar to both `=` (assignment) and `==` (equality testing). Its usage is:
*`<Call> is <Expr>`
*`<Expr> is <Expr>`

In the first case, if `<Call> == Nothing`, i.e it is yet unbound, then `<Call>` is bound to `<Expr>` and the `is` keyword functions equivalently to the assignment operator `=`. If `<Call>` has already been bound to a scope/section, then the `is` keyword functions as the equality testing operator `==` and the statement returns a boolean truth value.

In the second case `is` functions as the `==` operator.

Example:
```python
X is 4
print X
# 4

X is 7
print X
# 4

print X is 7
# true

if X is 4
    print "X is not 7"
# X is not 7
```

### Here
The `here` operator executes a bound section without changing scope. Its usage is `here <Call>()`. The section bound to `<Call>` is executed using the local scope, instead of the scope bound to `<Call>`.

Example
```python
CreateVariable():
    X = 4
    Y = 7

here CreateVariable()
print X + Y
# 11

```