#pragma once

#include "bench.hh"

constexpr const unsigned int CLIENT_TYPE = 1;
constexpr const unsigned int SERVER_TYPE = 2;

// Follows message specification: https://man7.org/linux/man-pages/man3/msgrcv.3p.html
// ```
// struct mymsg {
//     long    mtype;     /* Message type. */
//     char    mtext[1];  /* Message text. */
// };
// ```
struct Msgbuf
{
    long mtype;
    // Flexible array member
    // https://www.ibm.com/docs/en/i/7.4?topic=declarations-flexible-array-members
    char buffer[];
};

// Smart pointer managing the memory of a Msgbuf object
class MsgbufRAII
{
private:
    Msgbuf *msgbuf;
    size_t size;

public:
    MsgbufRAII(size_t message_size, long mtype) : size(message_size)
    {
        // `sizeof(Msgbuf)` evaluates to `sizeof(long)` which is the size of the `mtype` field
        msgbuf = static_cast<Msgbuf *>(std::malloc(sizeof(Msgbuf) + message_size * sizeof(char)));
        if (!msgbuf)
        {
            throw std::bad_alloc();
        }
        msgbuf->mtype = mtype;
        std::cout << "Msg buf type: " << msgbuf->mtype << std::endl;
        memset(msgbuf->buffer, '1', message_size);
    }

    ~MsgbufRAII()
    {
        std::free(msgbuf);
    }

    Msgbuf *data_ptr()
    {
        return msgbuf;
    }

    size_t get_len()
    {
        return size;
    }

    // Message data must be `size` bytes long, otherwise the data will be truncated
    // or garbage data will be read
    void set_data(const char *data)
    {
        std::strncpy(msgbuf->buffer, data, size);
    }
};