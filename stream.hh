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
#include <cstdio>

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

    // default constructor
    stream_data()
        : p(0), seq(0), ack(0), persist_timer(200), val(1500), frozen(false),update_zwa(true)
    {
        cb_ptrs.ptr=this;
        //zero_wnd.assign(send_zero_wnd, this);
    }

    // regular constructor
/*    stream_data(const Packet* p_in, tcp_seq_t seq_in, tcp_seq_t ack_in,
                void* data)
        : cb_ptrs(data), p(0), seq(seq_in), ack(ack_in), persist_timer(5),
          val(1.5), zero_wnd(callback, &cb_ptrs), frozen(false)
    {
        cb_ptrs.ptr = this;
        create_zero_wnd(p_in);
        // assert(p!=NULL);
        zero_wnd.initialize(cb_ptrs.el->router());
        zero_wnd.schedule_after_sec(persist_timer);
    }
*/
    stream_data(const Packet* p_in, tcp_seq_t seq_in, tcp_seq_t ack_in, Element* el)
        : p(0), seq(seq_in), ack(ack_in), persist_timer(200),
          val(1500), zero_wnd(callback, &cb_ptrs), frozen(false),update_zwa(true)
    {
        cb_ptrs.el = el;
        cb_ptrs.ptr = this;
        create_zero_wnd(p_in);
         assert(p!=NULL);
        zero_wnd.initialize(cb_ptrs.el->router());
        zero_wnd.schedule_after_ms(persist_timer);
    }

    // copy constructor

    stream_data(const stream_data& other)
        : cb_ptrs(other.cb_ptrs), seq(other.seq), ack(other.ack),
          persist_timer(other.persist_timer), val(other.val),
          zero_wnd(callback, &cb_ptrs), frozen(other.frozen),update_zwa(other.update_zwa)
    {
        cb_ptrs.ptr=this;
        p = other.p->clone()->uniqueify();
        zero_wnd.initialize(other.zero_wnd.element());
        zero_wnd.schedule_at(other.zero_wnd.expiry());
    }

    // stream_data(const stream_data& other) { *this = other; }

    stream_data& operator=(const stream_data& other)
    {
        if (this != &other)
        {
            assert(other.p != NULL);
            cb_ptrs.ptr = this;
            cb_ptrs.el = other.cb_ptrs.el;
            if (other.p)
            {
                p = other.p->clone()->uniqueify();
            }
            assert(p != NULL);
            seq = other.seq;
            ack = other.ack;
            persist_timer = other.persist_timer;
            val = other.val;
            zero_wnd.initialize(other.zero_wnd.element());
            zero_wnd.assign(callback, &cb_ptrs);
            zero_wnd.schedule_at(other.zero_wnd.expiry());
            frozen = other.frozen;
            update_zwa = other.update_zwa;
        }

        return *this;
    }

    ~stream_data()
    {
        if (p)
            p->kill();
        zero_wnd.clear();
        printf("Stream_Data Destructor\n");
    }

    static void callback(Timer* timer, void* data)
    {
        cb_data* cb = (cb_data*)data;
        assert(data != NULL);
        assert(timer != NULL);
        assert(cb != NULL);
        assert(cb->ptr != NULL);
        assert(cb->el != NULL);
        cb->ptr->send_zero_wnd(timer, cb->el);
    }

    void send_zero_wnd(Timer* timer, void* data)
    {
        //Element* el = (Element*)data;
/*        click_tcp* tcp = p->tcp_header();
        tcp->th_ack = ack;
        tcp->th_seq = seq;
        */
        // click_update_in_cksum

        // output 1 goes to a tcp checksum element followed by ipcsum
        // element before normal routing
        //el->output(1).push(p->clone()); // maybe wrong!!!!!!!!

        timer->reschedule_after_ms(update_persist_timer());
    }

    void send_zero_wnd()
    {
        //click_tcp* tcp = p->tcp_header();
        //tcp->th_ack = ack;
        //tcp->th_seq = seq;
        // click_update_in_cksum
//        printf("My ack: %ld\tMy seq: %ld\tACK: %ld\t SEQ: %ld\n",  ntohl(ack), ntohl(seq), ntohl(tcp->th_ack), ntohl(tcp->th_seq));
        // output 1 goes to a tcp checksum element followed by ipcsum
        // element before normal routing

        //for(int i = 0; i < 3; ++i)
            cb_ptrs.el->output(1).push(p->clone()->uniqueify()); // maybe wrong!!!!!!!!

        zero_wnd.reschedule_after_ms(update_persist_timer());
    }

    void send_zero_wnd(Packet* p_in)
    {
        update_zwa = false;
        const click_tcp* tcph = p_in->tcp_header();
        click_tcp* zwah = p->tcp_header();

        zwah->th_ack =  htonl(ntohl(tcph->th_seq)+1);
        cb_ptrs.el->output(1).push(p->clone()->uniqueify());
        zero_wnd.reschedule_after_ms(update_persist_timer());

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
        char data[66] = {0};
  //      assert(p == NULL);
        if (p == NULL)
        {

            // assert(sizeof(data) == 128);
            p = Packet::make(14, data, sizeof(data), 0);
        }
        assert(p != NULL);
        assert(sizeof(*p) >= sizeof(data));

        p->set_network_header(p->data(), 20);
        const click_ip* ip_in = p_in->ip_header();
        click_ip* ip_out = p->ip_header();
        const click_tcp* tcp_in = p_in->tcp_header();
        click_tcp* tcp_out = p->tcp_header();

        ip_out->ip_src = ip_in->ip_dst;
        ip_out->ip_dst = ip_in->ip_src;
        // ip_out->ip_tos = 0;
        ip_out->ip_hl=5;
        ip_out->ip_len = htons(sizeof(data));
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
        if (val < 60000)
            val = val * 2;
/*
        if (val < 5000)
        {
            persist_timer = 5000;
        }
        else if (val >= 60000)
        {
            persist_timer = 60000;
        }
        else
        {
            // persist_timer = (uint32_t)floor(val);
            persist_timer = (uint32_t)val;
        }
        */
        if(val >= 60000)
            persist_timer = 60000;
        else
            persist_timer = (uint32_t)val;
        return persist_timer; // remove 200 ms from timer to beat
    }

    /**
     * @brief resets the persist timer to its initial value of 5 sec & val to
     * 1.5
     * @details [long description]
     */
    void reset_timers()
    {
        frozen = false;
        persist_timer = 200;
        val = 1500;
        update_zwa = true;
        zero_wnd.schedule_after_ms(persist_timer);
    }

    void unfreeze()
    {
        reset_timers();
        WritablePacket* thaw = p->clone()->uniqueify();
        click_tcp* tcph = thaw->tcp_header();
        tcph->th_win = last_window;
        for(int i = 0; i < 3; ++i)
            cb_ptrs.el->output(1).push(thaw);
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
    bool update_zwa;
    uint32_t last_window;

}; // end class stream

#endif // end of @file stream.hh
