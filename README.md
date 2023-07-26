# VSD

Prints debugging messages of applications to console and supports logging of their output. VSD also works for Gui applications, it also attaches itself as a debugger which makes it possible to print the windows debug messages, which is otherwise only possible using GDB or DebugView. But GDB makes execution slow, and DebugView prints the debug output of all running applications. The possibility to attach to all sub-processes makes it interesting for debugging background processes started by the application.

It aims to be fast and simple.

### Help

```
Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]
Options:
--vsd-seperate-error             seperate stderr and stdout to identify sterr messages
--vsd-log logFile                write the logFile in colored html
--vsd-logplain logFile           write a log to logFile
--vsd-all                        debug also all processes created by TARGET_APPLICATION
--vsd-debug-dll                  Debugg dll loading
--vsd-log-dll                    Log dll loading
--vsd-nc                         monochrome output
--vsd-no-console                 don't log to console
--vsd-benchmark #iterations      VSD won't print the output, a slow terminal would fake the outcome
--help                           print this help
--version                        print version and copyright information
```

### Debug dll loading
`--vsd-debug-dll` can be used to debug a missing dll of a dynamically loaded module.

#### Example 
```
kstars(9496): 2518:8c04 @ 00508890 - LdrpProcessWork - ERROR: Unable to load DLL: "brotlidec.dll", Parent Module: "C:\Users\hanna\Downloads\kstars\bin\freetype.dll", Status: 0xc0000135
kstars(9496): 2518:8760 @ 00508906 - LdrpLoadDllInternal - RETURN: Status: 0xc0000135
kstars(9496): 2518:8760 @ 00508906 - LdrLoadDll - RETURN: Status: 0xc0000135
kstars(9496): qt.qpa.plugin: Could not load the Qt platform plugin "windows" in "" even though it was found.
