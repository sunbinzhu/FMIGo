#include "Logger.h"
#include "stdio.h"
#include <string>
#include <stdlib.h>

using namespace fmitcp;

Logger::Logger(){
    //default to only logging errors
    m_filter = LOG_ERROR;
    m_prefix = "";
}

Logger::~Logger(){

}

void Logger::log(Logger::LogMessageType type, const char * format, ...){
    if (type < m_filter) {
        //need to do this because reasons
        va_list args;
        va_start(args, format);
        va_end(args);
        fflush(NULL);
        return;
    }
    // Print prefix
    if(m_prefix != ""){
        fprintf(stderr, "%s",m_prefix.c_str());
    }

    // Print message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fflush(NULL);
}

void Logger::setFilter(int filter){
    m_filter = filter;
}

int Logger::getFilter() const {
    return m_filter;
}

void Logger::setPrefix(std::string prefix){
    m_prefix = prefix;
}
