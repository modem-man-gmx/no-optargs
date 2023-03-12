#ifndef NO_OPTARGS_PLUSPLUS
#define NO_OPTARGS_PLUSPLUS

#include <string>
#include <vector>
#include <map>
#include <stdexcept>


// ToDo: evaluate https://www.codeproject.com/Tips/5261900/Cplusplus-Lightweight-Parsing-Command-Line-Argumen

#if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
// pure Windows
#  define NOOAPP_DEFAULT_STYLE (No::Optargs::WINDOWS) // or better GNUWINMIX? No, let the user provide it if he wants out-of-usual
#else
// unixoids and MSYS/MinGW runtimes with real (ba)sh shell
#  define NOOAPP_DEFAULT_STYLE (No::Optargs::GNU)
#endif



namespace No {


#if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
  std::vector<std::string > get_folder_contentA( const char*    base_folder= ".\\", bool prepend=false, const char*    mask= "*.*", bool bFiles=true, bool bFolders=false, bool recursive=false );
  std::vector<std::wstring> get_folder_contentW( const wchar_t* base_folder=L".\\", bool prepend=false, const wchar_t* mask=L"*.*", bool bFiles=true, bool bFolders=false, bool recursive=false );
  #ifdef _UNICODE
  #  define get_folder_contentT get_folder_contentW
  #else
  #  define get_folder_contentT get_folder_contentA
  #endif
#endif



class Option
{
public:
  typedef enum { no_argument=0, required_argument, optional_argument } argtype;

  Option( const char* long_keyname, const char short_key, argtype canhave_arg=no_argument, const char* default_val=nullptr, const char* helptext=nullptr )
    : m_long_keyname( long_keyname ? long_keyname : "" )
    , m_short_keyname( short_key )
    , m_have_arg( canhave_arg )
    , m_value( default_val ? default_val : "" )
    , m_helptext( helptext ? helptext : "" )
  {};

  Option( const char* long_keyname, const char short_key, const char* value=nullptr )
    : Option( long_keyname, short_key, ((value && *value) ? required_argument : no_argument), value, nullptr )  // here we misuse "required_argument" to say "I have an argument assorted here"
  {};

  Option() = delete;
  ~Option(){};
  
  friend class Optargs;

private:
  std::string m_long_keyname; // i.e. "log-name" for --log-name
  const char  m_short_keyname; // i.e. 'L' for -L, or '\x200' for --log-name has no short key 
  argtype     m_have_arg;
  std::string m_value;
  std::string m_helptext;
};



struct OptargFailure : public std::runtime_error { OptargFailure()  : std::runtime_error( "ERROR: internal error"    ) {}; OptargFailure(const std::string& what)  : std::runtime_error(what) {}; };
struct OptargConvertF: public std::runtime_error { OptargConvertF() : std::runtime_error( "ERROR: conversion failed" ) {}; OptargConvertF(const std::string& what) : std::runtime_error(what) {}; };
struct OptargDefined : public std::runtime_error { OptargDefined()  : std::runtime_error( "ERROR: default conflict"  ) {}; OptargDefined(const std::string& what)  : std::runtime_error(what) {}; };
struct OptargBadInput: public std::runtime_error { OptargBadInput() : std::runtime_error( "ERROR: invalid arguments" ) {}; OptargBadInput(const std::string& what) : std::runtime_error(what) {}; };
struct OptargNoParsed: public std::runtime_error { OptargNoParsed() : std::runtime_error( "ERROR: must call parse()" ) {}; OptargNoParsed(const std::string& what) : std::runtime_error(what) {}; };



class Optargs
{
public:
  typedef enum style_e { GNU=0, POSIX, OLDUNIX, WINDOWS, GNUWINMIX, unchanged/*keep last set*/ } style_t;  // oldunix is f.i. the format of linux "find . -type f -iname '*.cpp'", longopts with single hyphens
  //typedef struct {bool needArg; bool canArg;} value_t;

  Optargs( int argc, char* argv[], const std::vector<No::Option>& Liste, No::Optargs::style_t style=NOOAPP_DEFAULT_STYLE );
  ~Optargs();

# if defined( __WIN32__ ) ||  defined( WIN32 )
  Optargs( int argc, wchar_t* argv[], const std::vector<No::Option>& Liste, No::Optargs::style_t style=NOOAPP_DEFAULT_STYLE );
# endif //defined( __WIN32__ ) ||  defined( WIN32 )

  const std::string getOption( const std::string& option ) const;
  const std::string getOption( const unsigned char opt_char ) const;
  bool hasOption( const std::string& option ) const;
  bool hasOption( const unsigned char opt_char ) const;
  void call_help( std::ostream& report_stream, const std::string& param = std::string() ) const;
  void listOptions( std::ostream& report_stream, const std::string& delimitter = std::string(", ") ) const;
  bool parse( std::string& last_option = std::string(), No::Optargs::style_t style = No::Optargs::unchanged );

private:
  std::string make_next_access_key( void );
  bool append_definition( const Option& opdef );
  bool uppercase_definition( void );
  bool append_option( const std::string& long_keyname, const std::string& value );
  bool append_value( const std::string& value );

  bool argument_requirement( const std::string& clean_key, Option::argtype& required ) const;
  bool seek_option( const std::string& option, bool get_value=false, std::string& result=std::string() ) const;
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
};

} // namespace No

#endif //define NO_OPTARGS_PLUSPLUS
