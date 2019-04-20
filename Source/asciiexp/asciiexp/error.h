#pragma once

#define ERRORBOX( fmt, ... )					{ ErrorBox( fmt, ##__VA_ARGS__ ); }
#define WRITEFILEFAST( filepath, data, size )	{ WriteFileFast( filepath, data, size ); }

void ErrorBox( const char * fmt, ... );
void WriteFileFast( const char * filepath, void * data, size_t size );