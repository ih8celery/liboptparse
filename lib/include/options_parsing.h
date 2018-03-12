/**
 * \file options_parsing.h
 * 
 * \author Adam Marshall (ih8celery)
 *
 * \brief interface to parse command line arguments/options
 */
#ifndef _MOD_CPP_OPTIONS_PARSING

#define _MOD_CPP_OPTIONS_PARSING

#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>
#include <exception>

namespace util {
  using opt_data_t = std::multimap<std::string, std::string>;

  using RangePair = std::pair<opt_data_t::const_iterator, opt_data_t::const_iterator>;

  namespace config_constants {
    struct case_sensitive_t {};
    struct bsd_opt_t {};
    struct merged_opt_t {};
    struct error_if_unknown_t {};
    struct subcommand_t {};

    const case_sensitive_t case_sensitive = case_sensitive_t();
    const bsd_opt_t bsd_opt = bsd_opt_t();
    const merged_opt_t merged_opt = merged_opt_t();
    const error_if_unknown_t error_unknown = error_if_unknown_t();
    const subcommand_t sub = subcommand_t();
  }

  /**
   * \enum Mod_Prop
   * \brief defines the type of modifier of an option
   * 
   * a specification of the Mod_Prop conforms to the following:
   * <mod_prop>     := '['<modifier>']' | <nil>
   * <modifier>     := <mod_function><mod_arg> | <mod_setting>
   * <mod_function> := '<'   // Mod_Prop::BEFORE
   *                 | '>'   // Mod_Prop::AFTER
   *                 | '!'   // Mod_Prop::NOT_WITH
   * <mod_arg>      := <option_name>
   * <mod_setting>  := '&'   // Mod_Prop::SUB
   *
   * in case a mod_function is used, the mod_arg need not name a
   * valid option
   */
  enum class Mod_Prop {
    NONE,    /// no modifier for option
    BEFORE,  /// option must appear before modifier argument
    AFTER,   /// option must appear after modifier argument
    SUB,     /// option is a subcommand
    NOT_WITH /// option may not appear in the same arglist as modifier argument
  };

  /**
   * \enum Num_Prop
   * \brief defines the number of times an option may appear
   *
   * <num_prop> := <nil> // Num_Prop::ZERO_ONE
   *             | '?'   // Num_Prop::ZERO_ONE
   *             | '*'   // Num_Prop::ZERO_MANY
   */
  enum class Num_Prop {
    ZERO_ONE, /// option may appear zero times or one
    ZERO_MANY /// option may appear any number of times
  };

  /**
   * \enum Assign_Prop
   * \brief defines the mode of assignment supported by option
   *
   * <assign_prop> := <nil> // Assign_Prop::NONE
   *                | '='   // Assign_Prop::EQ_REQUIRED
   *                | '=?'  // Assign_Prop::EQ_MAYBE
   *                | '=!'  // Assign_Prop::EQ_NEVER
   *                | '=|'  // Assign_Prop::STUCK_ARG
   */
  enum class Assign_Prop {
    NO_ASSIGN,   /// option may not take argument
    EQ_REQUIRED, /// option takes argument which must be delimited by '='
    EQ_MAYBE,    /// option takes argument which may or may not be delimited by '='
    EQ_NEVER     /// option takes argument NOT delimited by '='
  };

  /**
   * \enum Collect_Prop
   * \brief defines how arguments are interpreted
   *
   * <collect_prop> := <nil>             // Collect_Prop::SCALAR
   *                 | '['<data_prop>']' // Collect_Prop::LIST
   */
  enum class Collect_Prop {
    SCALAR, /// argument is stored as a single object
    LIST    /// argument is split on delimiter and stored as chunks
  };

  /**
   * \enum Data_Prop
   * \brief defines the type of data of an option's argument
   *
   * <data_prop> := <nil> // Data_Prop::STRING
   *              | 's'   // Data_Prop::STRING
   *              | 'i'   // Data_Prop::INTEGER
   *              | 'f'   // Data_Prop::FLOAT
   */
  enum class Data_Prop {
    STRING,  /// any printable character permitted
    INTEGER, /// argument must contain only digits
    FLOAT    /// argument must be /INTEGER/ or /INTEGER\.INTEGER/
  };

  /**
   * \struct option_t
   * \brief collects the properties and identity of an option
   */
  struct option_t {
    /**
     * \fn option_t()
     * \brief default constructor for option_t
     */
    option_t();

    Mod_Prop mod;
    std::string mod_arg;

    Num_Prop number;
    
    Assign_Prop assignment;
    
    Collect_Prop collection;
    
    Data_Prop data_type;

    std::string name;

    /**
     * \fn bool compatible(const option_t&)
     * \brief tests whether options are similar enough to be used \
     *   interchangeably
     */
    bool compatible(const option_t& opt) const {
      return (number == opt.number
              && assignment == opt.assignment
              && data_type == opt.data_type
              && collection == opt.collection);
    }

    /**
     * \fn bool operator==(const option_t&) const
     * \brief compares names of options
     */
    bool operator==(const option_t& opt) const {
      return (name == opt.name);
    }

    /**
     * \fn bool operator<(const option_t&) const
     * \brief compares names of options
     */
    bool operator<(const option_t& opt) const {
      return (name < opt.name);
    }
  };

  /**
   * \class opt_info
   * \brief represents data collected during parsing
   */
  class opt_info {
    friend class opt_parser;

    public:
      /**
       * \fn bool has(const string&)
       * \brief test whether a particular option was found in parsing
       */
      bool has(const std::string&) const;
      
      /**
       * \fn size_t count(const string&)
       * \brief count occurrences of option during parsing
       */
      opt_data_t::size_type count(const std::string&) const;

      /**
       * \fn string arg(const string&, const string = "")
       * \brief retrieve an option's argument, or default value if it had none
       */
      std::string arg(const std::string&, const std::string& = "") const;
      
      /**
       * \fn RangePair list(const string&)
       * \brief retrieve the range of iterators with parameter as key
       */
      RangePair list(const std::string&) const;

      /**
       * \var vector<string> rem
       * \brief contains non-options found during parsing
       */
      std::vector<std::string> rem;
    private:
      /**
       * \var opt_data
       * \brief contains option
       */
      opt_data_t opt_data;
  };

  /*
   * declarations of opt_parser's helpers
   */
  namespace {
    /*
     * called by parse() to check that the argument(s) to an option
     * conform to their declared Data_Prop
     */
    bool verify_arg_type(const std::string&, Data_Prop);
  }

  /**
   * \class opt_parser
   * \brief class controlling option declaration and parsing
   */
  class opt_parser {
    public:
      /**
       * \fn opt_parser()
       * \brief default constructor of class opt_parser
       */
      opt_parser();

      /**
       * \fn option_t option(const string&, const string& = "")
       * \brief declare an option to the parser
       *
       * option name is the second argument to the function
       * see doc/option/spec.md for full description of options
       *
       * throws an option_language_error if something goes wrong
       */
      option_t option(const std::string&, const std::string& = "");

      /**
       * \fn opt_info parse(char **&, int)
       * \brief extract options from argv into an opt_info object
       *
       * any options or data belonging to options is removed from
       * argv and replaced with nullptr
       *
       * throws a parse_error if something goes wrong
       */
      opt_info parse(char ** &argv, int argc);

      /** 
       * \fn void set(const config_constants::case_sensitive_t&, bool = true)
       * \brief configure opt_parser's case sensitivity when parsing
       * 
       * case sensitivity of option names and stored handles is NOT affected
       *
       * default: true
       */
      void set(const config_constants::case_sensitive_t& c, bool val = true) {
        if (this->empty()) {
          is_case_sensitive = val;
        }
      }
      
      /**
       * \fn void set(const config_constants::bsd_opt_t&, bool = true)
       * \brief configure opt_parser's ability to detect bsd-style options
       * 
       * zero or more bsd-style options may then appear at the beginning of argv,
       * under the following conditions:
       *   1. option prefixes (-, --, +, etc) are not allowed in these options
       *   2. option handles must be exactly one letter long without a prefix
       *   3. options must be concatenated into one string, stored in argv[0]
       *   4. the first invalid or incomplete option causes parsing to fail
       *
       * default: false
       *
       * NOTE: "BSD-style" options conflict with subcommands, so enabling
       *   this setting implicitly disables subcommands
       */
      void set(const config_constants::bsd_opt_t& c, bool val = true) {
        if (this->empty()) {
          is_bsd_opt_enabled = val;
          
          if (val) {
            is_subcommand_enabled = false;
          }
        }
      }
      
      /**
       * \fn void set(const config_constants::merged_opt_t&, bool = true)
       * \brief configure opt_parser's ability to detect merged options
       *
       * merged options are identical to bsd-style options except in one
       * respect: the first option detected in argv[0] must include its
       * prefix
       *
       * default: false
       *
       * NOTE: "merged" options conflict with subcommands, so enabling
       *   this setting implicitly disables subcommands
       */
      void set(const config_constants::merged_opt_t& c, bool val = true) {
        if (this->empty()) {
          is_merged_opt_enabled = val;

          if (val) {
            is_subcommand_enabled = false;
          }
        }
      }

      /**
       * \fn void set(const config_constants::error_if_unknown_t&, bool = true)
       * \brief configure opt_parser's response to a char* in argv that resembles \
       *   a valid option but is not known to the parser
       * 
       * true causes error to be thrown when such an invalid option found
       *
       * default: true
       */
      void set(const config_constants::error_if_unknown_t& c, bool val = true) {
        if (this->empty()) {
          is_error_unknown_enabled = val;
        }
      }

      /**
       * \fn void set(const config_constants::subcommand_t&, bool = true)
       * \brief configure opt_parser's ability to detect subcommands. opt_parser
       * may not support subcommands and merged options or bsd options
       * simultaneously. a subcommand may be any valid handle, but it must
       * appear in argv[0]. if subcommands are enabled, it is an error
       * NOT to provide one. subcommands may still be declared in the
       * option spec, but will be ignored if this setting is false.
       *
       * default: false
       *
       * NOTE: subcommands conflict with "BSD-style" and "merged" options,
       *   so enabling this setting implicitly disables those settings
       */
      void set(const config_constants::subcommand_t& c, bool val = true) {
        if (this->empty()) {
          is_subcommand_enabled = val;

          if (val) {
            is_merged_opt_enabled = false;
            is_bsd_opt_enabled    = false;
          }
        }
      }

      /**
       * \fn bool empty() const
       * \brief tests whether the parser has any registered options
       */
      bool empty() const {
        return name_set.empty();
      }

      std::string get_opt(std::string spec, char ** &argv, int argc);

      std::vector<std::string> get_opt_list(std::string spec, char ** &argv, int argc);
      
      /**
       * \fn void clear_options()
       * \brief clear the data collected from calls to option()
       */
      void clear_options() {
        handle_map.clear();
        name_set.clear();
      }

      /**
       * \fn bool handle_has_name(const string&, const string&)
       * \brief test whether option with handle has given name
       */
      bool handle_has_name(const std::string& handle, const std::string& name) const {
        const std::map<std::string, option_t>::const_iterator
          iter = handle_map.find(handle);

        if (iter == handle_map.cend()) {
          return false;
        }
        else {
          return (name == iter->second.name);
        }
      }
    private:
      std::map<std::string, option_t> handle_map;
      std::set<option_t> name_set;

      bool is_case_sensitive;
      bool is_bsd_opt_enabled;
      bool is_merged_opt_enabled;
      bool is_error_unknown_enabled;
      bool is_subcommand_enabled;
      bool is_mod_found;
  };

  /**
   * \class parse_error
   * \brief thrown during parsing
   */
  class parse_error : std::exception {
    public:
      parse_error(std::string&& msg): data(std::move(msg)) {}

      const char * what() const noexcept override {
        return data.c_str();
      }
    private:
      std::string data;
  };

  /**
   * \class option_language_error
   * \brief thrown when options declared or, in one case, at the \
   *   start of parsing
   */
  class option_language_error : std::exception {
    public:
      option_language_error(std::string&& msg): data(std::move(msg)) {}

      const char * what() const noexcept override {
        return data.c_str();
      }

    private:
      std::string data;
  };
}

#endif
