VSD
===

Prints debugging messages of applications to console and supports logging of their output. VSD also works for Gui applications, it also attaches itself as a debugger which makes it possible to print the windows debug messages, which is otherwise only possible using GDB or DebugView. But GDB makes execution slow, and DebugView prints the debug output of all running applications. The possibility to attach to all sub-processes makes it interesting for debugging background processes started by the application.

It aims to be fast and simple.

Help
==
    
	Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]
	Options:
	--vsd-log logFile Write a log in colored html to logFile
	--vsd-logplain logFile Write a log to logFile
	--vsd-all Debug also all processes created by TARGET_APPLICATION
	--vsd-nc Monochrome output
	--vsd-benchmark #iterations VSD won't print the output, a slow terminal would fake the outcome
	--help print this help
	--version print version and copyright information

Example
==
![](http://winkde.org/~pvonreth/other/vsd/vsd3.png)

The logfile for this [example](http://winkde.org/~pvonreth/other/vsd/dbg.html "example")

Colors
==
- Blue
	- vsd informations.

- Green
	- Debug output

- Red
	- stderr, and some critical vsd messages

- White
	- stdout

