To build atanks using Visual Studio 2013 you will need
to adapt the include path settings to where your allegro
includes are.
Further replace alleg44.dll and alleg44.lib with the
versions you want to use.

Release 32bit uses alleg44.lib          and alleg44.dll
Release 64bit uses alleg44_64.lib       and alleg44.dll
Debug   32bit uses alleg44-debug.lib    and alleg44-debug.dll
Debug   64bit uses alleg44_64-debug.lib and alleg44_64-debug.dll

As the windows build is originally not meant for debugging,
only the 32bit release versions are included in git.
You will need your own includes and libs!
