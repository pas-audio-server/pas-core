package main

import (
	"flag"
	"fmt"
	"net/http"
	"os"

	pashttp "./pashttp"
)

var (
	pasPort    = flag.Int64("pas_port", 0, "the port of the local pas server")
	serverPort = flag.Int64("server_port", 0, "the port on which to listen for HTTP connections")
)

// TODO(lozord): Add `usage` string. For now, `go run main.go --help` suffices.

func main() {
	flag.Parse()

	ph, err := pashttp.NewPasHTTPIntermediary(uint64(*pasPort))
	if err != nil {
		fmt.Printf("could not create an HTTP intermediary:\n\t%v\n", err)
		os.Exit(1)
	}

	defer ph.Shutdown()

	fmt.Printf("[PASHTTP]: Receiving on local port %d\n", *serverPort)
	http.ListenAndServe(fmt.Sprintf(":%d", *serverPort), ph.Router)
}
