#include "no_optargs_plusplus.hpp"

#if defined( __WIN32__ ) ||  defined( WIN32 )
#  include <tchar.h>
#  include <windows.h>
#  include <vector>
using std::wstring;
#endif

#include <algorithm>
#include <iostream>
#include <sstream>
#include <system_error>
#include <cctype>

namespace std
{
  #if defined(UNICODE) || defined(_UNICODE)
  wostream& tcout = wcout;
  wostream& tcerr = wcerr;
  #else
  ostream& tcout = cout;
  ostream& tcerr = cerr;
  #endif // UNICODE
}

using std::string;
using std::tstring;
using std::vector;

namespace No {

#define NOOAPP_ACCESS_KEY_PFX "key@"
#define NOOAPP_INTERNAL_HELP "@@@_internal_help_@@@"
#define NOOAPP_STRINGIFY(a) #a
#if defined( __WIN32__ ) ||  defined( WIN32 )
# define NOOAPP_ANSI CP_ACP
#else
# define NOOAPP_ANSI 0
#endif

#if defined(DEBUG) || !defined(NDEBUG)
# define HANDLE_PARSER_INCOMPLETE(msg) do{ std::cerr  << ((msg)) << std::endl; } while(0)
# define HANDLE_PARSER_ERROR(msg)      do{ std::cerr  << ((msg)) << std::endl; } while(0)
# define HANDLE_PARSER_UNKNOWN(msg)    do{ std::cerr  << ((msg)) << std::endl; /*call_help( std::cerr, key );*/ } while(0) //; return false;
# define HANDLE_DEFINITION_ERROR(msg)  do{ std::cerr  << ((msg)) << std::endl; } while(0)
# define DBG_TELL(msg)                 do{ std::cerr  << ((msg)) << std::endl; } while(0)
# define DBG_TELLw(msg)                do{ std::wcerr << ((msg)) << std::endl; } while(0)

#elif defined(NOOAPP_CALM_PARSER)
# define HANDLE_PARSER_INCOMPLETE(msg) do{ std::cerr  << ((msg)) << std::endl; } while(0)
# define HANDLE_PARSER_ERROR(msg)      do{ std::cerr  << ((msg)) << std::endl; return false; } while(0)
# define HANDLE_PARSER_UNKNOWN(msg)    do{ call_help( std::cerr, key ); } while(0);
# define HANDLE_DEFINITION_ERROR(msg)  do{ std::cerr  << ((msg)) << std::endl; return false; } while(0)
# define DBG_TELL(msg)
# define DBG_TELLw(msg)

#else
# define HANDLE_PARSER_INCOMPLETE(msg) do{ throw OptargNoParsed((msg)); } while(0)
# define HANDLE_PARSER_ERROR(msg)      do{ throw OptargBadInput((msg)); } while(0)
# define HANDLE_PARSER_UNKNOWN(msg)    return false;
# define HANDLE_DEFINITION_ERROR(msg)  do{ throw OptargDefined((msg));  } while(0)
# define DBG_TELL(msg)
# define DBG_TELLw(msg)
#endif

static std::string  basename_of_execT( const std::tstring& argv0 );
static std::string  basename_of_exec( const std::string& argv0 );
static std::string  WideString2CP( const std::wstring& UCS2, unsigned int Codepage=NOOAPP_ANSI );
static std::string  WideString2UTF8( const std::wstring& UCS2 );
static std::wstring UTF8toWideString( const std::string& UTF8 );
static void         cut_string_at( const std::string& input, const std::string& separators, std::string& lhs, std::string& rhs );
static std::string  trim_quotes( const std::string& input, const std::string& separators );
typedef std::map<std::string,std::string> PlaceholderReplace;
static std::string  replace_placeholder( const std::string& input, const PlaceholderReplace& placeholders, const std::string& lead_in=std::string("$("), const std::string& lead_out=std::string(")") );
static std::string  conditional_uppercase( const std::string& anycase, No::Optargs::style_t Style );
static std::string  get_option_prefix_long( No::Optargs::style_t Style );
static std::string  get_option_prefix_short(No::Optargs::style_t Style );

#if defined(UNICODE) || defined(_UNICODE)
# define tstring_to_ascii(a) WideString2UTF8(a)
#else
# define tstring_to_ascii(a) (a)
#endif



/* === 1 = POSIX Options ===
Arguments are options if they begin with a hyphen delimiter ('-').
Multiple options may follow a hyphen delimiter in a single token if the options do not take arguments. Thus, '-abc' is equivalent to '-a -b -c'.
Option names are single alphanumeric characters.
Certain options require an argument. For example, the -o option of the ld command requires an argument—an output file name.
An option and its argument may or may not appear as separate tokens. (In other words, the whitespace separating them is optional.) Thus, -o foo and -ofoo are equivalent.
Options typically precede other non-option arguments.
The implementations of getopt and argp_parse in the GNU C Library normally make it appear as if all the option arguments were specified 
before all the non-option arguments for the purposes of parsing, even if the user of your program intermixed option and non-option arguments. 
They do this by reordering the elements of the argv array.

The argument -- terminates all options; any following arguments are treated as non-option arguments, even if they begin with a hyphen.
A token consisting of a single hyphen character is interpreted as an ordinary non-option argument. By convention, it is used to specify input from or output to the standard input and output streams.
Options may be supplied in any order, or appear multiple times. The interpretation is left up to the particular application program.

/* === 2 = GNU Options ===
GNU adds long options to the POSIX conventions. Long options consist of -- followed by a name made of alphanumeric characters and dashes. 
Option names are typically one to three words long, with hyphens to separate words. Users can abbreviate the option names as long as the abbreviations are unique.

To specify an argument for a long option, write --name=value. This syntax enables a long option to accept an argument that is itself optional.

/* === 3 = Non standard Options (find . -type f -name ...) ===
This is neither POSIX nor GNU

/* === 4 = Windows Options ===
Arguments are options if they begin with a foreslash delimiter ('/') or a sometimes alternatively with a hyphen delimiter ('-').
Multiple options may separated by whitespace or glued together if delimitted by the foreslash. Thus, 'xcopy /K/R/E/I/S/C/H' is equivalent to 'xcopy /K /R /E /I /S /C /H', but neither -k-r-e-i-s-c-h nor -krei-sch.
Option names are single alphanumeric characters OR alphanumeric words.

To specify an argument for a long option, write '/name:value' or '/name value', even sometimes seen is '/name=value'. Same for '-' delimiters, but GNU longopt syntax '--name' is more and more used here.

=== */

Optargs::Optargs( int argc, const char* argv[], const std::vector<No::Option>& Liste, No::Optargs::style_t style )
  : m_requirements()
  , m_valuemap() // hold every value accessible by a short key (if no short key exist, one artifical short will be made for the long key)
  , m_access_by_name_map()  // find shortname of longname, if none exist, one artifical short will be assigned
  , m_argv()
  , m_style(style)
  , m_autonr(0)
  , m_parsed(false)
  , m_Progname()
{
  // always ASCII or at worst UTF-8 within the list.
  for( auto od : Liste )
  {
    append_definition( od );  // set the definition of accepted values.
  }

  // always UTF-8 on Unixoid systems, no need for UCS/Wide storage
  for( int i=0; i<argc; i++ )
  {
    m_argv.push_back( argv[i] );
  }
};

#if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ ) && !defined( __MINGW32__ ) && !defined( __MINGW64__ )
Optargs::Optargs( int argc, const wchar_t* argv[], const std::vector<No::Option>& Liste, No::Optargs::style_t style )
  : m_requirements()
  , m_valuemap() // hold every value accessible by a short key (if no short key exist, one artifical short will be made for the long key)
  , m_access_by_name_map()  // find shortname of longname, if none exist, one artifical short will be assigned
  , m_argv()
  , m_style(style)
  , m_autonr(0)
  , m_parsed(false)
  , m_Progname()
{
  for( auto od : Liste )
  {
    append_definition( od );  // set the definition of accepted values.
  }

// todo: rework this to use UCS2, wchar_t, std::wstring completely if running in windows...

  for( int i=0; i < argc; ++i )
  {
    m_argv.push_back( WideString2UTF8( argv[i] ) );
  }
};
#endif //(defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ ) && !defined( __MINGW32__ ) && !defined( __MINGW64__ )



Optargs::~Optargs()
{}

// ===================================================================
// ============================ The Getters ==========================
// ===================================================================
const std::vector<std::string> Optargs::getOptionValues( const std::string& option ) const
{
  if( ! m_parsed ) throw OptargNoParsed();
  string search_key( conditional_uppercase( option, m_style ) );
  string clean_key = get_key( search_key ); // arg 2+3 are dummies here, we expect the user provided pure keys or hyphened/dashed keys.

  string access_key = seek_key( clean_key );

  auto value_itr = m_valuemap.find( access_key );
  if( value_itr != m_valuemap.end() ) // found
  {
    return value_itr->second.m_multi;
  }
  return std::vector<std::string>{};
}



const string Optargs::getOptionStr( const std::string& option, int index ) const
{
  if( ! m_parsed ) throw OptargNoParsed();
  string res("");
  int cnt=index;
  this->seek_option( option, true, &cnt, res );
  return res;
}


signed long Optargs::getOptionInt( const std::string& option, int index ) const
{
  if( ! m_parsed ) throw OptargNoParsed();
  signed long result=0;
  try
  {
    string number( getOptionStr(option, index) );
    result = stol( number );
  }
  catch(...)
  {  // intentionally ignore:
     // std::invalid_argument if no conversion could be performed
     // std::out_of_range if the converted value would fall out of the range of the result type
  }
  return result;
}


int Optargs::getOptionCnt( const std::string & option ) const
{
  if( ! m_parsed ) throw OptargNoParsed();
  int cnt=0;
  if( seek_option( option, true, &cnt ) )
  {
    return cnt;
  }
  return 0;
}


bool Optargs::hasOption( const std::string& option ) const
{
  if( ! m_parsed ) throw OptargNoParsed();
  return seek_option( option );
}



void Optargs::call_help( std::ostream& report_stream, const std::string& param ) const
{
  string key( conditional_uppercase( param, m_style ) );
  string access_key = seek_key( key );

  auto opt_itr = m_requirements.find( access_key );
  if( m_requirements.end() != opt_itr && opt_itr->second.m_helptext.length() )
  {
    report_stream << opt_itr->second.m_helptext << std::endl;
    return;
  }

  // no match on param? then generic help
  opt_itr = m_requirements.find( seek_key( NOOAPP_INTERNAL_HELP ) );
  if( m_requirements.end() != opt_itr && opt_itr->second.m_helptext.length() )
  {
    PlaceholderReplace PH={{ "Progname", this->m_Progname}};
    report_stream << replace_placeholder( opt_itr->second.m_helptext, PH, string("$("), string(")") ) << std::endl;
  }

  string lpfx( get_option_prefix_long( m_style ) );
  string cpfx( get_option_prefix_short(m_style ) );

  size_t longest=0;
  for( auto entry : m_requirements )
  {
    size_t len = entry.second.m_long_keyname.length();
    if( len + lpfx.length() > longest ) longest = len + lpfx.length();
  }
  const size_t shortlen=2;

  for( auto entry : m_requirements )
  {
    int have = 0;
    if( !entry.second.m_long_keyname.empty()  ) have++;
    if( isalnum(entry.second.m_short_keyname) ) have++;

    if( ! entry.second.m_helptext.empty() && (have>0) )
    {
      string long_switches( (entry.second.m_long_keyname.empty())    ? "" : (lpfx + entry.second.m_long_keyname) );
      string shrt_switches( (!isalnum(entry.second.m_short_keyname)) ? "" : (cpfx + string(1,entry.second.m_short_keyname)) );

      long_switches.append( longest - long_switches.length(), ' ');
      shrt_switches.append( shortlen- shrt_switches.length(), ' ');

      report_stream << " " << shrt_switches << ((have>1)?", ": "  ") <<  long_switches << " " << entry.second.m_helptext << std::endl;
    }
  }
  return;
}


// ===================================================================
// ============================ The Tools ============================
// ===================================================================
bool Optargs::parse( std::string& last_option, No::Optargs::style_t style )
{
  DBG_TELL("\n");
  if( style != No::Optargs::unchanged )
  {
    m_style = style;
  }
  uppercase_definition(); // name is misleading. not the m_Requirements map is uppercased, but the lookup-map is
  
  m_parsed = true;
  bool only_values_following=false;
  // loop ... in m_argv
  for( size_t i=0; i < m_argv.size(); i++ )
  {
    if( i==0 )
    {
      m_Progname = basename_of_exec( m_argv[0] );
      continue;
    }

//ToDo: one single "-" should mean: read [all further] arguments from stdin
//ToDo: "@filename" with false==access("@filename") && true==access("filename") should mean: read [all further] arguments from filename, at least in windows

    string key( m_argv[i] );
    string par( (i+1 < m_argv.size()) ? m_argv[i+1] : "" );
    char lead_in = key[0];
    last_option = key;

    if( (Optargs::OLDUNIX==m_style || Optargs::POSIX==m_style || Optargs::GNU==m_style ) && 0==key.compare( "--" ) )
    {
      only_values_following = true;
      continue; // skip storing also the "--" itself
    }

    if( (only_values_following) ||
        ((Optargs::OLDUNIX  ==m_style) && ('-'!=lead_in)) ||
        ((Optargs::POSIX    ==m_style) && ('-'!=lead_in)) ||
        ((Optargs::GNU      ==m_style) && ('-'!=lead_in)) ||
        ((Optargs::WINDOWS  ==m_style) && ('/'!=lead_in)) ||
        ((Optargs::GNUWINMIX==m_style) && ('/'!=lead_in) && ('-'!=lead_in))
      )
    {
#     if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
      // pure Windows is missing wildcard resolution on shell
      if( '\'' != key[0] && key.npos != key.find_first_of("?*") )
      {
        vector<string> FilesAndFolders = get_folder_contentA( ".", true, key.c_str(), true, true );
        for( auto resolved : FilesAndFolders )
        {
          append_value( resolved );
        }
      }
      else
#     endif
      {
        append_value( key );
      }
      continue; // next could be a switch again
    }


    bool subletters_done = false;
    do // while( !subletters_done )
    {
      DBG_TELL( string( "get( " ) + key + ", [" + par + "])" );
      string split_off_keys, split_off_values;
      key = get_key( key, split_off_keys, split_off_values ); // arg 2+3 receiving split off glued arguments or keys
      last_option = key;

      if( ! know_key( key ) )
      {
        HANDLE_PARSER_UNKNOWN( "ERROR: " + key + " is not known" );
      }

      Option::argtype arg_presence;
      if( argument_requirement( key, arg_presence ) )
      {
        switch( m_style )
        {
          case Optargs::OLDUNIX  :
            // can have single-letter- and word- switches, both starting with single '-', arguments. argument values are always next word and are not optional
            // Valid Variants: "-type f -a -print0" with "-print0" NOT to be read as "-print 0" and with "f" being the value for "-type"
            subletters_done = true; // oldunix has no -abc == -a -b -c
            if( arg_presence == Option::optional_argument )
            {
// ToDo: this is likely wrong! I think "-argument1 value1 -argument2 --nextargument -argument3 lastargument" could have all 3 optional, because '-nextargument' can't be missinterpreted because of '--' escaping
              HANDLE_PARSER_ERROR( "ERROR: this style (" + string(NOOAPP_STRINGIFY(Optargs::OLDUNIX)) + ") can not have optional arguments"); // perhaps wrong, could also be the rule the next "-word" ist a switch but next "word" is argument value
            }
            if( arg_presence != Option::required_argument )
            {
// ToDo: see above
              par.clear(); // next value is _not_ the argument value.
            }
            else
            {
              i++; // we already take the argv[i+1] next as parameter, skip over this
            }
            break;

          case Optargs::WINDOWS  : // Valid variants "/A /B /C/D/E /F:filename /G pathname"
          case Optargs::GNUWINMIX:
            if( '/'==lead_in )
            {
              if( (arg_presence==Option::required_argument || arg_presence==Option::optional_argument) && ! split_off_keys.empty() )
              { // windows does not allow a key with arg followed by attached next keys - 1st the arg must be there
                HANDLE_PARSER_ERROR( "ERROR: missing argument for: " + key + " blocked by " + split_off_keys );
              }
              else if( (arg_presence==Option::required_argument || arg_presence==Option::optional_argument) && ! split_off_values.empty() )
              {
                par = split_off_values;
              } // else keep next word as argument
              else if( arg_presence==Option::no_argument || (arg_presence==Option::optional_argument && split_off_values.empty()) )
              {
                par.clear(); // next value is _not_ an argument value and splitoff has also nothing.
              }
              else if( (arg_presence==Option::required_argument ) && split_off_values.empty() )
              {
                i++; // we already take the argv[i+1] next as parameter, skip over this
              }
              // subletters not handled here. are split off in getkey
              subletters_done = split_off_keys.empty(); // xcopy /K/R/E/I/S/C/H would be seen as switch "K/R/E/I/S/C/H", not as switches K,R,E,I,S,C,H, need to be split inside get_key
              break;
            } //if( '/'==key[0] )
            else if( Optargs::WINDOWS==m_style ) // pure windows? no / ? then we are done!
            {
              break;
            }
            else // mixable gnu / windows syntax wanted? no / ? then we have GNU
            {
              ; // fallthrough wanted
            }
            // fallthrough wanted
          case Optargs::GNU      :
            // Valid Variants: "--no-arg --arg=1 --arg 2 -m"Comment" -mComment -abc -d maybe -- filenames"
            // with: "no-arg" is "no-arg" or perhaps best matching sub-string of "no-argument-given"
            //       "arg" + "1" => switch + value if required or optionally
            //       "arg" + "2" => switch + value if required ???
            //       "m"Comment" -mComment => either "m" + "Comment" => switch + value (if required) or "m", "C" + "omment", or "m", "C", "o" + "mment", depends on requiring an argument
            //       "a", "b", "c" switches, or like above: "a" +"bc" if arg required for a, "a", "b" + "c", if arg required for b, or "a","b","c" if no one requires an arg.
            //       "d" + "maybe" NOT SURE for now. is "maybe" the value? because it does not start with '-' ??? or ist this constellation invalid in GNU longopts?
            //       "-- filenames" all after '--' is unassigned, perhaps unsorted values
            {
              string full_keyname(key);
              if( key.length()>1 && ! submatch_key( key, full_keyname ) ) // true if key was identical or unique substring, false if no match or too many matches (ambiguous)
              {
                if( full_keyname.empty() ) // no match
                  HANDLE_PARSER_ERROR( "ERROR: unknown argument: " + key );
                else
                  HANDLE_PARSER_ERROR( "ERROR: ambiguous argument: " + key + " could match " + full_keyname + " or some other." );
                return false;
              }
              key = full_keyname;
            } // fallthrough intentional!
          case Optargs::POSIX    :
            // Valid Variants: "-m"Comment" -mComment -abc -d maybe -- filenames"
            //       "m"Comment" -mComment => either "m" + "Comment" => switch + value (if required) or "m", "C" + "omment", or "m", "C", "o" + "mment", depends on requiring an argument
            //       "a", "b", "c" switches, or like above: "a" +"bc" if arg required for a, "a", "b" + "c", if arg required for b, or "a","b","c" if no one requires an arg.
            //       "d" + "maybe" NOT SURE for now. is "maybe" the value? because it does not start with '-' ??? or ist this constellation invalid in GNU longopts?
            //       "-- filenames" all after '--' is unassigned, perhaps unsorted values
            if( (arg_presence==Option::required_argument || arg_presence==Option::optional_argument) && ! split_off_values.empty() )
            {
              par = split_off_values;
            } // else keep next word as argument
            else if( (arg_presence==Option::required_argument ) && split_off_values.empty() )
            {
              i++; // we already take the argv[i+1] next as parameter, skip over this
            }
            else if( key.length()>1 && arg_presence==Option::optional_argument && split_off_keys.empty() && ! know_key( get_key( par ) ) )
            {
              i++; // we already take the argv[i+1] next as parameter, skip over this
            }
            else if( key.length()==1 && arg_presence==Option::optional_argument && ! split_off_keys.empty() ) // single letter keys can not have space-delimitted OPTIONAL value, but "glued" ones, if the next letter is not a key?
            {
              if( know_key( split_off_keys.substr(0,2) ) ) // glued keys are always single-letter keys and split added the '-' in front.
              {
                par.clear(); // the next is a key
              }
              else
              {
                par = split_off_keys.substr(1); // the next is no key, so it is the optional value.
                split_off_keys.clear();
              }
            }
            else if( key.length()==1 && arg_presence==Option::no_argument  )
            {
              par.clear(); // next value is _not_ an argument value and splitoff has also nothing.
            }
            subletters_done = split_off_keys.empty();
            break;

          default:
            return false;
        } // switch( m_style )
      } // if( argument_requirement( clean_key, arg_presence ) )
      else
      {
        break; // no valid key, drop it for all styles!
      }

      DBG_TELL( string( "append_option(" ) + key + ", " + par + ")" );
      bool res = append_option( key, par ); // arg2 can be empty string
      if( !res )
      {
        HANDLE_PARSER_ERROR( string( "ERROR: can not store option " + key + " with required arguments") );
      }

      key = split_off_keys;
    } while( !subletters_done );

  }; // end-of-for( size_t i=0; i < m_argv.size() ; i++ )

  return true;
}

// ===================================================================
// ============================ OPTIONAL  ============================
// ===================================================================
std::pair<std::string,std::string> Optargs::get_Names( const std::string& access_key ) const
{
  std::pair<std::string, std::string> result;
  int keysfound=0;

  for( auto entry : m_access_by_name_map )
  {
    if( access_key == entry.second )
    {
      if( entry.first.length()==1 )
      {
        result.first = entry.first; 
        keysfound++;
      }
      else
      {
        result.second = entry.first; 
        keysfound++;
      }
    if( keysfound > 1 )
      return result; // for cases with short and log option, there is nothing else to find, we're done
    // for cases with either short or log option, we do not know if this is the case, so we need to iterate to end.
    }
  }
  return result;
}


static std::string string2hex( const std::string& shortname )
{
  const char hex[]="0123456789ABCDEF";
  string hexstring("");

  for( auto ch : shortname )
  {
    hexstring.push_back( hex[ (ch     ) & 0x0F ] );
    hexstring.push_back( hex[ (ch >> 4) & 0x0F ] );
  }
  std::reverse( hexstring.begin(), hexstring.end() );
  return hexstring;
}


static std::string make_printable( const std::string& shortname )
{
  string result( shortname );

  if( ! shortname.empty() )
  {
    const string valid_printables("-_$§?!%&#~");
    for( auto ch : shortname )
    {
      if( isalnum(ch) || string::npos != valid_printables.find( ch ) )
        continue;
      return string("0x") + string2hex( shortname );
    }
  }
  return result;
}


static std::string conditional_uppercase( const std::string& anycase, No::Optargs::style_t Style )
{
  string converted( anycase );
  if( Style==No::Optargs::WINDOWS )
  {
    std::for_each( converted.begin(), converted.end(), [](char& c) // modify in-place
    {
      c = std::toupper( static_cast<unsigned char>(c) );
    });
  } // windows only
  return converted;
}



// m_valuemap; // hold every value accessible by a short key (if no short key exist, one artifical short will be made for the long key)
// m_access_by_name_map;     // find shortname of longname, if none exist, one artifical short will be assigned
// m_short2longnames;     // find longname of shortname, if one exist
// m_noopts;              // hold everything which is not an argument's value, but a standalone value
void Optargs::listOptions( std::ostream& report_stream, const std::string& delimitter ) const
{
  if( ! m_parsed ) { HANDLE_PARSER_INCOMPLETE("not parsed entirely"); }
  int num=0;
  // dump all which have '--xxxx' @ '-x'   (long option assorted to printable short option)
  //       or which have '--xxxx' @ '0xXX' (long option assorted to non-printable short option)
  //       or which have       '' @ '-x'   (only a printable short option)
  report_stream << "Options:" << delimitter;
  for( auto usedentry_itr : m_valuemap )
  {
    if( ++num>1 ) report_stream << delimitter; // separate all next entries after 1st one

    const string& access_key( usedentry_itr.first );
    std::pair<std::string,std::string> names = get_Names( access_key );
    const string& longname  = (names.first.length() >1) ? names.first : names.second;
    const string& shortname = (names.first.length()==1) ? names.first : names.second;

    string value( usedentry_itr.second.m_multi[0] ); // can be empty string if it is only a switch!
    if( usedentry_itr.second.m_count > 1 )
    {
      int num=0;
      value.assign("{");
      for( auto next : usedentry_itr.second.m_multi )
      {
        if(num>0) value.append(",{");
        value.append( next );
        value.append("}");
        num++;
      }
    }
    else if( usedentry_itr.second.m_count == 0 && usedentry_itr.second.m_have_arg != Option::no_argument )
    {
      Option optdef;
      if( get_option_definition( longname.empty() ? shortname : longname, optdef ) )
      {
        value.assign( "default (" );
        value.assign( optdef.m_multi[0] );
        value.assign( ")" );
      }
    }


    string printable_short( make_printable( shortname ) );

    if( ! longname.empty() && ! printable_short.empty() )
    {
      report_stream << "[" << printable_short << "]," << longname << "=" << value;  // tell both keys
    }
    else if( ! longname.empty() )
    {
      report_stream << "[ ]"  << longname << "=" << value;  // tell long key
    }
    else if( ! printable_short.empty() )
    {
      report_stream << "[" << printable_short << "]=" << value;  // tell short key
    }
    else
    {
      throw OptargFailure("listOptions() found dead entry.");
    }
  } //for( auto shortname_itr : m_valuemap )

  // dump all the values not assorted to an option (GNUOPTS: after the "--")
  if( ++num>1 ) report_stream << delimitter;
  report_stream << "Values:";

  //report_stream << m_noopts.size() << " values,";
  size_t nr=0;
  for( auto value : m_noopts )
  {
    if( ++num>1 ) report_stream << delimitter;
    report_stream << "[" << nr++ << "] " << value;
  }
  //report_stream << ". END.";
  report_stream << std::endl;
  return;
}



// ===================================================================
// ============================  PRIVATE  ============================
// ===================================================================
std::string Optargs::make_next_access_key(void)
{
  std::string access_key( NOOAPP_ACCESS_KEY_PFX );
  access_key.append( std::to_string(++m_autonr) );
  return access_key;
}



// arg1 holds a cleaned key (no dash, slash, hyphen, inline arguments.
// arg2 returns if parameter con/must/must not follow.
bool Optargs::get_option_definition( const std::string& clean_key, Option& opt ) const
{
  string access_key = seek_key( conditional_uppercase( clean_key, m_style ) );

// ToDo: valuemap value is not a string but a vector of strings, to hold multiple same switches data
  auto require_itr = m_requirements.find( access_key );
  if( m_requirements.end() != require_itr ) // not found in definitions: invalid switch given!
  {
    opt = require_itr->second;
    return true;
  }
  return false;
}


// arg1 holds a cleaned key (no dash, slash, hyphen, inline arguments.
// arg2 returns if parameter con/must/must not follow.
bool Optargs::argument_requirement( const std::string& clean_key, Option::argtype& required ) const
{
  Option opt;
  if( get_option_definition( clean_key, opt ) )  // found in definitions: switch is in property
  {
    required = opt.m_have_arg;
    return true;
  }
  return false;
}


// m_valuemap; // hold every value accessible by a short key (if no short key exist, one artifical short will be made for the long key)
// m_access_by_name_map;     // find shortname of longname, if none exist, one artifical short will be assigned
bool Optargs::append_definition( const Option& opdef )
{
  string access_key( make_next_access_key() );
  //string long_keyname(  conditional_uppercase( opdef.m_long_keyname, m_style ) );
  //string short_keyname( conditional_uppercase( string( 1, opdef.m_short_keyname ), m_style ) );
  string long_keyname(  opdef.m_long_keyname );
  string short_keyname( string( 1, opdef.m_short_keyname ) );

  if( 0x00==opdef.m_short_keyname )
  {
    if( m_access_by_name_map.end() != m_access_by_name_map.find( NOOAPP_INTERNAL_HELP ) )
    { HANDLE_DEFINITION_ERROR( string("Duplicate option ") + NOOAPP_INTERNAL_HELP + " definition" );
    }
    m_access_by_name_map.insert( { NOOAPP_INTERNAL_HELP, access_key } );
  }

  if( ! long_keyname.empty() )
  {
    if( m_access_by_name_map.end() != m_access_by_name_map.find( long_keyname ) )
    { HANDLE_DEFINITION_ERROR( string("Duplicate option ") + long_keyname + " definition" );
    }
    m_access_by_name_map.insert( { long_keyname, access_key } );
  }

  if( ! short_keyname.empty() )
  {
    if( m_access_by_name_map.end() != m_access_by_name_map.find( short_keyname ) )
    { HANDLE_DEFINITION_ERROR( string("Duplicate option ") + make_printable( short_keyname ) + " definition" );
    }
    m_access_by_name_map.insert( { short_keyname, access_key } );
  }
  
  if( opdef.m_have_arg==No::Option::no_argument && !opdef.m_multi[0].empty() )
  { HANDLE_DEFINITION_ERROR( string("A 'no_argument' Option ") + long_keyname + "/" + make_printable( short_keyname ) + " can not have the default value:\"" + opdef.m_multi[0] + "\"" );
  }

  m_requirements.insert( { access_key, opdef } );
  return true;
}


bool Optargs::uppercase_definition(void)
{
  if( m_style==No::Optargs::WINDOWS )
  {
    std::map< std::string, std::string > m_access_by_uppercase;
    for( auto entry : m_access_by_name_map )
    {
      string UppercaseKey( conditional_uppercase( entry.first, m_style ) );
      if( m_access_by_uppercase.end() != m_access_by_uppercase.find( UppercaseKey ) )
      {
        HANDLE_DEFINITION_ERROR( string("Duplicate option ") + make_printable( UppercaseKey ) + " definition" );
      }
      m_access_by_uppercase.insert( { UppercaseKey, entry.second } );
    }
    m_access_by_name_map.clear();
    m_access_by_name_map.insert( m_access_by_uppercase.begin(), m_access_by_uppercase.end() );
  }
  return true;
}


// arg1 holds a cleaned key (no dash, slash, hyphen, inline arguments.
// arg2 holds the parameter or empty string if no parameter is given.
bool Optargs::append_option( const std::string& clean_key, const std::string& value )
{
  string key( conditional_uppercase( clean_key, m_style ) );
  string access_key = seek_key( key );

  // if we already have an valuemap entry, this is the 2nd or more access. So we already copied the content from requirements
  auto require_itr = m_valuemap.find( access_key );
  if( m_valuemap.end() == require_itr ) // we do not have an valuemap entry, this is the 1st access. Need to copy the content from requirements
  {
    require_itr = m_requirements.find( access_key );
    if( m_requirements.end() == require_itr ) // not found in definitions: invalid switch given!
    {
      return false;
    }
  }

  Option newEntry( require_itr->second );

  if( newEntry.m_have_arg == Option::no_argument && !value.empty() )
  {
    return false; // unexpected value
  }
  else if( newEntry.m_have_arg==Option::required_argument && value.empty() )
  {
    return false; // missing value
  }
  else if( newEntry.m_have_arg == Option::optional_argument && value.empty() && !newEntry.m_multi[0].empty() && newEntry.m_count==0 )
  {
    ;//keep the default value as given value
  }
  else if( (newEntry.m_have_arg == Option::optional_argument || newEntry.m_have_arg==Option::required_argument) && !value.empty() )
  {
    if( newEntry.m_count==0 ) //on 1st write kill the default
      newEntry.m_multi.clear();
    newEntry.m_multi.push_back( value );
  }
  newEntry.m_count++;

  m_valuemap[ access_key ] = newEntry;
  return true;
}



bool Optargs::append_value( const std::string& value )
{
  m_noopts.push_back( value ); // hold everything which is not an argument's value, but a standalone value
  return true;
}



// make the access key for the value map / step1
// input: short option (single char) or long option (word)
// allowed: prefix character(s) -, --, / (depending on style/flavor)
// always returning a string, cut of from the input, but if it comes from argv[] it needs an additional cleaning step.
std::string Optargs::get_key( const std::string& option, std::string& split_off_keys, std::string& split_off_values ) const
{
  string key( conditional_uppercase( option, m_style ) );

  split_off_keys.clear();
  split_off_values.clear();

  if( key.length()==2 && key[0]=='-' && (m_style==POSIX || m_style==GNU || m_style==OLDUNIX || m_style==GNUWINMIX) )  // user requested "-x" instead of "x" - we forgive it
  {
    key = option.substr(1,1);
  }
  else if( key.length()==2 && key[0]=='/' && (m_style==WINDOWS || m_style==GNUWINMIX) )  // user requested "/x" instead of "x" - we forgive it
  {
    key = option.substr(1,1);
  }
  else if( key.length()>2 && key[0]=='-' && key[1]=='-' && ((m_style==GNU) || (m_style==GNUWINMIX)) )  // user requested "--yyy", is only in GNU allowed
  {                                                                                                    // user requested "--yyy", is also in Gnu+Win Mix allowed, then the : or = are delimitters
    const char* delimitter = (m_style==GNUWINMIX) ? "=:" : ";";
    cut_string_at( option.substr(2), delimitter, key, split_off_values ); // note: if called with an argv[] value, there could hang data on, like --file=/tmp/123, with or without single- or double quotes
    string full_key;
    if( ! submatch_key( key, full_key ) )
    {
      return ""; // not a key!
    }
    else
    {
      key = full_key;
    }
    split_off_values = trim_quotes( split_off_values, "\"'" );
  }
  else if( key.length()>2 && key[0]=='/' && (m_style==WINDOWS || m_style==GNUWINMIX) )  // user requested "/yy" Caution! in gnu it could be a path! but the user asked for a key
  {
    string key_or_more( option.substr(1) );
    cut_string_at( key_or_more, "=:", key_or_more, split_off_values );  // note: if called with an argv[] value, there could hang data on, like /file:C:\\tmp\\123 or /file=C:\\tmp\\123, with or without single- or double quotes
    cut_string_at( key_or_more, "/", key, split_off_keys );
    if( !split_off_keys.empty() ) split_off_keys.insert(0,1,'/');
    split_off_values = trim_quotes( split_off_values, "\"'" );
  }
  else if( key.length()>2 && key[0]=='-' && key[1]!='-' && (m_style==POSIX || m_style==GNU) )  // user requested  -m"Git Comment" or -mUpdated alike. this is -m with an parameter
  {
    key = option.substr(1,1);  // note: if called with an argv[] value, there could hang data on, like -mGitComment, -m"Git Comment" or -abc same as -a -b -c
    Option::argtype req;       // here we do not know, if the next char is a key or a parameter!  DEPENDS ON DEFINITION!
    if( argument_requirement( key, req ) && req != Option::required_argument )
    {
      split_off_keys = "-" + option.substr(2);
    }
    else
    {
      split_off_values = trim_quotes( option.substr(2), "\"'" );
    }
  }
  else if( key.length()>=2 && key[0]=='-' && key[1]!='-' && (m_style==OLDUNIX) )
  {
    key = option.substr(1);  // note: if called with an argv[] value, there could be a single char or a word option, like 'type' of "find . -type f", but no -abc as short -a -b -c
  }
  return key;
}


inline bool Optargs::know_key( const std::string& key ) const
{
  // string access_key = seek_key( conditional_uppercase( clean_key, m_style ) );
  return ! seek_key( key ).empty();
}


// GNU allows abbreviated longopts keys, if the match uniqely
bool Optargs::submatch_key( const std::string& key, std::string& full_key ) const
{
  if( key.length()<2 ) // only for --word format keys. not for -a single letter ones
    return false;

  int count_matches=0;
  for( auto it : m_access_by_name_map )
  {
    if( 0==it.first.find( key ) ) // matches left-bordered
    {
      count_matches++;
      if( 1==count_matches ) full_key = it.first;
    }
  }
  return (1==count_matches);  // match uniqely. Not not found, not ambiguous
}


std::string Optargs::seek_key( const std::string& key ) const
{
  string search_key( conditional_uppercase( key, m_style ) );
  auto short_itr = m_access_by_name_map.find( search_key );
  if( m_access_by_name_map.end() == short_itr )
    return string("");
  return short_itr->second;
}


bool Optargs::seek_option( const std::string& option, bool get_value, int* pCount, std::string& result ) const
{
  string search_key( conditional_uppercase( option, m_style ) );
  string clean_key = get_key( search_key ); // arg 2+3 are dummies here, we expect the user provided pure keys or hyphened/dashed keys.

  string access_key = seek_key( clean_key );

  auto value_itr = m_valuemap.find( access_key );
  if( value_itr != m_valuemap.end() ) // found
  {
    if( get_value )
    {
      int count = (pCount) ? *pCount : 0;

      if( count<1 ) count=1; // count is 1-based index, if caller said "0" he means "no care, give me what U have"
      if( count > value_itr->second.m_count )
      {
        if( pCount ) *pCount = value_itr->second.m_count;
        return false;
      }

      if( value_itr->second.m_count>0 && value_itr->second.m_count <= (int)value_itr->second.m_multi.size() )
      {
        result = value_itr->second.m_multi[count-1]; // count is 1-based index
      }
      if( pCount ) *pCount = value_itr->second.m_count;
    }
    return true;
  }
  else if( get_value ) // can we return the default value?
  {
    auto deflt_itr = m_requirements.find( access_key );
    if( deflt_itr != m_requirements.end() )
    {
      result = deflt_itr->second.m_multi[0];
      return true;
    }
    if( pCount ) *pCount = 0;
  }

  // neither a user assigned short key nor an auto generated one? -> so we don't have an entry in the main list
  return false;
}


// ===================================================================
// ============== shell wildcardhandling emulation ===================
// ===================================================================

#if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
// 1) SEARCH_TYPE_SHELL => for n in *mask*; do echo $n; done -> files and folders matching *mask* from this DIR 
// 2) SEARCH_TYPE_FIND  => find . -type f -iname *mask*      -> all files matching *mask* from all DIR s


static vector<wstring> read_folder_contentW( const wchar_t* base_folder, bool prepend, const wchar_t* mask, bool bFiles, bool bFolders, bool recursive, const wchar_t* recurse_folder )
{
  wstring start_path( (base_folder && *base_folder) ? base_folder : L".\\" );
  if( L'\\' != *start_path.rbegin() && L'/' != *start_path.rbegin() ) start_path.push_back( L'\\' );

  wstring search_path( start_path );
  if( recurse_folder && *recurse_folder )
    search_path.append( recurse_folder );
  if( L'\\' != *search_path.rbegin() && L'/' != *search_path.rbegin() ) search_path.push_back( L'\\');

  wstring path_and_mask( search_path );
  path_and_mask.append( (mask && *mask) ? mask : L"*.*" );

  //DBG_TELLw( wstring( L">>>in \"" ) + search_path + L"\"" );
  vector<wstring> names;
  WIN32_FIND_DATAW fd;
  HANDLE hFind = ::FindFirstFileW( path_and_mask.c_str(), &fd ); 
  if( INVALID_HANDLE_VALUE != hFind )
  {
    do
    { 
      if( 0==wcscmp( fd.cFileName, L"." ) ) continue;
      if( 0==wcscmp( fd.cFileName, L".." ) ) continue;

      if( bFiles && ! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        wstring filename( ((prepend) ? start_path : L"") + fd.cFileName );
        names.push_back( filename );
        // DBG_TELLw( wstring( L"+file \"" ) + filename + L"\"" );
      }
      else if( bFolders && ! recursive && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        wstring dirname( ((prepend) ? start_path : L"") + fd.cFileName + L"\\" );
        names.push_back( dirname );
        // DBG_TELLw( wstring( L"+_dir \"" ) + dirname + L"\"" );
      }
      else if( recursive && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // be warned: files + recursive will only give matching folders with matching files, non-matching folders will also not be dived into!!
      {
        // DBG_TELLw( wstring( L".. dive-> \"" ) + fd.cFileName + L"\"" );
        wstring dirname( ((prepend) ? search_path : search_path.substr(start_path.length()) ) + fd.cFileName + L"\\" );
        names.push_back( dirname );
        vector<wstring> subcontent( read_folder_contentW( search_path.c_str(), prepend, mask, bFiles, true, true, fd.cFileName ) );
        // DBG_TELLw( wstring( L".. back<- \"" ) + fd.cFileName + L"\"" );
        names.insert( std::end(names), std::begin(subcontent), std::end(subcontent) );
      }
    } while( ::FindNextFileW( hFind, &fd) ); 
    ::FindClose( hFind ); 
  } // end-if( INVALID_HANDLE_VALUE != hFind )

  //DBG_TELLw( wstring( L"<<out \"" ) + search_path + L"\"" );
  return names;
}


std::vector< std::string> get_folder_contentA( const char* base_folder, bool prepend, const char* mask, bool bFiles, bool bFolders, bool recursive )
{
  vector<string> names_A;
  wstring base_folder_W = UTF8toWideString( base_folder ? base_folder : "" );
  wstring mask_W = UTF8toWideString( mask ? mask : "" );

  vector<wstring> names_W = get_folder_contentW( base_folder_W.c_str(), prepend, mask_W.c_str(), bFiles, bFolders, recursive );
  for( auto f : names_W )
  {
    names_A.push_back( WideString2UTF8( f ) );
  }
  return names_A;
}


std::vector<std::wstring> get_folder_contentW( const wchar_t* base_folder, bool prepend, const wchar_t* mask, bool bFiles, bool bFolders, bool recursive )
{
  if( bFiles && recursive && mask!=nullptr && 0!=wcscmp(mask,L"") && 0!=wcscmp(mask,L"*.*") )
  {
    //DBG_TELLw( wstring( L"-> folders\"" ) + base_folder + L"\"" );
    wstring start_path( (base_folder && *base_folder) ? base_folder : L".\\" );
    if( L'\\' != *start_path.rbegin() && L'/' != *start_path.rbegin() ) start_path.push_back( L'\\' );

    vector<wstring> all_files;
    vector<wstring> all_folders = read_folder_contentW( start_path.c_str(), true, L"", false, true, recursive, nullptr );
    all_folders.push_back( start_path );
    //DBG_TELLw( wstring( L"<- folders\"" ) + base_folder + L"\"" );
    for( auto folder : all_folders )
    {
      //DBG_TELLw( wstring( L"--> files \"" ) + folder + L"\"" );
      vector<wstring> files = read_folder_contentW( folder.c_str(), true, mask, true, false, false, nullptr );
      if( !prepend )
      { // all paths must remove base_folder
        for( wstring& name : files )
        {
          name = name.substr( start_path.length() );
        }
      }
      //DBG_TELLw( wstring( L"<-- files \"" ) + folder + L"\"" );
      all_files.insert( std::end(all_files), std::begin(files), std::end(files) );
    }
    return all_files;
  }
  return read_folder_contentW( base_folder, prepend, mask, bFiles, bFolders, recursive, nullptr );
}

#endif // if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )


// ===================================================================
// ====================== string handling ============================
// ===================================================================
static std::string basename_of_execT( const std::tstring& argv0 )
{
  return basename_of_exec( tstring_to_ascii( argv0 ) );
}


static std::string basename_of_exec( const std::string& argv0 )
{
  string exec_name( argv0 );

  size_t pos = exec_name.find_last_of("\\:/");
  if( exec_name.npos != pos && pos+1 < exec_name.length() )
  {
    exec_name = exec_name.substr( pos+1 ); // skip over the path

    pos = exec_name.rfind(".exe");
    if( exec_name.npos == pos ) pos = exec_name.rfind(".EXE");

    if( exec_name.npos != pos )
      exec_name = exec_name.substr( 0, pos ); // cut off an '.exe' suffix, if found.
  }
  return exec_name;
}



static void cut_string_at( const std::string& input, const std::string& separators, std::string& lhs, std::string& rhs )
{
  size_t cut = input.find_first_of( separators );
  if( input.npos != cut )
  {
    lhs = input.substr( 0, cut );
    if( cut+1 < input.length() )
      rhs = input.substr( cut+1 );
    else
      rhs.clear();
  }
  else
  {
    lhs = input;
    rhs.clear();
  }
}


static std::string trim_quotes( const std::string& input, const std::string& separators )
{
  string result( input );

  size_t len = input.length();
  if( len>=2 ) // if FALSE: this may be a quotation char or not ... it stands alone and is not a quotation, or the input is totally empty.
  {
    char sep1 = input[0];
    char sep2 = input[ len-1 ];

    if( sep1 == sep2 && string::npos != separators.find(sep1) ) // we have it!
    {
      result = input.substr(1,len-2);
    }
  }
  return result;
}


static std::string replace_placeholder( const std::string& input, const PlaceholderReplace& placeholders, const std::string& lead_in, const std::string& lead_out )
{
  string result( input );

  for( auto Placeholder : placeholders )
  {
    string Match( lead_in + Placeholder.first + lead_out );
    size_t start;
    while( result.npos != (start = result.find( Match )) )
    {
      result.replace( start, Match.length(), Placeholder.second ) ;
    }
  }
  return result;
}



/* useful values in win32/win64 are:
    CP_ACP (ANSI-Windows, default)
    CP_OEMCP (437 or 1250 or ,,,)
 */
std::string WideString2CP( const std::wstring & UCS2, unsigned int Codepage )
{
  string UTF8("");

  # if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
  int size_needed = ::WideCharToMultiByte( Codepage, WC_NO_BEST_FIT_CHARS, UCS2.c_str(), -1, nullptr, 0, nullptr, nullptr );
  if( size_needed > 0 )
  {
    //char* converted = new char[ size_needed ];
    //*converted = 0;
    UTF8.resize( size_needed + 10 );
    int res = ::WideCharToMultiByte( Codepage, WC_NO_BEST_FIT_CHARS, UCS2.c_str(), -1, &UTF8[0], (int)UTF8.size(), nullptr, nullptr );
    if( res > 0 )
    {
      //delete [] converted;
      UTF8.resize( res-1 );
      return UTF8;
    }
  }
  DWORD error = ::GetLastError();
  string err_msg = std::system_category().message( (int) error );
  throw OptargConvertF( string("ERROR WideString2UTF8: ") + err_msg );
  # else // non-windows platforms ...
  return UTF8;
  # endif
}



static std::string WideString2UTF8( const std::wstring& UCS2 )
{
  string UTF8("");

# if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
  int size_needed = ::WideCharToMultiByte( CP_UTF8, WC_NO_BEST_FIT_CHARS, UCS2.c_str(), -1, nullptr, 0, nullptr, nullptr );
  if( size_needed > 0 )
  {
    //char* converted = new char[ size_needed ];
    //*converted = 0;
    UTF8.resize( size_needed + 10 );
    int res = ::WideCharToMultiByte( CP_UTF8, WC_NO_BEST_FIT_CHARS, UCS2.c_str(), -1, &UTF8[0], (int)UTF8.size(), nullptr, nullptr );
    if( res > 0 )
    {
      //delete [] converted;
      UTF8.resize( res-1 );
      return UTF8;
    }
  }
  DWORD error = ::GetLastError();
  string err_msg = std::system_category().message( (int) error );
  throw OptargConvertF( string("ERROR WideString2UTF8: ") + err_msg );
# else // non-windows platforms ...
  return UTF8;
# endif
}


static std::wstring UTF8toWideString( const std::string& UTF8 )
{
  wstring UCS(L"");

# if (defined( __WIN32__ ) || defined( WIN32 )) && !defined( __CYGWIN__ )
  int size_needed = ::MultiByteToWideChar( CP_UTF8, 0, UTF8.c_str(), -1, nullptr, 0 );
  if( size_needed > 0 )
  {
    UCS.resize( size_needed + 10 );
    int res = ::MultiByteToWideChar( CP_UTF8, 0, UTF8.c_str(), -1, &UCS[0], (int)UCS.size() );
    if( res > 0 )
    {
      UCS.resize( res-1 );
      return UCS;
    }
  }
  DWORD error = ::GetLastError();
  string err_msg = std::system_category().message( (int) error );
  throw OptargConvertF( string("ERROR UTF8toWideString: ") + err_msg );
# else // non-windows platforms ...
  return UCS;
# endif
}


static std::string get_option_prefix( No::Optargs::style_t Style, bool bForLongOption )
{
  string prefix;
  char* lpfx = "";
  char  cpfx = '\0';
  switch( Style )
  {
    case No::Optargs::GNU       :
      lpfx = NOOAPP_GNU_PFX_LONG;      //  "--"
      cpfx = NOOAPP_GNU_PFX_CHAR;      //  '-'
      break;
    case No::Optargs::POSIX     :
      lpfx = NOOAPP_POSIX_PFX_LONG;    //  "-"
      cpfx = NOOAPP_POSIX_CHAR;        //  '-'
      break;
    case No::Optargs::OLDUNIX   :
      lpfx = NOOAPP_OLDUNIX_PFX_LONG;  //  "-"
      cpfx = NOOAPP_OLDUNIX_CHAR;      //  '-'
      break;
    case No::Optargs::WINDOWS   :
      lpfx = NOOAPP_WINDOWS_PFX_LONG;  //  "/"
      cpfx = NOOAPP_WINDOWS_CHAR;      //  '/'
      break;
    case No::Optargs::GNUWINMIX :
      lpfx = NOOAPP_GNUWINMIX_PFX_LONG;//  "--"
      cpfx = NOOAPP_GNUWINMIX_CHAR;    //  '/'
      break;
  }
  
  if( bForLongOption )
    return string( lpfx );
  else
    return string( 1, cpfx );
}


static std::string get_option_prefix_long( No::Optargs::style_t Style )
{
  return get_option_prefix( Style, true );
}

static std::string get_option_prefix_short(No::Optargs::style_t Style )
{
  return get_option_prefix( Style, false );
}



} // namespace No
