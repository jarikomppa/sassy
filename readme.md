# Sassy Audio Spreadsheet

![Logo](https://github.com/jarikomppa/sassy/blob/main/img/logo.png?raw=true)

Sassy is a virtual modular synthesizer with the interface of a spreadsheet.

The project is largely split into two parts: `eval`, which deals with evaluating the formulas in the cells, and `sassy`, the user interface. There's some overlap where eval handles things that are "pure math" whereas the interface can access data files and user interface elements.

# Eval

### eval.cpp
`eval.cpp` contains unit tests for the formula evaluator. Most of the functions are stubbed, because eval itself does not know what they do. All of the functions are called both through both interpreted and JITted code paths and the results are compared.

There's also a simple fuzzer.

### eval.h
`eval.h` contains definitions for all of the functions that can be used in the formulas. Each function has a `FUNC_` enum and a line in the `gFunc[]` array. Each line of the array is in the exact order of the enum. For every function there's the function name, parameter list, const flag, how much memory is needed relating to sample rate and how much memory is needed on top of that.

Several functions may have the same name as long as the parameter list differs. These variants will have a different `FUNC_` enum.

Parameter list consists of zero or more characters which may be `C`, `A`, `V`, `T` or `L`.

```
{ "rowof",			"V",			1,	0,	0 },
{ "columnof",		"V",			1,	0,	0 },
{ "dt",				"",				1,  0,	0 },
{ "step",			"C",			0,  0,	sizeof(double) },
{ "allpass",		"CCC",			0,  sizeof(double),	 sizeof(int) },
```

`C` is the most common argument, which is anything that eventually collapses into a numeric value. So it can be a number or it can be an equation itself.

`A` is an area, meaning that the parameter must evaluate into something like `A3:C9`. Example: `average()`

`V` is a variable, meaning that the parameter must evaluate into something like `C9`. Example: `rowof()`

`T` is text, that will not be used for math. It can be used as a filename or comment. Example: `loadwav()`

`L` is a literal number, and must not be an equation. In some cases having a parameter be an equation would be really problematic, so it's possible to force them to be just literal numbers. Example: `buffer()`

If a function is declared as const, it may get constant folded (if all its parameters are also const) and thus will not acually get called at sample rate. Functions that have state (i.e, they have allocated memory associated with them) can never be const.

Functions may need buffers to store their state. Some memory allocations are related to sample rate. For example, if a buffer is needed for echo, the length of the buffer depends on the sample rate. Other functions may need the same amount of memory regardless of the sample rate.

### eval_parse.cpp
`eval_parse.cpp` takes the input string from a cell and parses it into an array of tokens. Each token is stored as an `Op`. Each op contains opcode, which may be one of `C`, `A`, `V`, `T`, `L`, `F` or some math operator `*/+-><=()`. The opcodes are the same as with the parameter list, with the exception of the new `F` which stands for function.

### eval_impl_lex.cpp
`eval_impl_lex.cpp` takes the parsed tokens and peforms lexical analysis so we know the input can be executed. Some small transformations are done here, like converting literals to numbers (since eventually they are treated equally) and adding parentheses to function parameters.

### eval_impl_opt.cpp
`eval_impl_opt.cpp` contains a very simple and limited constant folder. If a function is constant and all its parameters are constant, the value is folded. The folder does not know how to reorder items, so things like `5+v+3` are not folded to `8+v`, whereas `5+3+v` is.

### eval_impl_postfix.cpp
`eval_impl_postfix.cpp` performs postfix transform for the operations and eliminates parentheses.

Examples of the above transforms:
```
eval:    "((1+2)*3)+1"
Parsed: ( ( L + L ) * L ) + L
Lexed:  ( ( 1.000 + 2.000 ) * 3.000 ) + 1.000
Postfix:1.000 2.000 + 3.000 * 1.000 +


eval:    "((1+2)*3)+1"
Parsed: ( ( L + L ) * L ) + L
Lexed:  ( ( 1.000 + 2.000 ) * 3.000 ) + 1.000
Opt'd:  10.000
Postfix:10.000
```

### eval_impl_compute.cpp
`eval_impl_compute.cpp` interprets the postfix-form operations and returns the resulting value. It's basically one huge switch-case function.

### eval_impl_jit.cpp
`eval_impl_jit.cpp` takes the postfix-form operations and generates x64 code using Xbyak. Functions are generally not inlined, but called. The virtual stack from `eval_impl_compute.cpp` turns into actual stack, with `xmm0` containing the topmost item of the virtual stack.

All of the cells are jit:ted into a single code blob to be executed once. Even though the level of optimization while jit:ing is minimal, the performance difference is massive, just by getting rid of the repeated switch-case evaluation.

# Adding new functions

The process of adding new functions to sassy - one of the most fun things to do in sassy code base - goes something like this:

1. Add new enum, function definition and prototype (`eval.h`)
2. Add stub and tests (`eval.cpp`)
3. Find existing function with similar fingerprint (i.e, same number of parameters) and duplicate its code in `eval_impl_compute.cpp`, `eval_impl_jit.cpp` and, if the function is const, also in `eval_impl_opt.cpp`
4. Implement actual function in `sassy_func.cpp`, or `eval_impl_func.cpp` if const.
5. Write help text in `sassy_help.cpp`

# Interface

### sassy.h
`sassy.h` contains todo list, structure definitions and prototypes for global data. All globals are prefixed with `g`, like `gSamplerate`. Alternative for globals would be to pass around some structure or (*shiver*) using a singleton, so deal with it.

### sassy.cpp
`sassy.cpp` contains the main user interface logic plus a bunch of miscellaneous stuff and is a bit of a dumpster. Some of the stuff should be split to a separate file just to make things cleaner. Heck, `main()` itself is about a thousand lines, which could use some cleanup..

### sassy_about.cpp
`sassy_about.cpp` has the about dialog implementation.

### sassy_asio.cpp
`sassy_asio.cpp` has stubs for the ASIO interface. The code that was written towards ASIO support was removed because ASIO isn't open source friendly. It wasn't ready in any case, so no big loss here.

### sassy_config.cpp
`sassy_config.cpp` hosts the config dialog implementation as well as config file i/o.

### sassy_contextmenu.cpp
`sassy_contextmenu.cpp` deals with the right-click menu.

### sassy_data.cpp
`sassy_data.cpp` has the implementation of global data.

### sassy_func.cpp
`sassy_func.cpp` stores the function implementations for functions in the equations. Except the const ones, that are in `eval_func.cpp`.

### sassy_gear_launchpad.cpp
`sassy_gear_launchpad.cpp` has the beginnings of novation launchpad support code. The idea in long term was to support different kinds of gear, but this didn't get too far.

### sassy_help.cpp
`sassy_help.cpp` is where the help dialog can be found. Should probably be changed to be data file based..

### sassy_keyboard.cpp
`sassy_keyboard.cpp` has the virtual MIDI keyboard implementation.

### sassy_kludge.cpp
`sassy_kludge.cpp` has some kludges over Dear Imgui's text editor to enable syntax hilighting.

### sassy_midi.cpp
`sassy_midi.cpp` has all the MIDI i/o code, including widgets.

### sassy_resource.cpp
`sassy_resource.cpp` has all external resource related code, such as wav and image loading.

### sassy_scope.cpp
`sassy_scope.cpp` has the virtual oscilloscope implementation.

### sassy_sid.cpp
`sassy_sid.cpp` has c64 SID related stuff, because they're so huge that dropping them into `sassy_func.cpp` would have been stupid.

### sassy_smf.cpp
`sassy_smf.cpp` has the MIDI file player.

### sassy_uibar.cpp
`sassy_uibar.cpp` has some custom IMGUI widgets.


