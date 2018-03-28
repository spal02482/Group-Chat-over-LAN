# Group Chat over LAN

This is a Chat application used for communication with all the clients
present over LAN made by using *socket programming* in **C/C++** . A message sent by a client ( or server  ) is broadcasted 
to all the clients in the LAN.

## Usage

 - Compile the server and client source code using  
    	`$ g++ gc_server.cpp -o gc_server`  
	`$ g++ gc_client.cpp -o gc_client`
 
 - Run the server program on the computer chosen to be server.  
	`$ ./gc_server server_ip server_port`
 
 - Run the client program on the client computer  
	`$ ./gc_client server_ip server_port`
 
> As soon as the connection is done, server and all other existing clients will be notified that a new client has joined the connection.
  
  From now on any message sent will be received by all the other computers
  along with the IP of the sender.

## What I used :

 - `getaddrinfo()` to get the linked list of addrinfo structures.
 - used *I/O multiplexing* i.e. `poll()` to handle updates on various file descriptors, such as taking input, receiving incoming connections and
 receiving messages from clients.
 - `C++ vector` to manage the addresses of the clients.

