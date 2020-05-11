#ifndef FILTER_UTILS_H
#define FILTER_UTILS_H

#include <string>
#include <boost/process/child.hpp>

namespace code_bytes{
    constexpr char SE = -16; //0xF0
    constexpr char NOP = -15; //0xF1
    constexpr char DM = -14; //0xF2
    constexpr char BRK = -13; //0xF3
    constexpr char IP = -12; //0xF4
    constexpr char AO = -11; //0xF5
    constexpr char AYT = -10; //0xF6
    constexpr char EC = -9; //0xF7
    constexpr char EL = -8; //0xF8
    constexpr char GA = -7; //0xF9
    constexpr char SB = -6; //0xFA
    constexpr char WILL = -5; //0xFB
    constexpr char WONT = -4; //0xFC
    constexpr char DO = -3; //0xFD
    constexpr char DONT = -2; //0xFE
    constexpr char IAC = -1; //0xFF
}

//filter all carriage return (CR) symbols
void filter_r(std::string& buf, std::size_t& str_size);

//replace line feed (LF) with CR LF
void add_r(std::string& buf, std::size_t& str_size);

#endif // FILTER_UTILS_H
