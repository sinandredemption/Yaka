#ifndef INC_MISC_H_
#define INC_MISC_H_
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
namespace Misc {
  void split_string(const std::string &str_, std::vector<std::string> * v);
  std::ostream& hash(std::ostream& os);
  std::ostream& unhash(std::ostream& os);
  //template <typename T> T convert_to(const std::string& str);
  template <typename T>
  T convert_to(const std::string& str)
  {
    T temp;
    std::stringstream ss(str);
    ss >> temp;
    return temp;
  }
}
#endif
