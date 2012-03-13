#include <stdio.h>
#include <string.h>
#include <sys/kernel.h>
#include <sys/name.h>

/**************************************************************************/

void main( int argc, char *argv[] )
{
char   buf[100];
pid_t  pid;

  if( argc < 2 ) {
    printf( "Syntax: %s name [c-connect]\n", argv[1] );
    return;
  }
  if( argc == 3 ) {
  // Client: Spoji se a posila zpravy s cekanim na odpoved.
    printf( "connect name %s - ", argv[1] );
    fflush( stdout );
    // nalezeni jmena
    if( (pid = qnx_name_locate( 0, argv[1], 0, NULL )) == -1 ) {
      printf( "ERR\n" );
      return;
    }
    printf( "OK\n" );
    while( 1 ) {
      printf( "Zadej zpravu: " );
      fflush( stdout );
      gets( buf );
      // poslani zpravy a cekani na odpoved
      if( Send( pid, buf, buf, strlen( buf ) + 1, sizeof( buf )) == -1 ) {

        perror( "Send error" );
        continue;
      }
      printf( "odpoved: %s\n", buf );
    }
  }
  //---------------------------------------------------------------------
  // Server: ceka na zpravu, splni ji a odpovi.
  printf( "attach name %s - ", argv[1] );
  fflush( stdout );
  if( qnx_name_attach( 0, argv[1] ) == -1 ) {  // jmenuji se 'argv[1]'
    printf( "ERR\n" );
    return;
  }
  printf( "OK\nreceiving...\n" );
  while( 1 ) {
    if( (pid = Receive( 0, buf, sizeof( buf ))) == -1 ) {  // cekam na
zpravu
      perror( "Receive error" );
      continue;
    }
    printf( "zprava: %s\n", buf );
    if( buf[0] != '#' ) {
      strupr( buf );
    } else {
      printf( "Zadej odpoved: " );
      fflush( stdout );
      gets( buf );
    }
    if( Reply( pid, buf, strlen( buf ) +1 ) == -1 ) {
      perror( "Reply error" );
    }
  }
}

