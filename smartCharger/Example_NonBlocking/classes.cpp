/*----------------------------------------------------------------------
 Portable function to set a socket into nonblocking mode.
 Calling this on a socket causes all future read() and write() calls on
 that socket to do only as much as they can immediately, and return 
 without waiting.
 If no data can be read or written, they return -1 and set errno
 to EAGAIN (or EWOULDBLOCK).
 Thanks to Bjorn Reese for this code.
----------------------------------------------------------------------*/
int setNonblocking(int fd)
{
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    /* Otherwise, use the old way of doing it */
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}     

#define BUFSIZE 1024
class filesender_t {
    int m_fd;               /* file being sent */
    char m_buf[BUFSIZE];    /* current chunk of file */
    int m_buf_len;          /* bytes in buffer */
    int m_buf_used;         /* bytes used so far; <= m_buf_len */
    enum { IDLE, SENDING } m_state; /* what we're doing */

public:
    filesender() {
        m_state = IDLE;     /* not doing anything initially */
    }

    /* Start sending a file on the given socket.
     * Set the socket to be nonblocking.
     * 'filename' is the name of the file to send.
     * 'socket' is an open network connection.
     */
    void sendFile(const char *filename, int socket)
    {
        int nread; 
        int nwrite, i;

        /* Force the network socket into nonblocking mode */
        setNonblocking(socket);

        /* Open the file */
        m_fd = open(filename, O_RDONLY);
        if (m_fd < 0)
            fatal_error("open failed");

        /* Start sending it */
        m_buf_len = 0;
        m_buf_used = 0;
        m_socket = socket;
        m_state = SENDING;
    }

    /* Continue sending the file started by sendFile().
     * Call this periodically.
     * Returns nonzero when done.
     */
    int handle_io()
    {
        if (m_state == IDLE)
            return 2;     /* nothing to do */

        /* If buffer empty, fill it */
        if (m_buf_used == m_buf_len) {
            /* Get one chunk of the file from disk */
            m_buf_len = read(m_fd, m_buf, BUFSIZE);
            if (m_buf_len == 0) {
                /* All done; close the file and the socket. */
                close(m_fd);
                close(m_socket);
                m_state = IDLE;
                return 1;
            }
            m_buf_used = 0;
        }

        /* Send one chunk of the file */
        assert(m_buf_len > m_buf_used);
        nwrite = write(m_socket, m_buf + m_buf_used, m_buf_len - m_buf_used);
        if (nwrite < 0) 
            fatal_error("write failed");
        m_buf_used += nwrite;
        return 0;
    }
}



