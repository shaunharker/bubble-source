#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>
#include <set>
#include <exception>

#include <cstdint>
#include <stack>
#include <unordered_set>

typedef boost::property_tree::ptree Node;
typedef std::pair<std::string const, Node> Child;
struct Item {
  std::string const name;
  Node const* node;
  uint64_t depth;
  Item () {}
  Item ( Child const& child ) 
  : name(child.first), node(&child.second), depth(0) {}  
  Item ( Child const& child, uint64_t depth ) 
  : name(child.first), node(&child.second), depth(depth+1) {}
  std::string payload ( void ) const {
    return node -> get<std::string> ( "" );
  }
};
std::ostream& operator << ( std::ostream& stream, Item const& item ) {
  for ( uint64_t i = 0; i < item.depth; ++ i ) stream << "  ";
  stream << item . name;
  return stream;
}

/// FullyQualifiedItem
///   name has . delimited string of parent tag names
struct FullyQualifiedItem {
  std::string const name;
  std::string const shortname;
  Node const* node;
  uint64_t depth;
  FullyQualifiedItem () {}
  FullyQualifiedItem ( Child const& child ) 
  : name(child.first), shortname(child.first), node(&child.second), depth(0) {}
  FullyQualifiedItem ( FullyQualifiedItem const& parent, Child const& child ) 
  : name(parent.name + "." + child.first), shortname(child.first), node(&child.second), depth(parent.depth + 1) {}  
  std::string payload ( void ) const {
    return node -> get<std::string> ( "" );
  }
};
std::ostream& operator << ( std::ostream& stream, FullyQualifiedItem const& item ) {
  stream << item . name;
  return stream;
}

int example1 ( int argc, char * argv [] ) {
  std::string query = "";
  if ( argc > 1 ) {
    query = argv[1];
  }
  std::string filename = "./xml/class_network.xml";
  Node tree;
  boost::property_tree::read_xml(filename, tree);
  std::stack<Item> items;
  for ( auto & x : tree ) items . push ( x );
  while ( not items . empty () ) {
    Item item = items . top ();
    items . pop ();
    if ( query . size () == 0 || item . name == query )
    std::cout << item << " : \"" << item.payload () << "\"\n";
    for ( auto & x : * item . node ) items . push ( Item ( x, item.depth ) );
  }
  //std::cout << "Finished query " << query << "\n";
  //std::cout << argc << "\n";
  return 0;
}

/// checkSelector 
///   If this were cool, it would accept W3 selector syntax
///   But instead it just checks for substrings.
bool checkSelector ( std::string const& selector, 
                     std::unordered_set<std::string> const& dict ){
  for ( std::string const& dictword : dict ) {
    if ( selector . find ( dictword ) != -1 ) {
      //std::cout << "Found " << selector << " in " << dictword << "\n";
      return true;
    }
  }
  return false;
}

// return final payload associated with selector string
std::string fetch ( std::string const& filename, std::string const& selector ) {
  std::string result;
  std::unordered_set<std::string> queries;
  queries . insert ( selector );
  Node tree;
  boost::property_tree::read_xml(filename, tree);
  std::stack<FullyQualifiedItem> items;
  for ( auto & x : tree ) items . push ( x );
  bool firstthing = true;
  while ( not items . empty () ) {
    FullyQualifiedItem item = items . top ();
    items . pop ();
    if ( queries . empty () || checkSelector ( item . name, queries ) ) {
      //std::cout << item << " : \"" << item.payload () << "\"\n";
      result = item.payload ();
    }
    for ( auto & x : * item . node ) items . push ( FullyQualifiedItem ( item, x ) );
  }
  return result;
}

// memberdef
//   extract specific data
struct memberdef {
  std::string name;
  std::string file;
  std::string kind;
  int64_t start;
  int64_t end;
  int init;
  memberdef ( Node const& tree )  {
    init = 0;
    std::stack<FullyQualifiedItem> items;
    for ( auto & x : tree ) items . push ( x );
    while ( not items . empty () ) {
      FullyQualifiedItem item = items . top ();
      items . pop ();
      if (item.shortname == "bodyend" ) {
        end = std::stoll(item.payload ());
        init |= 1;
      } 
      if (item.shortname == "bodystart" ) {
        start = std::stoll(item.payload ());
        init |= 2;
      }
      if ( item.shortname == "name" ) {
        name = item.payload ();
        init |= 4;
      }
      if ( item.shortname == "bodyfile" ) {
        file = item.payload ();
        init |= 8;
      }
      if ( item.shortname == "kind" ) {
        kind = item.payload ();
        init |= 16;
      }
      for ( auto & x : * item . node ) items . push ( FullyQualifiedItem ( item, x ) );
    }
  }
  uint64_t size ( void ) const {
    if ( end > start ) return end - start;
    return 1;
  }
  bool valid ( void ) const {
    return init == 31 && kind == "function" && end != -1;
  }
};


int createJSON ( int argc, char * argv [] ) {
  uint64_t start, end;
  std::vector<std::string> filenames;
  std::unordered_set<std::string> queries;
  // memberdef.location.\<xmlattr\>.bodystart 
  // memberdef.location.\<xmlattr\>.bodyend 
  // memberdef.location.\<xmlattr\>.bodyfile 
  // memberdef.name
  queries . insert ( "memberdef.location.<xmlattr>.bodystart");
  queries . insert ( "memberdef.location.<xmlattr>.bodyend");
  queries . insert ( "memberdef.location.<xmlattr>.bodyfile");
  queries . insert ( "memberdef.name");

  for ( int i = 1; i < argc; ++ i ) {
    filenames . push_back ( argv[i] );
  }
  bool outerfirst = true;
  std::cout << "{\"name\": \"DSGRN\",\n";
  std::cout << " \"children\":[\n";
  for ( std::string const& filename : filenames ) {
    if ( outerfirst ) outerfirst = false; else std::cout << ",";
    std::cout << "  {\"name\":\"" << fetch ( filename, "compoundname" ) << "\",\n";
    std::cout << "   \"children\": [\n";
    Node tree;
    boost::property_tree::read_xml(filename, tree);
    std::stack<FullyQualifiedItem> items;
    for ( auto & x : tree ) items . push ( x );
    bool innerfirst = true;
    while ( not items . empty () ) {
      FullyQualifiedItem item = items . top ();
      items . pop ();
      if ( item . shortname == "memberdef" ) {
        memberdef md ( *item.node );
        if ( md . valid () ) {
          if (innerfirst) innerfirst = false; else std::cout << ",\n";
          std::cout << "{\"name\": \"" << md.name << "\",";
          std::cout << "\"start\": " << md.start << ",";
          std::cout << "\"end\": " << md.end << ",";
          std::cout << "\"size\": " << md.size() << ",";
          std::cout << "\"file\": \"" << md.file << "\"}";
        }
      }
      for ( auto & x : * item . node ) items . push ( FullyQualifiedItem ( item, x ) );
    }
    std::cout << "]}\n";
  }
  std::cout << "]}\n";

  return 0;
}

int query ( int argc, char * argv [] ) {
  std::string filename ( argv[1] );
  std::unordered_set<std::string> queries;
  for ( int i = 2; i < argc; ++ i ) {
    queries . insert ( argv[i] );
  }
  Node tree;
  boost::property_tree::read_xml(filename, tree);
  std::stack<FullyQualifiedItem> items;
  for ( auto & x : tree ) items . push ( x );
  bool firstthing = true;
  while ( not items . empty () ) {
    FullyQualifiedItem item = items . top ();
    items . pop ();
    if ( queries . empty () || checkSelector ( item . name, queries ) ) {
      std::cout << item << " : \"" << item.payload () << "\"\n";
    }
    for ( auto & x : * item . node ) items . push ( FullyQualifiedItem ( item, x ) );
  }
  return 0;
}

int main ( int argc, char * argv [] ) {
  if ( argc < 2 ) {
    std::cout << "Help: \n"
                 " create [list-of-files] \n"
                 "   Create a json from doxygen of class_etc.xml files. Hint: use find | grep | xargs\n"
                 " query [filename] [list-of-dot-separated-strings]\n"
                 "   Search xml file for any entry containing any of the selector strings. \n"
                 "   dot separators correspond to drilling down xml levels.\n";
    return 0;
  }
  std::string command = argv[1];
  if ( command == "create" ) {
    createJSON ( argc - 1, argv + 1 );
    return 0;
  }
  if ( command == "query" ) {
    query ( argc - 1, argv + 1 );
    return 0;
  }
  std::cout << "Unrecognized command " << command << ".\n Run with no arguments for help.\n";
  return 1;
}
