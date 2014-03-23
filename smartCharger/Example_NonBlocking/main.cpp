#define MAXCLIENTS 10
main()
{
    filesender_t filesenders[MAXCLIENTS];
    int n_filesenders;
    int listenfd;
    struct sockaddr_in servaddr;

    /* Set up to be a daemon listening on port 8000 */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port   = htons(8000);
    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    /* Force the network socket into nonblocking mode */
    setNonblocking(listenfd);

    /* Wait for connections, and send anyone who connects 
     * the contents of foo.txt
     */
    n_filesenders = 0;
    do {
        /* If we don't have a full set of clients already,
         * listen for a new connection.
         */
        if (n_filesenders < MAXCLIENTS) {
            int fd = accept(listenfd, NULL, NULL);
            if (fd != -1) {
                /* Someone connected.  Send them the file */
                filesenders[n_filesenders].sendFile("foo.txt", fd);
                n_filesenders++;
            }
        }
        /* Pump data out to all the connected clients */
        for (int i=0; i<n_filesenders; i++)
            filesenders[i].handle_io();
    }
}
