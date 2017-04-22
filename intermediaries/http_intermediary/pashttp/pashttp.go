package pashttp

import (
	"encoding/binary"
	"fmt"
	"net"
	"net/http"

	"github.com/gorilla/mux"
)

const (
	debug                = false
	BUFF_SIZE            = 1024
	BYTES_IN_DATA_LENGTH = 4
)

// TODO(lozord): Make a PasBuffer type for easier serialization.

// HTTPIntermediary handles client-to-pas communication via HTTP.
type HTTPIntermediary struct {
	Router  *mux.Router
	// We keep the connection alive until it is forcibly closed or errors.
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
	/* TESTING ROUTES */
	i.Router.HandleFunc("/hello", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("Hello world!"))
	})

	i.Router.HandleFunc("/aloha", func(w http.ResponseWriter, r *http.Request) {
		i.WriteToPas([]byte("what is up my dude"))
	})

	i.Router.HandleFunc("/wazzup", func(w http.ResponseWriter, r *http.Request) {
		buff := make([]byte, 1024)

		// This makes it easier to view the response in the browser.
		w.Header().Set("Content-Type", "text/html")

		if err := i.WriteToPas([]byte("Hello pas!")); err != nil {
			w.Write([]byte(fmt.Sprintf("write error: %v", err)))
			return
		}

		buff, err := i.ReadFromPas()
		if err != nil {
			w.Write([]byte(fmt.Sprintf("read error: %v", err)))
			return
		}

		w.Write(buff)
	})
	/* END TESTING ROUTES */
}

// WriteToPas sends the given data to the pas server.
func (i *HTTPIntermediary) WriteToPas(data []byte) error {
	var wroteLen int

	// Prepend the length of the message.
	dataLenBuff := make([]byte, BYTES_IN_DATA_LENGTH)
	wroteLen = binary.PutVarint(dataLenBuff, int64(len(data)))
	if wroteLen < 0 {
		return fmt.Errorf("WriteToPas: message is too long to encode")
	}

	// TODO(lozord): Test the length-encoding prefix.
	// TODO(lozord): Separately send the length and data (see wiki).
	payload := append(dataLenBuff, data...)
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
	buff := make([]byte, BUFF_SIZE)
	// TODO(lozord): Need to correctly implement frame handling (see wiki).
	readLen, err := i.PasConn.Read(buff)
	if err != nil {
		return nil, err
	}

	if readLen < BYTES_IN_DATA_LENGTH {
		return nil, fmt.Errorf(
			"read only %d bytes from pas, wanted at least %d",
			readLen, BYTES_IN_DATA_LENGTH,
		)
	}

	dataLen, bytesRead := binary.Varint(buff[:BYTES_IN_DATA_LENGTH])
	if bytesRead < BYTES_IN_DATA_LENGTH {
		return nil, fmt.Errorf("could not read data length from the payload")
	}

	// Sanity check.
	if expectedLen := int(BYTES_IN_DATA_LENGTH + dataLen); readLen != expectedLen {
		return nil, fmt.Errorf(
			"read %d bytes from pas, but given (size + data) = %d was not equal",
			readLen, expectedLen,
		)
	}

	return buff[BYTES_IN_DATA_LENGTH:], nil
}

// Shutdown contains the destruction logic for an HTTPIntermediary.
func (i *HTTPIntermediary) Shutdown() error {
	return i.PasConn.Close()
}
