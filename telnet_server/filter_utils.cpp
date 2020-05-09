#include "filter_utils.hpp"

#include <boost/process/child.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <iostream>
#include <mutex>

#include <signal.h>

void filter_r(std::string& buf, std::size_t& str_size)
{
    if(str_size == 0)
        return;
    std::string::size_type pos;
    while((pos = buf.find_first_of('\r')) != std::string::npos){
        buf.erase(pos, 1);
        --str_size;
    }
}

void add_r(std::string& buf, std::size_t& str_size)
{
    std::size_t str_end = str_size;
    if(str_size == 0)
        return;
    std::string::size_type pos = 0;
    while((pos < str_end) && (pos = buf.find_first_of('\n', pos)) != std::string::npos){
        buf.replace(pos, 1, "\r\n");
        ++str_end;
        pos+=2;
    }
    str_size = str_end;
}
