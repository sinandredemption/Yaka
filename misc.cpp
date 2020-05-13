#include "misc.h"

namespace Misc {
  void split_string(const std::string &str_, std::vector<std::string> * v)
  {
    std::istringstream ss(str_, std::stringstream::in);
    std::string str;
    while (ss >> str) v->push_back(str);
  }

  // Prepare printing a 64-bit key, in hex
  std::ostream& hash(std::ostream& os)
  {
    os.fill('0');
    os.width(16);
    os << std::hex << std::uppercase;
    return os;
  }

  // Used after hash()
  std::ostream& unhash(std::ostream& os)
  {
    os << std::dec << std::nouppercase;
    return os;
  }

  //template <typename T>
  /*T convert_to(const std::string& str)
  {
  T temp;
  std::stringstream ss(str);
  ss >> temp;
  return temp;
  }*/
}