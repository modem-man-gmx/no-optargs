# no-optargs
portable and customizeable C++11 replacement for all optarg/optargs/longopts approaches.
* Define your command line options and parameters similar to optargs/longopts.
* Create an instance and give argc + argv
* Select your options style (Posix, Gnu-longopts, Windows, ...)
* Call the parser, access any element
* compiles with MSVC14 (Studio 2015 Community Edition)
* compiles with MSYS2/MinGW/GCC (soon)

## define:
```
std::vector< No::Option > Options {
 { "anton", 'a', No::Option::required_argument, "ignored", "Anton is not to be ignored" },
 { "cesar", 'c', No::Option::optional_argument, "WAR!"   , "Cesars argument was WAR" },
 { "emil",  'e', No::Option::no_argument      , "EEE"    , "Emil and the detectives" },
```

## Parse
```
No::Optargs args(argc, argv, Options);
if( !args.parse(No::Option::GNU))
{
    args.call_help(std::cerr);
}
```
## Use
```
if(args.hasOption("emil"))
{
    emil++;
}
std::cout << "Emperor said: " << args.getOption("cesar") << std::endl;
if(0==args.getOption("anton").compare("ignored"))
{
    args.call_help(std::cout, "anton");
}
```

# intention
* was tired of porting any of the GNU/Linux/... optargs-parser libraries, then finding it does not work with certain license
* was pi**es of of needing GNU longopt library ported to Windows and finding incompatible license
* got tired of including unreadable switch case monsters into main() to handle my options

# features
* One Class for all systems and platforms
* behaves like Linux shell with filename wildcards (*.txt -> readme.txt, notes.txt, ...) on windows
* can have default values right at definition
* can have help text for parameters right at definition

# plans
This is work on progress. If you do hate it, try another.
If you could like it but am not happy with: fork + create a branch, make improvements, start pull request from your fork's branch.
I will only accept PR from branches.

# warning
This library reflects MY interpretation of what GNU longopts is, what Posix optargs is, and so on. I might be wrong. 
Give me hints where I am missing facts.
Please: be patient, be polite.
