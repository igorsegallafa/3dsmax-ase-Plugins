#include "stdafx.h"
#include "error.h"

void ErrorBox( const char * fmt, ... )
{
	char text[2048];
	va_list ap;

	va_start( ap, fmt );
	vsprintf_s( text, _countof( text ), fmt, ap );
	va_end( ap );

	MessageBox( 0, GetWC(text), L"Error", MB_OK | MB_ICONEXCLAMATION );
}

void WriteFileFast( const char * filepath, void * data, size_t size )
{
	FILE * file;
	if( fopen_s( &file, filepath, "wb" ) == 0 )
	{
		fwrite( data, size, 1, file );
		fclose( file );
	}
}