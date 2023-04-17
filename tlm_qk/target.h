#ifndef TARGET_H
#define TARGET_H

// Needed for the simple_target_socket
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <sysc/datatypes/fx/sc_ufixed.h>
#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <cstdint>
#include "map.h"


// TLM target adder function
SC_MODULE(Adder)
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_target_socket<Adder> socket;

  enum { SIZE = 4 };

  SC_CTOR(Adder)
  : socket("socket")
  {
    // Register callback for incoming b_transport interface method call
    socket.register_b_transport(this, &Adder::b_transport);
    
    // Initialize input and output data register
    for (int i = 0; i < SIZE; i++) i_data[i] =0;
    sum=0;
  }

  // TLM-2 blocking transport method
  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address();
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    // Check address range and check for unsupported features
    if (byt != 0 || len > 4 || wid < len)
      SC_REPORT_ERROR("TLM-2", "Target does not support given generic payload transaction");

    // Obliged to implement read and write commands
    if ( cmd == tlm::TLM_READ_COMMAND ){
      if(adr==BASE_TARGET_OUTPUT_ADDR){
        //Copy 4 bytes to ptr
        unsigned char a=sum.to_string(SC_BIN)
        for (unsigned int i = 0; i < len; i++) ptr[i] = a[i];
        delay=sc_time(5, SC_NS);
      }
      else{
        SC_REPORT_ERROR("TLM-2", "Address not supported for read operation.");
      }
    }
    else if ( cmd == tlm::TLM_WRITE_COMMAND ){
      if(adr==BASE_TARGET_INPUT_ADDR){
        //Copy 4 bytes from ptr
        for (unsigned int i = 0; i < len; i++) i_data[i] = ptr[i];
        //Compute summation with lower two uint8_t integers
        sum=i_data[0]+i_data[1];
        delay=sc_time(10, SC_NS);
      }
      else{
        SC_REPORT_ERROR("TLM-2", "Address not supported for write operation.");
      }
    }

    // Obliged to set response status to indicate successful completion
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }

  sc_ufixed_fast<4,1> i_data[SIZE];
  sc_ufixed_fast<4,1> sum;
};

#endif
