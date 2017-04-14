package pashttp

import (
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"

	"github.com/gorilla/mux"
)

const debug = false

// HTTPIntermediary handles client-to-pas communication via HTTP.
type HTTPIntermediary struct {
	Router  *mux.Router
	PasConn net.Conn
}

// NewPasHTTPIntermediary returns a new HTTPIntermediary which communicates with
// the pas server located on the given port.
func NewPasHTTPIntermediary(port uint64) (*HTTPIntermediary, error) {
	i := &HTTPIntermediary{Router: mux.NewRouter()}

	// Create a vanilla TCP socket connection. Just use localhost, e.g. `:1234`.
	c, err := net.Dial("tcp", fmt.Sprintf(":%d", port))
	if err != nil {
		return nil, err
	}

	i.PasConn = c
	i.setUpRouter()
	return i, nil
}

func (i *HTTPIntermediary) setUpRouter() {
	i.Router.HandleFunc("/hello", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("Hello world!"))
	})

	i.Router.HandleFunc("/aloha", func(w http.ResponseWriter, r *http.Request) {
		i.WriteToPas([]byte("what is up my dude"))
	})
}

// WriteToPas sends the given data to the pas server.
func (i *HTTPIntermediary) WriteToPas(data []byte) error {
	var wroteLen int

	// Prepend the length of the message.
	dl := make([]byte, 8) // 64 = 8 * 8
	wroteLen = binary.PutVarint(dl, int64(len(data)))
	if wroteLen < 0 {
		return fmt.Errorf("WriteToPas: message is too long to encode")
	}

	payload := append(dl, data...)
	wroteLen, err := i.PasConn.Write(payload)
	if err != nil {
		return err
	}

	if wroteLen != len(payload) {
		return fmt.Errorf("WriteToPas: wanted to send %d bytes, but sent %d",
			len(payload), wroteLen)
	}

	if debug {
		fmt.Printf("successfully wrote to pas!\n")
	}

	return nil
}

// ReadFromPas reads the most recent data available from the pas server.
func (i *HTTPIntermediary) ReadFromPas() ([]byte, error) {
	return ioutil.ReadAll(i.PasConn)
}

// Shutdown contains the destruction logic for an HTTPIntermediary.
func (i *HTTPIntermediary) Shutdown() error {
	return i.PasConn.Close()
}
