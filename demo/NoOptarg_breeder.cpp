// NoOptarg_breeder.cpp : Defines the entry point for the console application.
//
#include "targetver.h"

#include <cstdio>
#include <tchar.h>
#include <windows.h>
#include <iostream> // cout
#include <sstream>

#include "no_optargs_plusplus.hpp"
#include "clock_time.h"

// /SUBSYSTEM:CONSOLE
// /ENTRY:"wmainCRTStartup"


int _tmain( int argc, const TCHAR* argv[] )
{
  try{
# if 0
  int CodePages[] = {
     0,
     437/*OEM United States*/,
     850/*OEM Multilingual Latin 1; Western Europe*/,
     1250/*ANSI Central Europe*/,
     1251/*ANSI cyrillic*/,
     1252/*ANSI Latin 1; Western Europe*/,
     1253/*greek*/
   };

  for( int loop=0; loop<(sizeof(CodePages)/sizeof(CodePages[0])); loop++ )
  {
    if( CodePages[loop] )
    {
      SetConsoleOutputCP( CodePages[loop] );
      printf( "Console CP=%d\n", ::GetConsoleOutputCP() );
    }
    else 
    {
      printf( "Console CP=%d (default)\n", ::GetConsoleOutputCP() );
    }

    int idx=0;
    while( idx < argc )
    {
      size_t len = _tcslen( argv[idx] );
      _tprintf( _T("%02d, len=%d, \"%s\"\n"), idx, len, argv[idx++] );
    }

  };
//# else
  SetConsoleOutputCP( 0 );
  std::wcout << L" 1) std::wcout - test" << std::endl;
  std::cout  <<  " 2) std::cout - Ascii" << std::endl;
  SetConsoleOutputCP( 1253 ); // greek
  std::wcout << L" 3) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  std::cout  <<  " 4) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  SetConsoleOutputCP( 1251 ); // cyrill
  std::wcout << L" 5) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  std::cout  <<  " 6) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  SetConsoleOutputCP( 1250 ); // Middle Europe
  std::wcout << L" 7) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  std::cout  <<  " 8) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  SetConsoleOutputCP( 1252 ); // Latin-1 west europe
  std::wcout << L" 9) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  std::cout  <<  "10) Ελληνικά, Español, Русский, Pójdźże, kiń tę chmurność w głąb flaszy! KnödelMädelsÜberschuß" << std::endl;
  std::cout  << std::endl;
# endif

SetConsoleOutputCP( 1252 ); // Latin-1 west europe

using std::vector;
using std::string;
using std::wstring;
using namespace No;

#if 0
vector<string> SList = get_folder_contentA( "..\\..\\", true, "*.cpp" );
for( auto n : SList )
{
  puts( string( string( "file= '" ) + n + "'" ).c_str() );
}
puts( "-----------------------------" );

// Shell-like resolving
vector<string> FilesaAndFolders_Flat = get_folder_contentA( "..\\..\\", false, "", true, true );
for( auto faf : FilesaAndFolders_Flat )
{
  puts( string( string( "both= '" ) + faf + "'" ).c_str() );
}
puts( "-----------------------------" );

vector<wstring> WList = get_folder_contentW( L"..\\..\\", true, L"*.cpp", /*files:*/true, /*folders:*/false, /*recurse:*/true );
for( auto n : WList )
{
  _putws( wstring( wstring( L"name= '" ) + n + L"'" ).c_str() );
}
puts( "-----------------------------" );

WList = get_folder_contentW( L"..\\..", false, L"*.h*", /*files:*/true, /*folders:*/false, /*recurse:*/true );
for( auto n : WList )
{
  _putws( wstring( wstring( L"name= '" ) + n + L"'" ).c_str() );
}
puts( "-----------------------------" );

vector<wstring> WCrazy = get_folder_contentW( L"..\\..\\", false, L"*n*", /*files:*/true, /*folders:*/true, /*recurse:*/true );
for( auto n : WCrazy )
{
  _putws( wstring( wstring( L"crzy=\"" ) + n + L"'" ).c_str() );
}
#endif //0


puts( "-----------------------------" );

# define NO_HELP_AVAIL "no help avail"

  std::vector< No::Option > Liste = 
  {
     { "anton", 'a', No::Option::required_argument, "ignored", "Anton is not ignored." }
    ,{ "cesar", 'c', No::Option::optional_argument, "WAR!"   , "Cesars argument was WAR!" }
    ,{ "emil",  'e', No::Option::no_argument      , ""       , "Switch on the EMIL." }
    ,{ "fritz", 'f', No::Option::no_argument      , ""       , "Allow the FRITZ." }
    ,{ "gustl", 'g', No::Option::no_argument      , ""       , "Invite GUSTL." }
    ,{ "hugo",  'h', No::Option::no_argument      , ""       , "Do not forget HUGO." }
    ,{ "ijkl", 0x01, No::Option::optional_argument, ""       , "ijkl are four" }
    ,{ "mnop",'\x2', No::Option::optional_argument, ""       , "Mnop! sounds funny?" }
    ,{ "msg",   'm', No::Option::required_argument, "message", "message to be sent..." }
    ,{ "",      'u', No::Option::optional_argument, "No umlauts", NO_HELP_AVAIL }
    ,{ "",      'v', No::Option::optional_argument, "English", "if nothing said, it's said in English" }
    ,{ "weh",   'w', No::Option::no_argument      , ""       , NO_HELP_AVAIL }
//  ,{ "Weh",   'W', No::Option::no_argument      , ""       , "Does it clash?" } // Yes in No::Optargs::WINDOWS
    ,{ "Iks",   'x', No::Option::required_argument, "x-y"    , NO_HELP_AVAIL }
    ,{ "Yps",   'Y', No::Option::optional_argument, "42"     , "An illustrated paper." }
    ,{ "Zed",   'Z', No::Option::required_argument, "4711"   , "top-ZZ?" }
    ,{ "",      '-', No::Option::no_argument      , ""       , NO_HELP_AVAIL }
    ,{ "help",  '?', No::Option::optional_argument, ""       , "this help with no argument, else the specific help" }
    ,{ nullptr,0x00, No::Option::no_argument      , nullptr  , " Sample: $(Progname) --This -i s -t\"he\" -general \\help\\ful text.*" }
  };

  using Style_t = std::pair<std::string, No::Optargs::style_t>;
  std::vector<Style_t> Variants 
  { 
    { "Legacy Unix" , No::Optargs::OLDUNIX },
    { "POSIX/optarg", No::Optargs::POSIX },
    { "GNU/longopts", No::Optargs::GNU },
    { "Windows/MS"  , No::Optargs::WINDOWS },
    { "Win+Gnu-Mix" , No::Optargs::GNUWINMIX },
  };

  for( auto variant : Variants )
  {
    // -a b -cd /e /f/g/h --ijkl +4 -mnopqrßt  -u:"Üü Öö Ää" -v="Ελληνικά, Español, Русский,  KnödelMädelÜberschuß" -wxyz /W/X/Y/Z -- '*.c' "*.h"  *.*
    No::Optargs args( argc, argv, Liste );

    printf("\n\n=== verify %s ===\n=>", variant.first.c_str() );
    for( int i=1; i<argc; i++)
    { _tprintf( _T("%s "), argv[i] );
    };

    string failed_key;
    if( ! args.parse( failed_key, variant.second ) )
    {
      args.call_help( std::cerr, failed_key );
    }

    printf("*** summarize %s ***\n", variant.first.c_str() );
    args.listOptions( std::cout, "\n" );
    printf("--- probing %s ---\n", variant.first.c_str() );

    if( args.hasOption( "help" ) )
    {
      args.call_help( std::cerr, args.getOptionStr( "help" ) );
    }


    for( char opt='a'; opt<='z' ; opt++ )
    {
      if( args.hasOption( opt ) )
      {
        int cnt = args.getOptionCnt( opt );
        long num= args.getOptionInt( opt );
        printf( "has '%c' %d times => \"%s\", as integer:%ld\n", opt, cnt, args.getOptionStr( opt ).c_str(), num );
      }
    }

    std::vector<std::string> longopts = 
    { "anton", "cesar", "cd", "emil", "fritz", "gustl", "hugo",
      "ijkl", "-ijkl", "--ijkl", "mnop", "msg", "weh", "Weh", 
      "Iks", "Yps", "Zed", 
      "help", "+", "-", "--",
      "", "0x00"
    };
    for( auto opt : longopts )
    {
      if( args.hasOption( opt ) )
      {
        int cnt = args.getOptionCnt( opt );
        long num= args.getOptionInt( opt );
        printf( "has \"%s\" %d times => \"%s\", as integer:%ld\n", opt.c_str(), cnt, args.getOptionStr( opt ).c_str(), num );
      }
    }
  } // for( auto variant : Variants )

  #if 0
  struct timespec res_rt, res_cpu;
  if( 0==clock_getres( CLOCK_REALTIME, &res_rt ) && 0==clock_getres( CLOCK_PROCESS_CPUTIME_ID, &res_cpu ) )
  {
    printf( "CLOCK_RESOLUTION rtc=%f Hz, cpu=%f Hz\n", 1000000000.0/res_rt.tv_nsec, 1000000000.0/res_cpu.tv_nsec );
    struct timespec t_rtc{0}, t_cpu{0};
    int countdown = 15;
    while( countdown-->0 && 0==clock_gettime( CLOCK_REALTIME, &t_rtc ) && 0==clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &t_cpu ) )
    {
      printf( "CLOCK tick rtc=%llu,%9lu, cpu=%llu,%09lu\n", t_rtc.tv_sec,t_rtc.tv_nsec, t_cpu.tv_sec, t_cpu.tv_nsec );
      Sleep(1050);
    }
  }
  #endif // 0

  }
  catch( const std::exception& e )
  {
    std::cerr << "Exception: " << e.what() << std::endl;
    abort();
  }
  catch( ... )
  {
    std::cerr << "Exception found" << std::endl;
    abort();
  }
  return 0;
}

