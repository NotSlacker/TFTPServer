## TFTP Server

Here's an implementation of a server using TFTP for data exchange.
TFTP is a data transfer protocol that is written in accordance with RFC 1350, in which it is described.
The project is written in C for Linux.
The project's folder is used as a storage for server.
The native tftp client was used for testing.

## How to use

Open the project folder and use the following code to build and run the server on port 10118 (11111 by default) for example:

```sh
make
./tftps 10118
```

## Interaction example
  ![](https://github.com/NotSlacker/tftp-server/blob/master/tftp-project-interaction-example.gif)
