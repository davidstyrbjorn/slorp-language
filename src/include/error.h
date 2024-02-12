#ifndef slorp_error_h
#define slorp_error_h

/// Some public helper functions for logging error!
void errorAtCurrentToken(const char *message);
void errorAtPreviousToken(const char *message);

#endif