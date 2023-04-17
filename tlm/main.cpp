#include "initiator.h"
#include "target.h"
int sc_main(int argc, char* argv[])
{
  Initiator initiator("initiator");
  Adder   adder("adder");

  // Bind initiator socket to target socket
  initiator.socket.bind( adder.socket );
  sc_start();
  return 0;
}
