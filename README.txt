Chord - DHT README
===================================================
Akshay Rajput

===================================================

Instructions
-----------------
run 
1. make
2. ./dht

to run the dht. 

Commands 
-----------
create - create the ring at present IP and port
join <ip> <port> - join the ring at address - ip and port p .
port <port> - set the port to port
quit - quit the ring
put <key> <value> - insert <key,value> pair to ring . Here key is integer and value is string.
get <key> - get the key specified by key(integer).
finger - displays addresses of nodes in the finger table.(bonus)
successor - gives the ip of the successor node in ring.(bonus)
predecessor - gives the ip of the predecessor node in ring. (bonus)
dump -  display all information pertaining to calling node.(bonus)


