# Rusync
Rusync is a tool which allows you to sync your folder with server one (like gdrive). It consists of 2 main parts - client and server.
## Client
### Usage: rusync_client <path/to/dir> <server_ip> <server_port> \<key\>
Key is some unique string which allows server to distringuish between clients

## Server
### Usage: rusync_server \<ip\> \<port\> \<path/to/dir\>
Internally server will save client files into key/dir_path where key - is key parameter from http API and dir_path provided from command line
## Prerequisites:
cmake, C\+\+20 - complient compiller, conan, git  
Verified setup: cmake/3.16.3, g++/11.1.0, conan/1.38.0  
## Building project
$`git clone `  
$`cd rusync`  
$`git submodule update --init`  
$`mkdir build`  
$`cd build`  
$`cmake ..`  
$`make -j$(nproc)`  
Run client/rusync_client and server/rusync_server 
## Docs:
Doxygen-generated documentation could be found within docs folder. Simply open docs/html/index.html in your browser  
API sequence startup diagram: https://www.websequencediagrams.com/files/render?link=n9DkCe8wlbCsMw5x07yZChXD1HbfnXLqeyo7ivFNBuNfTImHBY7ivTB1t1bJPfi0  
Operational diagram: https://www.websequencediagrams.com/files/render?link=aQ4F6UDqSPAnlbmTkyKQgWIJYbnpaws4ImG8Y2RxsuumqPHlwMTTK0skEyQxUh6P
## Algorithm:
If file was modified client and server both agregate chunks - structure which contains size and hash of chunk. By comparing hash client understans which part of file have changed and send patches (see diagram above).  
## Limitations:
Currently application is not operating properly with large files.    
Build type is hardcoded to DEBUG since nghtt2 have a bug which results in SEGFAULT within library in release mode. 
## TODO:
* Implement file renaming more efficiently
* Add encryption (https)
* Currently application is mostly state-less which is not very efficient. File meta and file caches should be implemented  
* Add more robust class hierarchy which allows proper unit-testing
* Throw libnghtt2_asio to bin and replace with adequate solution


