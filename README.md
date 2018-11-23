# Mock FTP

A 2-connection client-server network application modeled after the File Transfer Protocol (FTP) whereby there is a socket listening on each host that accepts a connection from the other host at the data and control port respectively. Host 1 (indicated below) listens for an incoming connection to the control port socket; host 2 accepts host 1's connection request to the data port socket. If a valid command is made by host 2, it is transmitted to host 1 via the control socket. If there is an error, the error is transmitted from host 1 to host 2 via the control socket; otherwise, host 1 transmits the requested information to host 2 via the data socket.

## Getting Started

These instructions will get you a local copy of the project up and running on your machine.

### Installing
```
git clone https://github.com/code-tsunami/mockftp
cd mockftp
make # run default rule in Makefile
chmod +x ftclient.py # should have execute permissions already but just to be safe
```

### Running ftserver executable on host 1
**Start up server**:
```sh
ftserver <control_port>
```
**To stop server (and close control socket)**:
```
^C (on host 1 -- the host running ftserver)
```

### Running ftclient.py on host 2
**Get file listing in server-side's cwd**:
```sh
ftclient.py <host1_name> <control_port> -l <data_port>
```

**Get file**:
```sh
ftclient.py <host1_name> <control_port> -g <file_to_get> <data_port>
```

<!-- ## Built With

* [text](http://somelink.com) - what it was -->

## Author

* **Sonam Kindy** - [code-tsunami](https://github.com/code-tsunami)

## Acknowledgments

* [Beej's Guide to Network Programming](http://beej.us/guide/bgnet/html/single/bgnet.html)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
