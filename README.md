# IPv4 WAN address checker

This is a small daemon program that checks if the WAN address has changed.
It is used with another program that updates DNS records on a remote server.
Its purpose is to reduce the load on the remote server.

It does the following:

1. Creates a server listening via a TCP/IPv4 socket on port 65535.
2. Executes the program passed as the first argument.
3. Obtains the current address by reading the output of the program.
   The program must exit with 0 and write only the address to `stdout`.
4. Connects to the server using the address and sends itself a nonce.
5. If the nonce is not received, it is assummed the address has changed;
   the program jumps to step 2.
6. Waits for 10 minutes and jumps to step 4.

## License

The program is released into the public domain.
