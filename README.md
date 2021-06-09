# libnet
a tcp library base on [murraycai/libmc](https://github.com/murrayCai/libbase "libmc")

# this library give these specials :
- event socket base on kqueue
- you can known some times about each pack at below:
1. the time of net pack generated
2. the time of net pack writed to buf
3. the time of net pack be recved time by another side
4. the time of net ack pack be writed to buf by another side
5. the time of net ack pack accepted
6. the time of net fin pack be writed to buf
7. the time of net fin pack be accepted
so, you can known the whole process times of the socket.
- you can safety shutdown socket to avoid the socket pack lost by tcp_client_shut_down() or tcp_server_client_shut_down()
- this library used libmc library,so you can known the whole process of memory malloced and freed,and runtime callstack printf.

# test and useage
this library was writted on freebsd system, other system don't known how about it at now.
```
./compile
./build/test/test_server
./build/test/test_client
```

attention:
- if could not found libmc library, you should copy libmc library source to ./libs/libmc directory 

happy!
