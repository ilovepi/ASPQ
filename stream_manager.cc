#include <click/config.h>
#include "stream_manager.hh"

CLICK_DECLS

StreamManager::StreamManager() {}
StreamManager::~StreamManager() {}

int StreamManager::initialize(ErrorHandler* errh) { return 0; }

int StreamManager::configure(Vector<String>& conf, ErrorHandler* errh)
{
    String channel;
    // errh->message("My int = %u", 22);
    if (Args(conf, this, errh).read("CHANNEL", WordArg(), channel).complete() <
        0)
    {
        return -1;
    }

    _errh = router()->chatter_channel(channel);
    //_errh = errh;
    return 0;
}

// Packet* StreamManager::simple_action(Packet* p) {}

void StreamManager::push(int port, Packet* p)
{
    p = handle_packet(port, p);
    if (p)
        output(0).push(p);
}

Packet* StreamManager::pull(int port)
{
    Packet* p = input(port).pull();
    if (p)
        p = handle_packet(0, p);
    return p;
}

/**
 * @brief Handles packets comming in on each port
 * @details [long description]
 *
 * @param port click port number
 * @param p an IP/TCP packet
 *
 * @return returns the packet to exit the elements output
 */
Packet* StreamManager::handle_packet(int port, Packet* p)
{
    switch (port)
    {
    // add stream
    case 0:
        p = add_stream(p);
        break;

    // update timers
    case 1:
        p = update_stream(p);
        break;

    case 2:
        p = remove_stream(p);
        break;

    // remove stream
    // case 3:
    // p = update_ack();
    //  break;

    // not one of our ports, so do nothing, and pass the packet on.
    default:
        break;
    };

    return p;
}

/**
 * @brief adds a stream to the table, with timers
 * @details uses a flow id to identify streams, and properly add them to the
 * hash table
 *
 * @param p pointer to a click packet
 * @return returns a packet to be output, identical to input
 */
Packet* StreamManager::add_stream(Packet* p)
{
    const click_ip* iph = p->ip_header();
    if (p->length() < 40 || iph->ip_p != IPPROTO_TCP)
    {
        //   DEBUG_CHATTER("Non TCP");
        // ignore non-TCP traffic
        return p;
    }

    const click_tcp* tcph = p->tcp_header();
    // int header_len = (iph->ip_hl << 2) + (tcph->th_off << 2);
    // int datalen = p->length() - header_len;

    // extract flow id
    //IPFlowID id(iph->ip_src.s_addr, tcph->th_sport, iph->ip_dst.s_addr, tcph->th_dport);

	IPFlowID id(p,false);

	stream_data::cb_data cb;
	cb.ptr =NULL;
	cb.el = this;
    // assign the stream to the hashtable
    hash[id] = stream_data(p, tcph->th_seq, tcph->th_ack, &cb);

    //hash[id].zero_wnd.initialize(this);

    return p;
}

Packet* StreamManager::remove_stream(Packet* p)
{
    const click_ip* iph = p->ip_header();
    if (p->length() < 40 || iph->ip_p != IPPROTO_TCP)
    {
        // DEBUG_CHATTER("Non TCP");
        // ignore non-TCP traffic
        return p;
    }

    const click_tcp* tcph = p->tcp_header();

    // extract flow id
    IPFlowID id(iph->ip_src.s_addr, tcph->th_sport, iph->ip_dst.s_addr,
                tcph->th_dport);


    if(hash.erase(id) ==0)
    	hash.erase(id.reverse());

    return p;
}

Packet* StreamManager::update_stream(Packet* p)
{
    const click_ip* iph = p->ip_header();
    if (p->length() < 40 || iph->ip_p != IPPROTO_TCP)
    {
        // DEBUG_CHATTER("Non TCP");
        // ignore non-TCP traffic
        return p;
    }

    const click_tcp* tcph = p->tcp_header();

    // extract flow id
    //IPFlowID id(iph->ip_src.s_addr, tcph->th_sport, iph->ip_dst.s_addr,tcph->th_dport);

    IPFlowID id(p,false);

    // lock the hash??
    HashTable_iterator<Pair<const IPFlowID, stream_data> > it = hash.find(id);
    if (it != hash.end())
    {
        it->second.reset_timers();
    }
    else
    {
        if ((it = hash.find(id.reverse())) != hash.end())
        {
            if (it->second.seq > tcph->th_seq)
                it->second.seq = tcph->th_seq;

            if (it->second.ack > tcph->th_ack)
                it->second.ack = tcph->th_ack;

            //if the persist timer has expired, we've already sent
            //0 WND notification. Unfreeze TCP with tri-ACK(maybe only send 2 here?)
            if(it->second.frozen)
            {
                output(0).push(p->clone());
                output(0).push(p->clone());
                output(0).push(p->clone());
                it->second.frozen = false;
            }
        }
    }
    // unlock the hash??

    return p;
}

// Packet* StreamManager::check_ACK(Packet* p) {}
CLICK_ENDDECLS
EXPORT_ELEMENT(StreamManager)
