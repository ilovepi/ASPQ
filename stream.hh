#ifndef CLICK_MY_STREAM_HH
#define CLICK_MY_STREAM_HH

#include <click/config.h>
#include <click/element.hh>
#include <click/packet.hh>
#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <click/hashtable.hh>
#include <click/sync.hh>
#include <click/timer.hh>
//#include <cmath>
//#include <memory>
//#include <mutex>

class stream_data
{
  public:
    struct cb_data
    {
        stream_data* ptr;
        Element* el;
        cb_data() : ptr(NULL), el(NULL){};
        cb_data(void* ptr_in)
        {
            ptr = ((cb_data*)ptr_in)->ptr;
            el = ((cb_data*)ptr_in)->el;
        }
    };

    stream_data()
        : p(0), seq(0), ack(0), persist_timer(5), val(1.5), frozen(false)
    {
        // zero_wnd.assign(send_zero_wnd, this);
    }

    stream_data(const Packet* p_in, tcp_seq_t seq_in, tcp_seq_t ack_in,
                void* data)
        : cb_ptrs(data), p(0), seq(seq_in), ack(ack_in), persist_timer(5),
          val(1.5), zero_wnd(callback, &cb_ptrs), frozen(false)
    {
        create_zero_wnd(p_in);
        zero_wnd.initialize(cb_ptrs.el->router());
        zero_wnd.schedule_after_sec(persist_timer);
    }

    stream_data(const stream_data& other)
        : cb_ptrs(other.cb_ptrs), seq(other.seq), ack(other.ack),
          persist_timer(other.persist_timer), val(other.val),
          zero_wnd(callback, &cb_ptrs), frozen(false)
    {

        p = other.p->uniqueify();

        zero_wnd.initialize(other.zero_wnd.element());
        zero_wnd.schedule_at(other.zero_wnd.expiry());
    }

    stream_data& operator=(const stream_data& other)
    {
        if (this != &other)
        {
            cb_ptrs = other.cb_ptrs;
            p = other.p->uniqueify();
            seq = other.seq;
            ack = other.ack;
            persist_timer = other.persist_timer;
            val = other.val;
            zero_wnd.initialize(other.zero_wnd.element());
            zero_wnd.assign(callback, &cb_ptrs);
            zero_wnd.schedule_at(other.zero_wnd.expiry());
            frozen = other.frozen;
        }

        return *this;
    }

    ~stream_data()
    {
        if (p)
            p->kill();
    }

    static void callback(Timer* timer, void* data)
    {
        cb_data* cb = (cb_data*)data;
        cb->ptr->send_zero_wnd(timer, cb->el);
    }

    void send_zero_wnd(Timer* timer, void* data)
    {
        Element* el = (Element*)data;
        click_tcp* tcp = p->uniqueify()->tcp_header();
        tcp->th_ack = ack;
        tcp->th_seq = seq;
        // click_update_in_cksum

        // output 1 goes to a tcp checksum element followed by ipcsum
        // element before normal routing
        el->output(1).push(p->clone()); // maybe wrong!!!!!!!!

        timer->reschedule_after_sec(update_persist_timer());
    }

    /**
     * @brief create a zero window packet to be sent with the callback timer
     * @details create a zero window packet to be sent with the call back timer.
     * will be updated with seq and ack numbers
     *
     * @param p_in the packet to extract the source and dest infor from
     * @return returns a pointer to the newly created packet
     */
    Packet* create_zero_wnd(const Packet* p_in)
    {
        char data[128] = {0};

        p = Packet::make(data, sizeof(data));
        assert(sizeof(*p) >= 128);
        if (p)
        {
            p->set_network_header(p->data(), 20);
            const click_ip* ip_in = p_in->ip_header();
            click_ip* ip_out = p->ip_header();
            const click_tcp* tcp_in = p_in->tcp_header();
            click_tcp* tcp_out = p->tcp_header();

            ip_out->ip_src = ip_in->ip_dst;
            ip_out->ip_dst = ip_in->ip_src;
            // ip_out->ip_tos = 0;
            ip_out->ip_len = htons(60);
            ip_out->ip_id = ip_in->ip_id;
            // ip_out->ip_csum = 0;
            ip_out->ip_ttl = 255;
            ip_out->ip_p = ip_in->ip_p;

            tcp_out->th_dport = tcp_in->th_sport;
            tcp_out->th_sport = tcp_in->th_dport;
            // tcp_out->th_seq = 0;
            // tcp_out->th_ack = 0;
            tcp_out->th_off = 5;
            // tcp_out->th_flags = 0;
            tcp_out->th_flags |= TH_ACK;
            // tcp_out->tcp_win = 0;
            // tcp_out->th_sum = 0;
        }
        return p;
    }

    /**
     * @brief updates the persist timer for the zero window response based on
     * standard behavior on linux
     *
     * @details updates the persist timer for the zero window response based on
     * standard behavior on linux.
     * because our tool doesn't search for zero window probes(perhaps a good
     * idea for future implementations)
     * it simply relies on the timer to prempt the sender as best it can.
     *
     * @return the value of the persist_timer
     */
    uint32_t update_persist_timer()
    {
        frozen = true;

        // we're limiting val to 1.5,3,6,12,24,48,60,60,60...
        // we limit the persist_timer to 5,5,6,12,24,48,60,60,60...
        if (val < 60)
            val = val * 2;

        if (val < 5)
        {
            persist_timer = 5;
        }
        else if (val >= 60)
        {
            persist_timer = 60;
        }
        else
        {
            // persist_timer = (uint32_t)floor(val);
            persist_timer = (uint32_t)val;
        }
        return persist_timer;
    }

    /**
     * @brief resets the persist timer to its initial value of 5 sec & val to
     * 1.5
     * @details [long description]
     */
    void reset_timers()
    {
        frozen = false;
        persist_timer = 5;
        val = 1.5;
    }

    // Timer keepalive;
    cb_data cb_ptrs;
    WritablePacket* p; // consider making a smartpointer
    tcp_seq_t seq;
    tcp_seq_t ack;
    uint32_t persist_timer;
    float val;
    Timer zero_wnd;
    bool frozen;

}; // end class stream

#if 0
typedef std::shared_ptr<HashTable<stream_id, stream_data>> hash_ptr;
hash_ptr tbl_ptr;
SimpleSpinlock tbl_lock;

hash_ptr make_stream_table()
{
	std::lock_guard<SimpleSpinlock> lock(tbl_lock);
	if (!tbl_ptr)
		tbl_ptr = std::make_shared<hash_ptr>();
	return tbl_ptr;
}

void release_global_hash()
{
	std::lock_guard<SimpleSpinlock> lock(tbl_lock);
	if(tbl_ptr)
		tbl_ptr.reset();
}

#endif

#endif