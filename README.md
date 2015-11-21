esp-ginx
--------

##Robust HTTP server for the ESP8266

This project was inspired esp-httpd by Sprite_tm, NodeMcu ( https://github.com/nodemcu/nodemcu-firmware ) and NGINX

Main features
=============

1. Event-driven, single threaded. No blocking, nowhere.
2. DNS Server built in, means we have a captive portal ( will open upon connection os most phones and computers )
3. Host check, means we can ensure server is on here.com and not there.com
4. Redirects ( so #2 will work )
5. HTTP Method enforcement
7. URL rewrite ( very simple one but gives us friendly urls)
8. CGI ( similar to esp-httpd but with more features )
9. CORS enabled 
10. Zero-copy ( or almost ) request parse, inspired on NGINX parse.
11. ROFS compressed with gzip
12. WebSockects! 
12. Bonus http client :)

Overview
-------
After playing around with esp-httpd I decided to extend it and ended up deciding to take it to the next level.
I've used the NodeMcu firmware as base, removed all the lua related code and got a solid framework to work on.
Later I ported Joyent's http parser (https://github.com/joyent/http-parser), which is almost the NGINX implementation, to the esp and developed from there. 

The File System
---------------
I personally didn't like the overhead of decompressing static files to send over http as in esp-httpd, so I created a rom file system that consists of a static FS descriptor and an array with the data concatenated. To keep size short, I compress the files with gzip and send the compressed data stream with the gzip content-encoding. I could fit the whole bootstrap css+js files even if I didn't need to, plus cats pictures to honor Sprite_tm.

Static files are placed in the folder 'html' and there's a tool written in python to create the file system. The file system in the end id just a c file.
Issuing 'Make html' creates/updates the file system.

There's a Spiffs built on the firmware, so a next step should be use it instead, but for my needs right now a static FS is enough.

CGI
---
I don't believe in template html for such low memory devices. I rather prefer the static html + json api approach. So that's what you will see. 

When a request arrives, the server will start to parse it and call a cgi dispatcher on several points of the process, mainly on the http parser callbacks. This allows me to reject a request even before the body was received for example. 
It also allows me not to copy unnecessary data to the precious ESP memory. CGI functions can say which headers they need to read and the parser will extract them as the data comes. We never save the whole request on a buffer.

CGI functions are very flexible. They can allow other rules to proccess or keep the processing to themselfs. We can plugin features like request filtering by inserting wildcard cgi functions on the pipe for example. CORS cgi will plug itself on the pre-headers received and post headers received only.

DNS server and Captive Portal
---------
I wanted to have captive portal-like featured on the server.
To do so the first step was to compile our own version of LwIP stack so we can enable DNS server announce via DHCP. Fortunatelly NodeMCU had LwIP sources already.
We then need to answer some known domains queries with our own ip, so we can capture those requests. Iphones for example to detect captive portal will issue a request to some random apple owned domains expecting 200 OK response, if we respond those requests with a redirect, voilà, the phone will open a login page with anything we like. It also works on PCs, windows will issue a requet to msftncsi.com to check for internet connection. When we redirect it, it will open our esp-ginx server on our loved browser.
There's a list of knows domains on the DNS server. I could just answer all DNS queries with our esp ip, but it would then flood our little esp with nonsense http requests. 

Websockets
---
Why? Mostly because I didn't see it around.
I use it to speed test the esp's tcp capabilities, so a very simple application is written on top of the websocket stack that will keep flushing tcp packets of the chosen size so we can measure how many bytes / second we receive on the other end.

The Demo
--------
This demo application assumes:
* Relays connects to GPIO 5 and 4
* DHT22 sensor connected to GPIO 2

You can :
* Scan available wifi networks
* Connect to an wifi network
* Direct test the relays 
* Read temperature and humidity data
* Do a tcp speed test on ESP8266 using websockets ( no more discussion about it, data wins )
* See the required pictures of cats

![Speed Test](http://i.gyazo.com/89e3fcea70641e871a3bfbaf5d116d66.png)


