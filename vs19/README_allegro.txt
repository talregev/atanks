To build atanks using Visual Studio 2015 you will need
to adapt the include path settings to where your allegro
includes are.
Further replace alleg44.dll and alleg44_64.dll with the
versions you want to use.

Release 32bit uses alleg44.lib      and alleg44.dll
Release 64bit uses alleg44_64.lib   and alleg44_64.dll
Debug   32bit uses alleg44_d.lib    and alleg44_d.dll
Debug   64bit uses alleg44_64_d.lib and alleg44_64_d.dll

As the windows build is originally not meant for debugging,
only the release versions are included in git.
You will need your own includes and libs!
