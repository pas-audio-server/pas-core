package main

// Taken from https://coderwall.com/p/wohavg/creating-a-simple-tcp-server-in-go.

import (
	"fmt"
	"net"
	"os"
)

const (
	CONN_HOST = "localhost"
	CONN_PORT = "8989"
	CONN_TYPE = "tcp"
	BUFF_SIZE = 1024
)

func main() {
	l, err := net.Listen(CONN_TYPE, CONN_HOST+":"+CONN_PORT)
	if err != nil {
		fmt.Printf("cannot listen: %v\n", err)
		os.Exit(1)
	}
	defer l.Close()

	fmt.Printf("Listening on %s:%s\n\n", CONN_HOST, CONN_PORT)

	// Accept new connections forever.
	for {
		conn, err := l.Accept()
		if err != nil {
			fmt.Printf("cannot accept connection: %v", err)
			os.Exit(1)
		}

		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	buff := make([]byte, BUFF_SIZE)
	defer conn.Close()

	// Read forever until there is an error (or close).
	for {
		readLen, err := conn.Read(buff)
		if err != nil {
			fmt.Printf("cannot read: %v", err)
			break
		}

		// Handle the read content from the connection.
		resp := handleMessage(conn, buff[:readLen])

		conn.Write(resp)
	}
}

func handleMessage(conn net.Conn, buff []byte) []byte {
	// TODO(lozord): Handle the length-encoding prefix.
	s := string(buff)
	fmt.Printf("[RECV]: %q\n", s)
	return buff
}
