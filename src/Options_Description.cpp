#include "program_options/OptionsDescription.hpp"
#include <cassert>
#include <climits>
#include <cstring>
#include <cstdarg>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <iostream>
#include "../include/program_options/Parsers.hpp"

using namespace std;

namespace options {

   namespace {

       template< class charT >
       std::basic_string< charT >  tolower_(const std::basic_string< charT >& str)
       {
           std::basic_string< charT > result;
           for (typename std::basic_string< charT >::size_type i = 0; i < str.size(); ++i)
           {
               result.append(1, static_cast< charT >(std::tolower(str[i])));
           }   
           return result;
       }

    }

    OptionDescription::OptionDescription()
    {
    }
    
    OptionDescription::
    OptionDescription(const char* name,
                       const Value_semantic* s)
    : m_value_semantic(s)
    {
        this->set_name(name);
    }
                                           

    OptionDescription::
    OptionDescription(const char* name,
                       const Value_semantic* s,
                       const char* description)
    : m_description(description), m_value_semantic(s)
    {
        this->set_name(name);
    }

    OptionDescription::~OptionDescription()
    {
    }

    OptionDescription::match_result
    OptionDescription::match(const std::string& option, 
                              bool approx, 
                              bool long_ignore_case,
                              bool short_ignore_case) const
    {
        match_result result = no_match;        
        
        std::string local_long_name((long_ignore_case ? tolower_(m_long_name) : m_long_name));
        
        if (!local_long_name.empty()) {
        
            std::string local_option = (long_ignore_case ? tolower_(option) : option);

            if (*local_long_name.rbegin() == '*')
            {
                // The name ends with '*'. Any specified name with the given
                // prefix is OK.
                if (local_option.find(local_long_name.substr(0, local_long_name.length()-1))
                    == 0)
                    result = approximate_match;
            }

            if (local_long_name == local_option)
            {
                result = full_match;
            }
            else if (approx)
            {
                if (local_long_name.find(local_option) == 0)
                {
                    result = approximate_match;
                }
            }
        }
         
        if (result != full_match)
        {
            std::string local_option(short_ignore_case ? tolower_(option) : option);
            std::string local_short_name(short_ignore_case ? tolower_(m_short_name) : m_short_name);
            if (local_short_name == local_option)
            {
                result = full_match;
            }
        }

        return result;        
    }

    const std::string& 
    OptionDescription::key(const std::string& option) const
    {        
        if (!m_long_name.empty()) 
            if (m_long_name.find('*') != string::npos)
                // The '*' character means we're long_name
                // matches only part of the input. So, returning
                // long name will remove some of the information,
                // and we have to return the option as specified
                // in the source.
                return option;
            else
                return m_long_name;
        else
            return m_short_name;
    }

    std::string 
    OptionDescription::canonical_display_name(int prefix_style) const
    {
        if (!m_long_name.empty()) 
        {
            // if (prefix_style == command_line_style::allow_long)
                return "--" + m_long_name;
            // if (prefix_style == command_line_style::allow_long_disguise)
            //     return "-" + m_long_name;
        }
        // sanity check: m_short_name[0] should be '-' or '/'
        if (m_short_name.length() == 2)
        {
            // if (prefix_style == command_line_style::allow_slash_for_short)
                return string("/") + m_short_name[1];
            // if (prefix_style == command_line_style::allow_dash_for_short)
            //     return string("-") + m_short_name[1];
        }
        if (!m_long_name.empty()) 
            return m_long_name;
        else
            return m_short_name;
    }


    const std::string&
    OptionDescription::long_name() const
    {
        return m_long_name;
    }

    OptionDescription&
    OptionDescription::set_name(const char* _name)
    {
        std::string name(_name);
        string::size_type n = name.find(',');
        if (n != string::npos) {
            assert(n == name.size()-2);
            m_long_name = name.substr(0, n);
            m_short_name = '-' + name.substr(n+1,1);
        } else {
            m_long_name = name;
        }
        return *this;
    }

    const std::string&
    OptionDescription::description() const
    {
        return m_description;
    }

    std::shared_ptr<const Value_semantic>
    OptionDescription::semantic() const
    {
        return m_value_semantic;
    }
    
    std::string 
    OptionDescription::format_name() const
    {
        if (!m_short_name.empty())
        {
            return m_long_name.empty() 
                ? m_short_name 
                : string(m_short_name).append(" [ --").
                  append(m_long_name).append(" ]");
        }
        return string("--").append(m_long_name);
    }

    std::string 
    OptionDescription::format_parameter() const
    {
        if (m_value_semantic->max_tokens() != 0)
            return m_value_semantic->name();
        else
            return "";
    }

    Description_easy_init::
    Description_easy_init(OptionsDescription* owner)
    : owner(owner)
    {}

    Description_easy_init&
    Description_easy_init::
    operator()(const char* name,
               const char* description)
    {
        // Create untypes semantic which accepts zero tokens: i.e. 
        // no value can be specified on command line.
        // FIXME: does not look exception-safe
        std::shared_ptr<OptionDescription> d(
            new OptionDescription(name, new Untyped_value(true), description));

        owner->add(d);
        return *this;
    }

    Description_easy_init&
    Description_easy_init::
    operator()(const char* name,
               const Value_semantic* s)
    {
        std::shared_ptr<OptionDescription> d(new OptionDescription(name, s));
        owner->add(d);
        return *this;
    }

    Description_easy_init&
    Description_easy_init::
    operator()(const char* name,
               const Value_semantic* s,
               const char* description)
    {
        std::shared_ptr<OptionDescription> d(new OptionDescription(name, s, description));

        owner->add(d);
        return *this;
    }

    OptionsDescription::OptionsDescription(unsigned line_length,
                                             unsigned min_description_length)
    : m_line_length(line_length)
    , m_min_description_length(min_description_length)
    {
        // we require a space between the option and description parts, so add 1.
        assert(m_min_description_length < m_line_length - 1);    
    }

    OptionsDescription::OptionsDescription(const std::string& caption,
                                             unsigned line_length,
                                             unsigned min_description_length)
    : m_caption(caption)
    , m_line_length(line_length)
    , m_min_description_length(min_description_length)
    {
        // we require a space between the option and description parts, so add 1.
        assert(m_min_description_length < m_line_length - 1);
    }
    
    void
    OptionsDescription::add(std::shared_ptr<OptionDescription> desc)
    {
        m_options.push_back(desc);
        belong_to_group.push_back(false);
    }

    OptionsDescription&
    OptionsDescription::add(const OptionsDescription& desc)
    {
        std::shared_ptr<OptionsDescription> d(new OptionsDescription(desc));
        groups.push_back(d);

        for (size_t i = 0; i < desc.m_options.size(); ++i) {
            add(desc.m_options[i]);
            belong_to_group.back() = true;
        }

        return *this;
    }

    Description_easy_init
    OptionsDescription::add_options()
    {       
        return Description_easy_init(this);
    }

    const OptionDescription*
    OptionsDescription::find(const std::string& name, 
                              bool approx,
                              bool long_ignore_case,
                              bool short_ignore_case) const
    {
        return find_nothrow(name, approx,
                                       long_ignore_case, short_ignore_case);
    }

    const std::vector< std::shared_ptr<OptionDescription> >& 
    OptionsDescription::options() const
    {
        return m_options;
    }

    const OptionDescription*
    OptionsDescription::find_nothrow(const std::string& name, 
                                      bool approx,
                                      bool long_ignore_case,
                                      bool short_ignore_case) const
    {
        std::shared_ptr<OptionDescription> found;
        bool had_full_match = false;
        vector<string> approximate_matches;
        vector<string> full_matches;
        
        // We use linear search because matching specified option
        // name with the declared option name need to take care about
        // case sensitivity and trailing '*' and so we can't use simple map.
        for(unsigned i = 0; i < m_options.size(); ++i)
        {
            OptionDescription::match_result r = 
                m_options[i]->match(name, approx, long_ignore_case, short_ignore_case);

            if (r == OptionDescription::no_match)
                continue;

            if (r == OptionDescription::full_match)
            {                
                full_matches.push_back(m_options[i]->key(name));
                found = m_options[i];
                had_full_match = true;
            } 
            else 
            {                        
                // FIXME: the use of 'key' here might not
                // be the best approach.
                approximate_matches.push_back(m_options[i]->key(name));
                if (!had_full_match)
                    found = m_options[i];
            }
        }
        if (full_matches.size() > 1) 
            throw std::exception();
        
        // If we have a full match, and an approximate match,
        // ignore approximate match instead of reporting error.
        // Say, if we have options "all" and "all-chroots", then
        // "--all" on the command line should select the first one,
        // without ambiguity.
        if (full_matches.empty() && approximate_matches.size() > 1)
            throw std::exception();

        return found.get();
    }

    
    std::ostream& operator<<(std::ostream& os, const OptionsDescription& desc)
    {
        desc.print(os);
        return os;
    }

    namespace {

        /* Given a string 'par', that contains no newline characters
           outputs it to 'os' with wordwrapping, that is, as several
           line.

           Each output line starts with 'indent' space characters, 
           following by characters from 'par'. The total length of
           line is no longer than 'line_length'.
                                      
        */
        void format_paragraph(std::ostream& os,
                              std::string par,
                              unsigned indent,
                              unsigned line_length)
        {                    
            // Through reminder of this function, 'line_length' will
            // be the length available for characters, not including
            // indent.
            assert(indent < line_length);
            line_length -= indent;

            // index of tab (if present) is used as additional indent relative
            // to first_column_width if paragrapth is spanned over multiple
            // lines if tab is not on first line it is ignored
            string::size_type par_indent = par.find('\t');

            if (par_indent == string::npos)
            {
                par_indent = 0;
            }
            else
            {
                // only one tab per paragraph allowed
                if (count(par.begin(), par.end(), '\t') > 1)
                {
                    throw std::exception(error(
                        "Only one tab per paragraph is allowed in the options description"));
                }
          
                // erase tab from string
                par.erase(par_indent, 1);

                // this assert may fail due to user error or 
                // environment conditions!
                assert(par_indent < line_length);

                // ignore tab if not on first line
                if (par_indent >= line_length)
                {
                    par_indent = 0;
                }            
            }
          
            if (par.size() < line_length)
            {
                os << par;
            }
            else
            {
                string::const_iterator       line_begin = par.begin();
                const string::const_iterator par_end = par.end();

                bool first_line = true; // of current paragraph!        
            
                while (line_begin < par_end)  // paragraph lines
                {
                    if (!first_line)
                    {
                        // If line starts with space, but second character
                        // is not space, remove the leading space.
                        // We don't remove double spaces because those
                        // might be intentianal.
                        if ((*line_begin == ' ') &&
                            ((line_begin + 1 < par_end) &&
                             (*(line_begin + 1) != ' ')))
                        {
                            line_begin += 1;  // line_begin != line_end
                        }
                    }

                    // Take care to never increment the iterator past
                    // the end, since MSVC 8.0 (brokenly), assumes that
                    // doing that, even if no access happens, is a bug.
                    unsigned remaining = static_cast<unsigned>(std::distance(line_begin, par_end));
                    string::const_iterator line_end = line_begin + 
                        ((remaining < line_length) ? remaining : line_length);
            
                    // prevent chopped words
                    // Is line_end between two non-space characters?
                    if ((*(line_end - 1) != ' ') &&
                        ((line_end < par_end) && (*line_end != ' ')))
                    {
                        // find last ' ' in the second half of the current paragraph line
                        string::const_iterator last_space =
                            find(reverse_iterator<string::const_iterator>(line_end),
                                 reverse_iterator<string::const_iterator>(line_begin),
                                 ' ')
                            .base();
                
                        if (last_space != line_begin)
                        {                 
                            // is last_space within the second half ot the 
                            // current line
                            if (static_cast<unsigned>(std::distance(last_space, line_end)) < 
                                (line_length / 2))
                            {
                                line_end = last_space;
                            }
                        }                                                
                    } // prevent chopped words
             
                    // write line to stream
                    copy(line_begin, line_end, ostream_iterator<char>(os));
              
                    if (first_line)
                    {
                        indent += static_cast<unsigned>(par_indent);
                        line_length -= static_cast<unsigned>(par_indent); // there's less to work with now
                        first_line = false;
                    }

                    // more lines to follow?
                    if (line_end != par_end)
                    {
                        os << '\n';
                
                        for(unsigned pad = indent; pad > 0; --pad)
                        {
                            os.put(' ');
                        }                                                        
                    }
              
                    // next line starts after of this line
                    line_begin = line_end;              
                } // paragraph lines
            }          
        }                              
        
        void format_description(std::ostream& os,
                                const std::string& desc, 
                                unsigned first_column_width,
                                unsigned line_length)
        {
            // we need to use one char less per line to work correctly if actual
            // console has longer lines
            assert(line_length > 1);
            if (line_length > 1)
            {
                --line_length;
            }

            // line_length must be larger than first_column_width
            // this assert may fail due to user error or environment conditions!
            assert(line_length > first_column_width);

            // Note: can't use 'tokenizer' as name of typedef -- borland
            // will consider uses of 'tokenizer' below as uses of
            // boost::tokenizer, not typedef.
            // typedef boost::tokenizer<boost::char_separator<char> > tok;
          
            // tok paragraphs(
            //     desc,
            //     char_separator<char>("\n", "", boost::keep_empty_tokens));
          
            // tok::const_iterator       par_iter = paragraphs.begin();                
            // const tok::const_iterator par_end = paragraphs.end();

            // while (par_iter != par_end)  // paragraphs
            // {
            //     format_paragraph(os, *par_iter, first_column_width, 
            //                      line_length);
            
            //     ++par_iter;
            
            //     // prepair next line if any
            //     if (par_iter != par_end)
            //     {
            //         os << '\n';
              
            //         for(unsigned pad = first_column_width; pad > 0; --pad)
            //         {
            //             os.put(' ');
            //         }                    
            //     }            
            // }  // paragraphs
        }
    
        void format_one(std::ostream& os, const OptionDescription& opt, 
                        unsigned first_column_width, unsigned line_length)
        {
            stringstream ss;
            ss << "  " << opt.format_name() << ' ' << opt.format_parameter();
            
            // Don't use ss.rdbuf() since g++ 2.96 is buggy on it.
            os << ss.str();

            if (!opt.description().empty())
            {
                if (ss.str().size() >= first_column_width)
                {
                   os.put('\n'); // first column is too long, lets put description in new line
                   for (unsigned pad = first_column_width; pad > 0; --pad)
                   {
                      os.put(' ');
                   }
                } else {
                   for(unsigned pad = first_column_width - static_cast<unsigned>(ss.str().size()); pad > 0; --pad)
                   {
                      os.put(' ');
                   }
                }
            
                format_description(os, opt.description(),
                                   first_column_width, line_length);
            }
        }
    }

    unsigned                                                                    
    OptionsDescription::get_option_column_width() const                                
    {
        /* Find the maximum width of the option column */
        unsigned width(23);
        unsigned i; // vc6 has broken for loop scoping
        for (i = 0; i < m_options.size(); ++i)
        {
            const OptionDescription& opt = *m_options[i];
            stringstream ss;
            ss << "  " << opt.format_name() << ' ' << opt.format_parameter();
            width = (max)(width, static_cast<unsigned>(ss.str().size()));            
        }

        /* Get width of groups as well*/
        for (unsigned j = 0; j < groups.size(); ++j)                            
            width = max(width, groups[j]->get_option_column_width());

        /* this is the column were description should start, if first
           column is longer, we go to a new line */
        const unsigned start_of_description_column = m_line_length - m_min_description_length;

        width = (min)(width, start_of_description_column-1);
        
        /* add an additional space to improve readability */
        ++width;
        return width;                                                       
    }

    void 
    OptionsDescription::print(std::ostream& os, unsigned width) const
    {
        if (!m_caption.empty())
            os << m_caption << ":\n";

        if (!width)
            width = get_option_column_width();

        /* The options formatting style is stolen from Subversion. */
        for (unsigned i = 0; i < m_options.size(); ++i)
        {
            if (belong_to_group[i])
                continue;

            const OptionDescription& opt = *m_options[i];

            format_one(os, opt, width, m_line_length);

            os << "\n";
        }

        for (unsigned j = 0; j < groups.size(); ++j) {            
            os << "\n";
            groups[j]->print(os, width);
        }
    }

}
