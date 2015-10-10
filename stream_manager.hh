#ifndef CLICK_STREAM_MANAGER_HH
#define CLICK_STREAM_MANAGER_HH

#include <click/config.h>
#include <click/element.hh>
#include <click/confparse.hh>
#include <click/packet.hh>
#include <click/error.hh>
#include <click/router.hh>
#include <click/args.hh>
#include <click/ipflowid.hh>
#include "stream.hh"

CLICK_DECLS

/**
 * @brief A click element that manages streams entering an SPQ router
 * @details StreamManager monitors tcp streams coming from an ipclassifier or
 * IPfilter element. It expects specific types ofinputs on particular ports, and
 * outputs to specific elements
 *
 * Inputs:
 * Port 0: Adds new streams
 * Port 1: updates existing streams
 * Port 2: removes streams
 * Port 3: do nothing, pass packet out
 *
 * Outputs:
 * Port 0: goes to normal routing, incoming packets exit here normally
 * Port 1: goes through TCP and IP checksum elements prior to normal routing
 * It is used primarily(exclusively) for sending the generated Zero Window
 * notifications used to "freeze" the TCP connection.
 *
 */
class StreamManager : public Element
{
  public:
    StreamManager();
    ~StreamManager();

    const char* class_name() const { return "StreamManager"; }
    const char* port_count() const { return "5/3"; }
    const char* processing() const { return "h/h"; }

    int initialize(ErrorHandler* errh);
    int configure(Vector<String>& conf, ErrorHandler* errh);

    void push(int port, Packet* p);
   // Packet* pull(int port);

    Packet* handle_packet(int port, Packet* p);
    Packet* add_stream(Packet* p);
    Packet* remove_stream(Packet* p);
    Packet* update_stream(Packet* p);
    Packet* update_ack(Packet* p);
    Packet* hp_streams(Packet* p);


  private:
    ErrorHandler* _errh;
    HashTable<IPFlowID, stream_data> hash;
    HashTable<FlowID, int> hp_hash;
    SimpleSpinlock tbl_lock;
    SimpleSpinlock hp_lock;
    bool frozen;
};

CLICK_ENDDECLS
#endif // end of @file stream_manager.hh
