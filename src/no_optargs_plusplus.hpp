#ifndef NO_OPTARGS_PLUSPLUS
#define NO_OPTARGS_PLUSPLUS

#include <tchar.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>


#if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ ) // pure Windows
#  define NOOAPP_DEFAULT_STYLE (No::Optargs::WINDOWS) // or better GNUWINMIX? No, let the user provide it if he wants out-of-usual
#else // unixoids and MSYS/MinGW runtimes with real (ba)sh shell
#  define NOOAPP_DEFAULT_STYLE (No::Optargs::GNU)
#endif


namespace std
{
# if defined(UNICODE) || defined(_UNICODE)
  typedef wstring tstring;
  extern wostream& tcout;
  extern wostream& tcerr;
# else
  typedef string tstring;
  extern ostream& tcout;
  extern ostream& tcerr;
# endif // UNICODE
}


namespace No {


#if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
  std::vector<std::string > get_folder_contentA( const char*    base_folder= ".\\", bool prepend=false, const char*    mask= "*.*", bool bFiles=true, bool bFolders=false, bool recursive=false );
  std::vector<std::wstring> get_folder_contentW( const wchar_t* base_folder=L".\\", bool prepend=false, const wchar_t* mask=L"*.*", bool bFiles=true, bool bFolders=false, bool recursive=false );
  #ifdef _UNICODE
  #  define get_folder_contentT get_folder_contentW
  #else
  #  define get_folder_contentT get_folder_contentA
  #endif
#else
  //not needed on unices, because the shell is doing the job
#endif



class Option // always Ascii, because nobody would expect command line options and default to be non-latin
{
public:
  typedef enum { no_argument=0, required_argument, optional_argument } argtype;

  Option( const char* long_keyname, const char short_key, argtype canhave_arg, const char* default_val=nullptr, const char* helptext=nullptr )
    : m_long_keyname( long_keyname ? long_keyname : "" )
    , m_short_keyname( short_key )
    , m_have_arg( canhave_arg )
    , m_helptext( helptext ? helptext : "" )
    , m_count(0)
    , m_multi{ ((default_val) ? default_val : "") }
  {};

  Option( const char* long_keyname, const char short_key, const char* value=nullptr )
    : Option( long_keyname, short_key, ((value && *value) ? required_argument : no_argument), value )  // here we misuse "required_argument" to say "I have an argument assorted here"
  {};

  Option()
    : Option( "", '\0' )
  {};

  ~Option(){};

  friend class Optargs;

private:
  std::string m_long_keyname; // i.e. "log-name" for --log-name
  char        m_short_keyname; // i.e. 'L' for -L, or '\x200' for --log-name has no short key 
  argtype     m_have_arg;
  std::string m_helptext;
  int         m_count;
  std::vector<std::string> m_multi;
};



struct OptargFailure : public std::runtime_error { OptargFailure()  : std::runtime_error( "ERROR: internal error"    ) {}; OptargFailure(const std::string& what)  : std::runtime_error(what) {}; };
struct OptargConvertF: public std::runtime_error { OptargConvertF() : std::runtime_error( "ERROR: conversion failed" ) {}; OptargConvertF(const std::string& what) : std::runtime_error(what) {}; };
struct OptargDefined : public std::runtime_error { OptargDefined()  : std::runtime_error( "ERROR: default conflict"  ) {}; OptargDefined(const std::string& what)  : std::runtime_error(what) {}; };
struct OptargBadInput: public std::runtime_error { OptargBadInput() : std::runtime_error( "ERROR: invalid arguments" ) {}; OptargBadInput(const std::string& what) : std::runtime_error(what) {}; };
struct OptargNoParsed: public std::runtime_error { OptargNoParsed() : std::runtime_error( "ERROR: must call parse()" ) {}; OptargNoParsed(const std::string& what) : std::runtime_error(what) {}; };



class Optargs // pure UTF8 on unices, bit mixed UCS2(wchar_t), UTF an windows
{
public:
  typedef enum style_e { GNU=0, POSIX, OLDUNIX, WINDOWS, GNUWINMIX, unchanged/*keep last set*/ } style_t;  // oldunix is f.i. the format of linux "find . -type f -iname '*.cpp'", longopts with single hyphens

  # if defined( __WIN32__ ) || defined( WIN32 )
  Optargs( int argc, const wchar_t* argv[], const std::vector<No::Option>& Liste, No::Optargs::style_t style=NOOAPP_DEFAULT_STYLE );
  # endif //defined( __WIN32__ ) || defined( WIN32 )

  Optargs( int argc, const char* argv[], const std::vector<No::Option>& Liste, No::Optargs::style_t style=NOOAPP_DEFAULT_STYLE );
  Optargs() {};
  ~Optargs();

  const std::vector<std::string> getOptionValues( const std::string& option ) const; // get all (string) values for option, like --file 11 --file aa -f bb => {{11}, {aa}, {bb}}
  const std::string getOptionStr( const std::string& option, int index=1 ) const; // get one (string) value for option, 1st is default (1-based index). like --file 11 --file aa -f bb => {11}
  signed long getOptionInt( const std::string& option, int index=1 ) const; // get one (integer) value for option, 1st is default (1-based index). like --num 11 --num 22 -n 3 => {11}
  int  getOptionCnt( const std::string& option ) const;  // get count of option(s), like --verbose --verbose -vvv => 5
  bool hasOption( const std::string& option ) const; // just tell if option exist, regardless if bool, multiple, string, multiple strings ,...

  inline const std::string getOptionStr( const unsigned char opt_char, int index=1 ) const {return getOptionStr(std::string(1,opt_char),index );}; // get one (string) value for single char option, 1st is default (1-based index). like --file 11 --file aa -f bb => {11}
  inline signed long getOptionInt( const unsigned char opt_char, int index=1 ) const {return getOptionInt(std::string(1,opt_char),index );};  // get one (integer) value for single char option, 1st is default (1-based index). like --num 11 --num 22 -n 3 => {11}
  inline int  getOptionCnt( const unsigned char opt_char ) const {return getOptionCnt(std::string(1,opt_char));}; // get count of for single char option(s), like -v -v -vvv => 5
  inline bool hasOption( const unsigned char opt_char ) const {return hasOption(std::string(1,opt_char));}; // tell for single char key if option exist, regardless if bool, multiple, string, multiple strings ,...

  void call_help( std::ostream& report_stream, const std::string& param = std::string() ) const; // invoke the help for a certain parameter - or for all together
  void listOptions( std::ostream& report_stream, const std::string& delimitter = std::string(", ") ) const; // intended for debug or summarizing
  bool parse( std::string& last_option = std::string(), No::Optargs::style_t style = No::Optargs::unchanged ); // call the parser ... REQUIRED after Construction
  bool parse( No::Optargs::style_t style = No::Optargs::unchanged ) { return parse( std::string(), style );};

private:
  std::string make_next_access_key( void );
  bool append_definition( const Option& opdef );
  bool uppercase_definition( void );
  bool append_option( const std::string& long_keyname, const std::string& value );
  bool append_value( const std::string& value );

  bool get_option_definition( const std::string &clean_key, Option& opt ) const;
  bool argument_requirement( const std::string& clean_key, Option::argtype& required ) const;
  bool seek_option( const std::string& option, bool get_value=false, int* pCount=nullptr, std::string& result=std::string() ) const;
  std::string get_key( const std::string& option, std::string& split_off_keys=std::string(/*n.u.*/), std::string& split_off_values=std::string(/*n.u.*/) ) const;
  std::string seek_key( const std::string& key ) const;
  bool know_key( const std::string& key ) const;
  bool submatch_key( const std::string& key, std::string& full_key ) const;
  std::pair<std::string, std::string> get_Names( const std::string& access_key ) const;

private:
  std::map< std::string, Option > m_requirements; // hold every DEFINED / ALLOWED value with its short and long names, if they need an argument, optional help text, a.s.o.
  std::map< std::string, Option > m_valuemap; // hold every parsed/set value, accessible by a short key (if no short key exist, one artifical short will be made for the long key)
  std::map< std::string, std::string > m_access_by_name_map;  // holds synthetic access key for m_valuemap. access key can be assorted to up to 2 map keys (short option and long option name)
  std::vector< std::string > m_argv; // raw arguments, not parsed so far.
  std::vector< std::string > m_noopts; // hold everything which is not an argument's value, but a standalone value
  style_t m_style; // style of command line options of this whole instance
  unsigned int m_autonr; // internal counter for auto generated short-keys
  bool m_parsed;
  std::string m_Progname;
};

} // namespace No

#endif //define NO_OPTARGS_PLUSPLUS
