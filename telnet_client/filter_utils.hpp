#ifndef FILTER_UTILS_HPP
#define FILTER_UTILS_HPP

#include <string>

//filter all carriage return (CR) symbols
void filter_r(std::string& buf, std::size_t& str_size);

//replace line feed (LF) with CR LF
void add_r(std::string& buf, std::size_t& str_size);

#endif // FILTER_UTILS_HPP
