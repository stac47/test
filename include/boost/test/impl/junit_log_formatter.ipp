//  (C) Copyright 2016 Raffi Enficiaud.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///@brief Contains the implementatoin of the Junit log formatter (OF_JUNIT)
// ***************************************************************************

#ifndef BOOST_TEST_JUNIT_LOG_FORMATTER_IPP__
#define BOOST_TEST_JUNIT_LOG_FORMATTER_IPP__

// Boost.Test
#include <boost/test/output/junit_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/tree/test_unit.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/xml_printer.hpp>
#include <boost/test/utils/string_cast.hpp>

// Boost
#include <boost/version.hpp>

// STL
#include <iostream>
#include <set>

#include <boost/test/detail/suppress_warnings.hpp>


#if 0
<?xml version="1.0" encoding="UTF-8"?>
<testsuites disabled="" errors="" failures="" name="" tests="" time="">
    <testsuite disabled="" errors="" failures="" hostname="" id=""
        name="" package="" skipped="" tests="" time="" timestamp="">
        <properties>
            <property name="" value=""/>
            <property name="" value=""/>
        </properties>
        <testcase assertions="" classname="" name="" status="" time="">
            <skipped/>
            <error message="" type=""/>
            <error message="" type=""/>
            <failure message="" type=""/>
            <failure message="" type=""/>
            <system-out/>
            <system-out/>
            <system-err/>
            <system-err/>
        </testcase>
        <testcase assertions="" classname="" name="" status="" time="">
            <skipped/>
            <error message="" type=""/>
            <error message="" type=""/>
            <failure message="" type=""/>
            <failure message="" type=""/>
            <system-out/>
            <system-out/>
            <system-err/>
            <system-err/>
        </testcase>
        <system-out/>
        <system-err/>
    </testsuite>
    <testsuite disabled="" errors="" failures="" hostname="" id=""
        name="" package="" skipped="" tests="" time="" timestamp="">
        <properties>
            <property name="" value=""/>
            <property name="" value=""/>
        </properties>
        <testcase assertions="" classname="" name="" status="" time="">
            <skipped/>
            <error message="" type=""/>
            <error message="" type=""/>
            <failure message="" type=""/>
            <failure message="" type=""/>
            <system-out/>
            <system-out/>
            <system-err/>
            <system-err/>
        </testcase>
        <testcase assertions="" classname="" name="" status="" time="">
            <skipped/>
            <error message="" type=""/>
            <error message="" type=""/>
            <failure message="" type=""/>
            <failure message="" type=""/>
            <system-out/>
            <system-out/>
            <system-err/>
            <system-err/>
        </testcase>
        <system-out/>
        <system-err/>
    </testsuite>
</testsuites>
#endif


//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {


struct s_replace_dot
{
  template <class T>
  void operator()(T& to_replace)
  {
    if(to_replace == '/')
      to_replace = '.';
  }
};

inline std::string tu_name_normalize(std::string full_name)
{
  std::for_each(full_name.begin(), full_name.end(), s_replace_dot());
  return full_name;
}

// ************************************************************************** //
// **************               junit_log_formatter              ************** //
// ************************************************************************** //

void
junit_log_formatter::log_start( std::ostream& ostr, counter_t test_cases_amount)
{
    ostr  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    map_tests.clear();
}

//____________________________________________________________________________//

void
junit_log_formatter::log_finish( std::ostream& ostr )
{
    std::set<std::string> to_be_processed, to_be_processed_next;
    for(map_trace_t::iterator it(map_tests.begin()); it != map_tests.end(); ++it)
    {
      if(it->second.test_type == TUT_CASE)
      {
        to_be_processed.insert(it->first);
      }
    }


    // report from leaves to parent the different attributes
    junit_impl::test_unit_ root;
    while(!to_be_processed.empty())
    {
      to_be_processed_next.clear();
      for(std::set<std::string>::const_iterator it(to_be_processed.begin()); it != to_be_processed.end(); ++it)
      {
        std::ostringstream o_current_test;
        const junit_impl::test_unit_& v = map_tests[*it];
        to_be_processed.erase(it);

        // report the assertion counts to the parent, fist start with the leaves that are test units
        if(!v.parent.empty())
        {
           to_be_processed_next.insert(v.parent);
        }
        junit_impl::test_unit_& parent = v.parent.empty() ? root : map_tests[v.parent];

        parent.assertions += v.assertions;
        parent.failure_nb += v.failure_nb;
        parent.error_nb += v.error_nb;
        parent.disabled_nb += v.disabled_nb;
        parent.test_nb += v.test_type == TUT_CASE ? 1 : v.test_nb;
        parent.time += v.time;
      }

      to_be_processed = to_be_processed_next;
      for(std::set<std::string>::iterator it(to_be_processed_next.begin()); it != to_be_processed_next.end(); ++it)
      {
        const junit_impl::test_unit_& v = map_tests[*it];
        if(to_be_processed_next.count(v.parent) != 0)
        {
          to_be_processed.erase(v.parent);
        }
      }

      //to_be_processed_next.swap(to_be_processed);
    }


    ostr << "<testsuites" 
      << " disabled" << utils::attr_value() << root.disabled_nb
      << " errors"   << utils::attr_value() << root.error_nb
      << " failures" << utils::attr_value() << root.failure_nb 
      << " name"     << utils::attr_value() << "root" // todo replace by the module name
      << " tests"    << utils::attr_value() << root.test_nb
      << " time"     << utils::attr_value() << root.time
      << ">"
      ;

    // would be good to have the full command line and the running environment
#if 0
        <properties>
            <property name="" value=""/>
            <property name="" value=""/>
        </properties>
#endif


    for(map_trace_t::iterator it(map_tests.begin()); it != map_tests.end(); ++it)
    {
      std::ostringstream o_current_test;
      const std::string &name = it->first;
      junit_impl::test_unit_& v = it->second;

      o_current_test << 
        "<testcase classname=\"\" name=\"\" status=\"\" time=\"\">";

      std::string name_last_component;
      std::string name_class;
      std::string::size_type rev = name.rfind('/');
      if(rev != std::string::npos)
      {
        name_last_component = name.substr(rev);
        name_class = name.substr(0, rev);
      }
      else
      {
        name_last_component = name;
        name_class = "root";
      }



      o_current_test << "<testcase "
         << " name"              << utils::attr_value() << name_last_component
         << " classname"         << utils::attr_value() << tu_name_normalize(name_class)
         << " status"            << utils::attr_value() << "failed" // TODO
         << " time"              << utils::attr_value() << v.time
         << ">";

#if 0
         << " result"            << utils::attr_value() << descr
         << " assertions"        << utils::attr_value() << v.assertions
         << " assertions_failed"        << utils::attr_value() << tr.p_assertions_failed
         << " warnings_failed"          << utils::attr_value() << tr.p_warnings_failed
         << " expected_failures"        << utils::attr_value() << tr.p_expected_failures;
#endif

      if(v.skipped)
      {
        o_current_test << "<skipped/>" << std::endl;
        o_current_test << "<system-out>" << std::endl;
        o_current_test << utils::cdata() << v.cdata;
        o_current_test << "</system-out>" << std::endl;
      }
      else
      {
        o_current_test << "<system-out>" << std::endl;
        o_current_test << utils::cdata() << v.cdata;
        o_current_test << "</system-out>" << std::endl;            
#if 0
            <error message="" type=""/>
            <error message="" type=""/>
            <failure message="" type=""/>
            <failure message="" type=""/>
            <system-out/>
            <system-out/>
            <system-err/>
            <system-err/>
#endif
      }
      o_current_test << "</testcase>" << std::endl;
      std::cout << o_current_test.str();
      ostr << o_current_test.str();
    }

    
}

//____________________________________________________________________________//

void
junit_log_formatter::log_build_info( std::ostream& ostr )
{
    ostr  << "<BuildInfo"
            << " platform"  << utils::attr_value() << BOOST_PLATFORM
            << " compiler"  << utils::attr_value() << BOOST_COMPILER
            << " stl"       << utils::attr_value() << BOOST_STDLIB
            << " boost=\""  << BOOST_VERSION/100000     << "."
                            << BOOST_VERSION/100 % 1000 << "."
                            << BOOST_VERSION % 100      << '\"'
            << "/>";
}

//____________________________________________________________________________//

void
junit_log_formatter::test_unit_start( std::ostream& ostr, test_unit const& tu )
{
    junit_impl::test_unit_& v = map_tests[tu.full_name()];
    current_test_unit = map_tests.find(tu.full_name());

    v.test_id = tu.p_id;
    v.test_type = tu.p_type;
    v.parent = framework::get<test_suite>( tu.p_parent_id ).full_name();

    if( !tu.p_file_name.empty() )
        ostr << BOOST_TEST_L( " file" ) << utils::attr_value() << tu.p_file_name
             << BOOST_TEST_L( " line" ) << utils::attr_value() << tu.p_line_num;
}

//____________________________________________________________________________//

void
junit_log_formatter::test_unit_finish( std::ostream& ostr, test_unit const& tu, unsigned long elapsed )
{
    junit_impl::test_unit_ &v = map_tests[tu.full_name()];
    v.disabled = !tu.is_enabled();
    v.time = double(elapsed) / 1000;

    current_test_unit = map_tests.find(v.parent);
}

//____________________________________________________________________________//

void
junit_log_formatter::test_unit_skipped( std::ostream& ostr, test_unit const& tu, const_string reason )
{
    if(tu.p_type == TUT_CASE)
    {
      junit_impl::test_unit_& v = map_tests[tu.full_name()];
      v.skipped = true;
      v.cdata.assign(reason.begin(), reason.end());
    }
}

//____________________________________________________________________________//

void
junit_log_formatter::log_exception_start( std::ostream& ostr, log_checkpoint_data const& checkpoint_data, execution_exception const& ex )
{
    std::ostringstream o;
    execution_exception::location const& loc = ex.where();

    o << "An uncaught exception occured:" << std::endl;
    if( !loc.m_function.is_empty() )
        o << "- function: \""   << loc.m_function << "\"";

    o << "- file: " << loc.m_file_name << std::endl
      << "- line: " << loc.m_line_num;

    if( !checkpoint_data.m_file_name.is_empty() ) {
        o << "Last checkpoint:" << std::endl 
          << "- message: \"" << checkpoint_data.m_message << "\"" << std::endl
          << "- file: " << checkpoint_data.m_file_name << std::endl
          << "- line: " << checkpoint_data.m_line_num << std::endl
        ;
    }

    o << "exception trace:" << std::endl << ex.what();
}

//____________________________________________________________________________//

void
junit_log_formatter::log_exception_finish( std::ostream& ostr )
{
  // check if we need to go to the parent 
}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_start( std::ostream& ostr, log_entry_data const& entry_data, log_entry_types let )
{
    //static literal_string xml_tags[] = { "Info", "Message", "Warning", "Error", "FatalError" };

#if 0
      enum log_entry_types { BOOST_UTL_ET_INFO,       ///< Information message from the framework
                           BOOST_UTL_ET_MESSAGE,    ///< Information message from the user
                           BOOST_UTL_ET_WARNING,    ///< Warning (non error) condition notification message
                           BOOST_UTL_ET_ERROR,      ///< Non fatal error notification message
                           BOOST_UTL_ET_FATAL_ERROR ///< Fatal error notification message
#endif

    if(let == unit_test_log_formatter::BOOST_UTL_ET_INFO)
    {

    }

    if(let == unit_test_log_formatter::BOOST_UTL_ET_ERROR || let == unit_test_log_formatter::BOOST_UTL_ET_FATAL_ERROR)
    {
      current_test_unit->second.assertions ++;
      current_test_unit->second.error_nb ++;

      std::ostringstream o;
      m_curr_tag = "failure";
      o << "<" << m_curr_tag
        << " type" << utils::attr_value() 
                   << (let == unit_test_log_formatter::BOOST_UTL_ET_ERROR ? "assertion error" : "fatal error")
        << ">";
      

      o << BOOST_TEST_L( "Assertion failed at " ) 
        << entry_data.m_file_name << ":" << entry_data.m_line_num << std::endl;

      m_value_closed = false;
      current_test_unit->second.failure += o.str();
    }

#if 0
   
    ostr << '<' << m_curr_tag
         << BOOST_TEST_L( " file" ) << utils::attr_value() << entry_data.m_file_name
         << BOOST_TEST_L( " line" ) << utils::attr_value() << entry_data.m_line_num
         << BOOST_TEST_L( "><![CDATA[" );

#endif
}

      //____________________________________________________________________________//



//____________________________________________________________________________//

void
junit_log_formatter::log_entry_value( std::ostream& ostr, const_string value )
{
    std::ostringstream o;
    utils::print_escaped_cdata( o, value );
    current_test_unit->second.failure += o.str();
}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_finish( std::ostream& ostr )
{
#if 0
    if( !m_value_closed ) {
        ostr << BOOST_TEST_L( "]]>" );
        m_value_closed = true;
    }
#endif

    if(!m_curr_tag.empty())
    {
      current_test_unit->second.failure += "</" + m_curr_tag + ">";
      m_curr_tag.clear();
    }

}

//____________________________________________________________________________//

void
junit_log_formatter::entry_context_start( std::ostream& ostr, log_level )
{

}

//____________________________________________________________________________//

void
junit_log_formatter::entry_context_finish( std::ostream& ostr )
{

}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_context( std::ostream& ostr, const_string context_descr )
{
    ostr << BOOST_TEST_L( "<Frame>" ) << utils::cdata() << context_descr << BOOST_TEST_L( "</Frame>" );
}

//____________________________________________________________________________//

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_junit_log_formatter_IPP_020105GER
