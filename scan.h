#ifndef SCAN_H
#define SCAN_H


enum port_state {
    _OPEN      = 01,            // when an open port gets scanned
    _CLOSED    = 02,            // when a closed port gets scanned 
    _FILTERED  = 04,            // when a filtered port gets scanned
    _UNREACHABLE = 16,          // for unreachable networks    
    _ERROR     = 32,            // when error occurred
    _UNDEFINED = 64             // undefined state, beyond(ENETUNREACH, ETIMEDOUT, ECONNREFUSED)
};

enum port_state scan_port(int , const char *, long);       // USED  FOR SCANNING PORTS

#endif
