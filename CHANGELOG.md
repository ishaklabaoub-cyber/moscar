### v0.2 (unreleased)

## Changes:

# TASK 1 DONE:
# Add a new counter "error_count" in order to always add up in the summary , it gets incremented when a socket() failed or the _UNDEFINED and _ERROR cases returned from port_state() so the ports counters always add up.


# TASK 2 DONE:
# Separate the project into headers (parsing.h(parsing funcs), utils.h(config structs-prog state-and some funcs), options.h, scan.h) implementation files (parsing.c, utils.c, options.c, scan.c)

# Did a bitmap and its functions and implemented it to the project.
# ADDED a list option to the port option 


# added a timeout function in the option.c and added int timeout in struct config to use it in the scan_port function in scan.c

# added a version displaying option that displays moscar's version 
# the open_ports array now is taking valid data (call the json option function after the connection end to print open_ports array after it gathered data).
