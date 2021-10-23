# Rusync
Rusync is a tool which allows you to sync your folder with server one (like gdrive). It consists of 2 main parts - client and server.
## Client
### Usage: rusync_client <path/to/folder> <server_ip> <server_port> <key>

## Server
### Usage: rusync_server <ip> <port> <path/to/dir>
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
Doxygen-generated documentation could be found within docs folder. Simple open docs/html/index.html in your browser
API sequence startup diagram: https://www.websequencediagrams.com/files/render?link=n9DkCe8wlbCsMw5x07yZChXD1HbfnXLqeyo7ivFNBuNfTImHBY7ivTB1t1bJPfi0
Operational diagram: https://www.websequencediagrams.com/files/render?link=aQ4F6UDqSPAnlbmTkyKQgWIJYbnpaws4ImG8Y2RxsuumqPHlwMTTK0skEyQxUh6P