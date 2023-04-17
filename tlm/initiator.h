#ifndef INITIATOR_H
#define INITIATOR_H

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;
#include <sysc/datatypes/fx/sc_ufixed.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include <cstdint>
#include "map.h"
const float x_input_signal[8*16]={0.500,   0.525,   0.549,   0.574,   0.598,   0.622,   0.646,   0.670,
                            0.693,   0.715,   0.737,   0.759,   0.780,   0.800,   0.819,   0.838,
                            0.856,   0.873,   0.889,   0.904,   0.918,   0.931,   0.943,   0.954,
                            0.964,   0.972,   0.980,   0.986,   0.991,   0.995,   0.998,   1.000,
                            1.000,   0.999,   0.997,   0.994,   0.989,   0.983,   0.976,   0.968,
                            0.959,   0.949,   0.937,   0.925,   0.911,   0.896,   0.881,   0.864,
                            0.847,   0.829,   0.810,   0.790,   0.769,   0.748,   0.726,   0.704,
                            0.681,   0.658,   0.634,   0.610,   0.586,   0.562,   0.537,   0.512,
                            0.488,   0.463,   0.438,   0.414,   0.390,   0.366,   0.342,   0.319,
                            0.296,   0.274,   0.252,   0.231,   0.210,   0.190,   0.171,   0.153,
                            0.136,   0.119,   0.104,   0.089,   0.075,   0.063,   0.051,   0.041,
                            0.032,   0.024,   0.017,   0.011,   0.006,   0.003,   0.001,   0.000,
                            0.000,   0.002,   0.005,   0.009,   0.014,   0.020,   0.028,   0.036,
                            0.046,   0.057,   0.069,   0.082,   0.096,   0.111,   0.127,   0.144,
                            0.162,   0.181,   0.200,   0.220,   0.241,   0.263,   0.285,   0.307,
                            0.330,   0.354,   0.378,   0.402,   0.426,   0.451,   0.475,   0.500};
float y_downsample_by2[64]={0.429,   0.558,   0.606,   0.654,   0.700,   0.744,   0.786,   0.825,
                               0.861,   0.894,   0.922,   0.946,   0.966,   0.982,   0.993,   0.998,
                               0.999,   0.996,   0.987,   0.973,   0.955,   0.933,   0.906,   0.875,
                               0.841,   0.803,   0.762,   0.719,   0.673,   0.626,   0.578,   0.529,
                               0.479,   0.430,   0.382,   0.334,   0.289,   0.245,   0.204,   0.165,
                               0.130,   0.099,   0.071,   0.048,   0.029,   0.015,   0.006,   0.001,
                               0.001,   0.006,   0.016,   0.031,   0.050,   0.074,   0.101,   0.133,
                               0.168,   0.207,   0.248,   0.292,   0.338,   0.386,   0.434,   0.484};
float h_k[3]={0.5,1/3,1/6};
// Initiator module generating generic payload transactions
SC_MODULE(Initiator)
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<Initiator> socket;

  SC_CTOR(Initiator)
  : socket("socket")  // Construct and name socket
  {
    SC_THREAD(thread_process);
  }

  void thread_process()
  {
    // TLM-2 generic payload transaction, reused across calls to b_transport
    tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;
    sc_time delay = sc_time(10, SC_NS);

    // Generate two transaction of write and read
    
    for (int i = 0; i < 16*8; i++)
      for(int j=0;j<3;j++){
      {

        tlm::tlm_command cmd = tlm::TLM_WRITE_COMMAND; 
        //prepare 4 bytes (uint8_t)
        data[0]=x_input_signal[i*2-j+1];
        data[1]=h_k[j];
        data[2]=0;
        data[3]=0;

        // Prepare payload
        trans->set_command( cmd );
        trans->set_address( BASE_TARGET_INPUT_ADDR ); //P2P TLM write to target's address 0
        trans->set_data_ptr( reinterpret_cast<unsigned char*>(&data) );
        trans->set_data_length( 4 );
        trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
        trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
        trans->set_dmi_allowed( false ); // Mandatory initial value
        trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

        socket->b_transport( *trans, delay );  // Blocking transport call

        // Initiator obliged to check response status and delay
        if ( trans->is_response_error() )
          SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

        cout << "trans = " << (cmd ? 'W' : 'R')
            << ", data = " << +data[3] << +data[2]<< +data[1]<< +data[0]<< " at time " << sc_time_stamp()
            << " delay = " << delay << endl;

        // Realize the delay annotated onto the transport call
        wait(delay);

        //Read back results
        cmd = tlm::TLM_READ_COMMAND; 

        // Prepare payload
        trans->set_command( cmd );
        trans->set_address( BASE_TARGET_OUTPUT_ADDR ); //P2P TLM read from target's address 4
        trans->set_data_ptr( reinterpret_cast<unsigned char*>(&result) );
        trans->set_data_length( 4 );
        trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
        trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
        trans->set_dmi_allowed( false ); // Mandatory initial value
        trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

        socket->b_transport( *trans, delay );  // Blocking transport call

        // Initiator obliged to check response status and delay
        if ( trans->is_response_error() )
          SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

        cout << "trans = " << (cmd ? 'W' : 'R')
            << ", result = " << result << " at time " << sc_time_stamp()
            << " delay = " << delay << endl;

        // Realize the delay annotated onto the transport call
        wait(delay);
      }
    }
  }

  // Internal data buffer used by initiator with generic payload
  sc_ufixed_fast<4,1> data[4];
  sc_ufixed_fast<4,1> result;
};

#endif
